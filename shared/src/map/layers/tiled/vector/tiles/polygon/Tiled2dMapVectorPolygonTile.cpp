/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorPolygonTile.h"
#include "Tiled2dMapVectorLayer.h"
#include "Tiled2dMapVectorRasterSubLayerConfig.h"
#include "RenderObject.h"
#include "RenderPass.h"
#include "MapCamera2dInterface.h"
#include "earcut.hpp"
#include "Logger.h"
#include "PolygonHelper.h"

namespace mapbox {
    namespace util {

        template <>
        struct nth<0, ::Coord> {
            inline static auto get(const ::Coord &t) {
                return t.x;
            };
        };
        template <>
        struct nth<1, ::Coord> {
            inline static auto get(const ::Coord &t) {
                return t.y;
            };
        };

    } // namespace util
} // namespace mapbox

Tiled2dMapVectorPolygonTile::Tiled2dMapVectorPolygonTile(const std::weak_ptr<MapInterface> &mapInterface,
                                                         const Tiled2dMapTileInfo &tileInfo,
                                                         const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                                         const std::shared_ptr<PolygonVectorLayerDescription> &description)
        : Tiled2dMapVectorTile(mapInterface, tileInfo, vectorLayer), description(description) {
    usedKeys = std::move(description->getUsedKeys());
    auto pMapInterface = mapInterface.lock();
    if (pMapInterface) {
        shader = pMapInterface->getShaderFactory()->createPolygonGroupShader();
    }
}

void Tiled2dMapVectorPolygonTile::update() {
    auto mapInterface = this->mapInterface.lock();
    if (!mapInterface) {
        return;
    }
    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(mapInterface->getCamera()->getZoom());

    std::vector<float> shaderStyles;

    for (auto const &[hash, feature]: featureGroups) {
        const auto& ec = EvaluationContext(zoomIdentifier, feature);
        const auto& color = description->style.getFillColor(ec);
        const auto& opacity = description->style.getFillOpacity(ec);

        shaderStyles.push_back(color.r);
        shaderStyles.push_back(color.g);
        shaderStyles.push_back(color.b);
        shaderStyles.push_back(color.a);
        shaderStyles.push_back(opacity * alpha);
    }

    auto s = SharedBytes((int64_t)shaderStyles.data(), (int32_t)featureGroups.size(), 5 * (int32_t)sizeof(float));
    shader->setStyles(s);
}

std::vector<std::shared_ptr<RenderPassInterface>> Tiled2dMapVectorPolygonTile::buildRenderPasses() {
    return renderPasses;
}

void Tiled2dMapVectorPolygonTile::clear() {
    for (auto const &polygon: polygons) {
        if (polygon->getPolygonObject()->isReady()) polygon->getPolygonObject()->clear();
    }
}

void Tiled2dMapVectorPolygonTile::setup() {
    auto mapInterface = this->mapInterface.lock();
    if (!mapInterface) {
        return;
    }
    const auto &context = mapInterface->getRenderingContext();
    for (auto const &polygon: polygons) {
        if (!polygon->getPolygonObject()->isReady()) polygon->getPolygonObject()->setup(context);
    }
    vectorLayer.message(&Tiled2dMapVectorLayer::tileIsReady, tileInfo);
}

void Tiled2dMapVectorPolygonTile::setScissorRect(const std::optional<::RectI> &scissorRect) {
    this->scissorRect = scissorRect;
    preGenerateRenderPasses();
}

void Tiled2dMapVectorPolygonTile::setTileData(const std::shared_ptr<MaskingObjectInterface> &tileMask,
                 const std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>> &layerFeatures) {

    if (!mapInterface.lock()) {
        return;
    }

    this->tileMask = tileMask;

    std::string layerName = description->sourceId;

    std::string defIdPrefix =
            std::to_string(tileInfo.x) + "/" + std::to_string(tileInfo.y) + "_" + layerName + "_";
    if (!layerFeatures.empty() &&
        description->minZoom <= tileInfo.zoomIdentifier &&
        description->maxZoom >= tileInfo.zoomIdentifier) {

        std::vector<std::tuple<std::vector<std::tuple<std::vector<Coord>, int>>, std::vector<int32_t>>> objectDescriptions;
        objectDescriptions.push_back({{},{}});

        std::vector<int32_t> indices;
        std::int32_t indices_offset = 0;
        for (const auto &feature : layerFeatures) {
            const FeatureContext &featureContext = std::get<0>(feature);

            if (featureContext.geomType != vtzero::GeomType::POLYGON) { continue; }

            if (description->filter == nullptr || description->filter->evaluateOr(EvaluationContext(-1, featureContext), true)) {
                const auto &geometryHandler = std::get<1>(feature);
                const auto &polygonCoordinates = geometryHandler.getPolygonCoordinates();
                const auto &polygonHoles = geometryHandler.getHoleCoordinates();

                std::vector<Coord> positions;

                for (int i = 0; i < polygonCoordinates.size(); i++) {

                    size_t verticesCount = polygonCoordinates[i].size();
                    std::vector<std::vector<::Coord>> pol = {polygonCoordinates[i]};
                    for (auto const &hole: polygonHoles[i]) {
                        verticesCount += polygonHoles[i].size();
                        pol.push_back(hole);
                    }

#ifndef __APPLE__
                    // TODO: android currently only supports 16bit indices
                    // more complex polygons may need to be simplified on-device to render them correctly
                    if (verticesCount >= std::numeric_limits<uint16_t>::max()) {
                        LogError <<= "Too many vertices to use 16bit indices: " + std::to_string(verticesCount);
                        continue;
                    }
#endif

                    std::vector<uint32_t> new_indices = mapbox::earcut<uint32_t>(pol);

                    size_t posAdded = 0;
                    for (auto const &coords: pol) {
                        positions.insert(positions.end(), coords.begin(), coords.end());
                        posAdded += coords.size();
                    }

                    // check overflow
                    size_t new_size = indices_offset + posAdded;
#ifdef __APPLE__
                    if (new_size >= std::numeric_limits<uint32_t>::max()) {
                        objectDescriptions.push_back({{}, {}});
                        indices_offset = 0;
                    }
#else
                    if (new_size >= std::numeric_limits<uint16_t>::max()) {
                        objectDescriptions.push_back({{}, {}});
                        indices_offset = 0;
                    }
#endif

                    for (auto const &index: new_indices) {
                        std::get<1>(objectDescriptions.back()).push_back(indices_offset + index);
                    }

                    indices_offset += posAdded;

                    hitDetectionPolygonMap[tileInfo].push_back({PolygonCoord(polygonCoordinates[i], polygonHoles[i]), featureContext});
                }

                int styleIndex = -1;
                {
                    auto const hash = featureContext.getStyleHash(usedKeys);

                    for (size_t i = 0; i != featureGroups.size(); i++) {
                        auto const &[groupHash, group] = featureGroups.at(i);
                        if (hash == groupHash) {
                            styleIndex = (int) i;
                            break;
                        }
                    }

                    if (styleIndex == -1) {
                        featureGroups.push_back({ hash, featureContext });
                        styleIndex = (int) featureGroups.size() - 1;
                    }
                }

                std::get<0>(objectDescriptions.back()).push_back({positions, styleIndex});
            }
        }

        addPolygons(objectDescriptions);
    } else {
        vectorLayer.message(&Tiled2dMapVectorLayer::tileIsReady, tileInfo);
    }
}

