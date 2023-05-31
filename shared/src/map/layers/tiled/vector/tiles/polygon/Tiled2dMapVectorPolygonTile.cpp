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
#include "Tiled2dMapVectorRasterSubLayerConfig.h"
#include "RenderObject.h"
#include "MapCamera2dInterface.h"
#include "earcut.hpp"
#include "Logger.h"
#include "PolygonHelper.h"
#include "CoordinateSystemIdentifiers.h"

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
                                                         const WeakActor<Tiled2dMapVectorLayerTileCallbackInterface> &tileCallbackInterface,
                                                         const std::shared_ptr<PolygonVectorLayerDescription> &description)
        : Tiled2dMapVectorTile(mapInterface, tileInfo, description, tileCallbackInterface), usedKeys(std::move(description->getUsedKeys())) {
    auto pMapInterface = mapInterface.lock();
    if (pMapInterface) {
        shader = pMapInterface->getShaderFactory()->createPolygonGroupShader();
        shader->asShaderProgramInterface()->setBlendMode(description->style.getBlendMode(EvaluationContext(std::nullopt, FeatureContext())));
    }
}

void Tiled2dMapVectorPolygonTile::updateVectorLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                                         const Tiled2dMapVectorTileDataVector &tileData) {
    Tiled2dMapVectorTile::updateVectorLayerDescription(description, tileData);
    featureGroups.clear();
    hitDetectionPolygonMap.clear();
    usedKeys = std::move(description->getUsedKeys());
    setVectorTileData(tileData);
}

void Tiled2dMapVectorPolygonTile::update() {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!mapInterface || !camera) {
        return;
    }
    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(camera->getZoom());
    zoomIdentifier = std::max(zoomIdentifier, (double) tileInfo.zoomIdentifier);

    std::vector<float> shaderStyles;
    auto polygonDescription = std::static_pointer_cast<PolygonVectorLayerDescription>(description);
    for (auto const &[hash, feature]: featureGroups) {
        const auto& ec = EvaluationContext(zoomIdentifier, feature);
        const auto& color = polygonDescription->style.getFillColor(ec);
        const auto& opacity = polygonDescription->style.getFillOpacity(ec);

        shaderStyles.push_back(color.r);
        shaderStyles.push_back(color.g);
        shaderStyles.push_back(color.b);
        shaderStyles.push_back(color.a);
        shaderStyles.push_back(opacity * alpha);
    }

    auto s = SharedBytes((int64_t)shaderStyles.data(), (int32_t)featureGroups.size(), 5 * (int32_t)sizeof(float));
    shader->setStyles(s);
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

    auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
    tileCallbackInterface.message(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady, tileInfo, description->identifier, selfActor);
}

void Tiled2dMapVectorPolygonTile::setVectorTileData(const Tiled2dMapVectorTileDataVector &tileData) {

    if (!mapInterface.lock()) {
        return;
    }

    const std::string layerName = description->sourceId;

    const auto indicesLimit = std::numeric_limits<uint16_t>::max();

    if (!tileData->empty()) {

        bool anyInteractable = false;

        std::vector<ObjectDescriptions> objectDescriptions;
        objectDescriptions.push_back({{},{}});

        std::int32_t indices_offset = 0;

#ifndef __APPLE__
        for (auto featureIt = tileData->rbegin(); featureIt != tileData->rend(); featureIt++) {
#else
        for (auto featureIt = tileData->begin(); featureIt != tileData->end(); featureIt++) {
#endif

            const auto &[featureContext, geometryHandler] = *featureIt;

            if (featureContext.geomType != vtzero::GeomType::POLYGON) { continue; }

            EvaluationContext evalContext = EvaluationContext(tileInfo.zoomIdentifier, featureContext);
            if (description->filter == nullptr || description->filter->evaluateOr(evalContext, true)) {
                const auto &polygonCoordinates = geometryHandler.getPolygonCoordinates();
                const auto &polygonHoles = geometryHandler.getHoleCoordinates();

                std::vector<Coord> positions;

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

                bool interactable = description->isInteractable(evalContext);
                for (int i = 0; i < polygonCoordinates.size(); i++) {

                    size_t verticesCount = polygonCoordinates[i].size();

                    // TODO: Android/iOS currently only supports 16bit indices
                    // more complex polygons may need to be simplified on-device to render them correctly
                    if (verticesCount >= indicesLimit) {
                        LogError <<= "Too many vertices in a polygon to use 16bit indices: " + std::to_string(verticesCount);
                        continue;
                    }

                    std::vector<std::vector<::Coord>> pol = { polygonCoordinates[i] };

                    for (auto const &hole: polygonHoles[i]) {

                        if (verticesCount + hole.size() >= indicesLimit) {
                            LogError <<= "Too many vertices by polygon holes to use 16bit indices - remaining holes are dropped";
                            break;
                        }

                        verticesCount += hole.size();
                        pol.push_back(hole);
                    }

                    std::vector<uint16_t> new_indices = mapbox::earcut<uint16_t>(pol);

                    for (auto const &coords: pol) {
                        positions.insert(positions.end(), coords.begin(), coords.end());
                    }

                    // check overflow
                    size_t new_size = indices_offset + verticesCount;

                    if (new_size >= indicesLimit) {
                        objectDescriptions.push_back({{},{}});
                        indices_offset = 0;
                    }

                    for (auto const &index: new_indices) {
                        objectDescriptions.back().indices.push_back(indices_offset + index);
                    }

                    objectDescriptions.back().vertices.push_back({positions, styleIndex});
                    positions.clear();

                    indices_offset += verticesCount;

                    if (interactable) {
                        anyInteractable = true;
                        hitDetectionPolygonMap[tileInfo].push_back(
                                {PolygonCoord(polygonCoordinates[i], polygonHoles[i]), featureContext});
                    }
                }
            }
        }

        if (anyInteractable) {
            tileCallbackInterface.message(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsInteractable, description->identifier);
        }

        addPolygons(objectDescriptions);
    } else {
        auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
        tileCallbackInterface.message(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady, tileInfo, description->identifier, selfActor);
    }
}

void Tiled2dMapVectorPolygonTile::addPolygons(const std::vector<ObjectDescriptions> &polygons) {

    if (polygons.empty()) {
        auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
        tileCallbackInterface.message(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady, tileInfo, description->identifier, selfActor);
        return;
    }

    auto mapInterface = this->mapInterface.lock();
    auto objectFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
    auto scheduler = mapInterface ? mapInterface->getScheduler() : nullptr;
    auto converter = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;

    if (!mapInterface || !objectFactory || !scheduler || !converter || !shader) {
        return;
    }

    std::vector<std::shared_ptr<GraphicsObjectInterface>> newGraphicObjects;

    for (auto const& polygon: polygons) {
        const auto polygonObject = objectFactory->createPolygonGroup(shader->asShaderProgramInterface());

        auto layerObject = std::make_shared<PolygonGroup2dLayerObject>(converter, polygonObject, shader);
        layerObject->setVertices(polygon.vertices, polygon.indices);

        this->polygons.emplace_back(layerObject);
        newGraphicObjects.push_back(polygonObject->asGraphicsObject());
    }

#ifdef __APPLE__
    setupPolygons(newGraphicObjects);
#else
    auto selfActor = WeakActor(mailbox, shared_from_this()->weak_from_this());
    selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorPolygonTile::setupPolygons, newGraphicObjects);
#endif
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

    auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
    tileCallbackInterface.message(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady, tileInfo, description->identifier, selfActor);
}

std::vector<std::shared_ptr<RenderObjectInterface>> Tiled2dMapVectorPolygonTile::generateRenderObjects() {
    std::vector<std::shared_ptr<RenderObjectInterface>> newRenderObjects;
    for (auto const &object : polygons) {
        for (const auto &config : object->getRenderConfig()) {
            newRenderObjects.push_back(std::make_shared<RenderObject>(config->getGraphicsObject()));
        }
    }

    return newRenderObjects;
}

bool Tiled2dMapVectorPolygonTile::onClickConfirmed(const Vec2F &posScreen) {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto converter = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    if (!camera || !selectionDelegate || !converter) {
        return false;
    }
    auto point = camera->coordFromScreenPosition(posScreen);

    for (auto const &[tileInfo, polygonTuples] : hitDetectionPolygonMap) {
        for (auto const &[polygon, featureContext]: polygonTuples) {
            if (PolygonHelper::pointInside(polygon, point, mapInterface->getCoordinateConverterHelper())) {
                selectionDelegate->didSelectFeature(featureContext.getFeatureInfo(), description->identifier, converter->convert(CoordinateSystemIdentifiers::EPSG4326(), point));
                return true;
            }
        }
    }

    return false;
}