void Tiled2dMapVectorPolygonTile::updateTileMask(const std::shared_ptr<MaskingObjectInterface> &tileMask) {
    this->tileMask = tileMask;
    preGenerateRenderPasses();
}

void Tiled2dMapVectorPolygonTile::addPolygons(const std::vector<std::tuple<std::vector<std::tuple<std::vector<Coord>, int>>, std::vector<int32_t>>> &polygons) {
    if (polygons.empty()) {
        vectorLayer.message(&Tiled2dMapVectorLayer::tileIsReady, tileInfo);
        return;
    }

    auto mapInterface = this->mapInterface.lock();
    auto objectFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
    auto scheduler = mapInterface ? mapInterface->getScheduler() : nullptr;
    auto converter = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;

    if (!mapInterface || !objectFactory || !scheduler || !converter || !shader) {
        return;
    }

    std::vector<std::shared_ptr<PolygonGroup2dLayerObject>> polygonObjects;
    std::vector<std::shared_ptr<GraphicsObjectInterface>> newGraphicObjects;

    for (auto const& tuple: polygons) {
        const auto polygonObject = objectFactory->createPolygonGroup(shader->asShaderProgramInterface());

        auto layerObject = std::make_shared<PolygonGroup2dLayerObject>(converter,
                                                                       polygonObject, shader);


        layerObject->setVertices(std::get<0>(tuple), std::get<1>(tuple));

        this->polygons.emplace_back(layerObject);
        newGraphicObjects.push_back(polygonObject->asGraphicsObject());
    }

#ifdef __APPLE__
    setupPolygons(newGraphicObjects);
#else
    auto selfActor = WeakActor(mailbox, shared_from_this()->weak_from_this());
    selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorPolygonTile::setupPolygons, newGraphicObjects);
#endif
    
    preGenerateRenderPasses();
}

void Tiled2dMapVectorPolygonTile::setupPolygons(const std::vector<std::shared_ptr<GraphicsObjectInterface>> &newPolygonObjects) {
    auto mapInterface = this->mapInterface.lock();
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) {
        return;
    }

    for (auto const &polygon: newPolygonObjects) {
        if (!polygon->isReady()) polygon->setup(renderingContext);
    }

    vectorLayer.message(&Tiled2dMapVectorLayer::tileIsReady, tileInfo);
}

void Tiled2dMapVectorPolygonTile::preGenerateRenderPasses() {
    std::vector<std::shared_ptr<RenderPassInterface>> newRenderPasses;

    if (!tileMask) {
        renderPasses = newRenderPasses;
        return;
    }

    std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> renderPassObjectMap;
    for (auto const &object : polygons) {
        for (const auto &config : object->getRenderConfig()) {
            renderPassObjectMap[config->getRenderIndex()].push_back(
                    std::make_shared<RenderObject>(config->getGraphicsObject()));
        }
    }

    for (const auto &passEntry : renderPassObjectMap) {
        std::shared_ptr<RenderPass> renderPass = std::make_shared<RenderPass>(
                RenderPassConfig(description->renderPassIndex.value_or(passEntry.first)),
                passEntry.second, tileMask);
        renderPass->setScissoringRect(scissorRect);
        newRenderPasses.push_back(renderPass);
    }

    renderPasses = newRenderPasses;
}

bool Tiled2dMapVectorPolygonTile::onClickConfirmed(const Vec2F &posScreen) {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!camera || selectionDelegate) {
        return false;
    }
    auto point = camera->coordFromScreenPosition(posScreen);

    for (auto const &[tileInfo, polygonTuples] : hitDetectionPolygonMap) {
        for (auto const &[polygon, featureContext]: polygonTuples) {
            if (PolygonHelper::pointInside(polygon, point, mapInterface->getCoordinateConverterHelper())) {
                selectionDelegate.message(&Tiled2dMapVectorLayerSelectionInterface::didSelectFeature, featureContext, description, point);
                return true;
            }
        }
    }

    return false;
}
