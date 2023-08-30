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
#include "Tiled2dMapVectorLayerConfig.h"
#include "RenderObject.h"
#include "MapCamera2dInterface.h"
#include "earcut.hpp"
#include "Logger.h"
#include "PolygonHelper.h"
#include "CoordinateSystemIdentifiers.h"
#include "Tiled2dMapVectorStyleParser.h"


Tiled2dMapVectorPolygonTile::Tiled2dMapVectorPolygonTile(const std::weak_ptr<MapInterface> &mapInterface,
                                                         const Tiled2dMapTileInfo &tileInfo,
                                                         const WeakActor<Tiled2dMapVectorLayerTileCallbackInterface> &tileCallbackInterface,
                                                         const std::shared_ptr<PolygonVectorLayerDescription> &description,
                                                         const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                                         const std::shared_ptr<Tiled2dMapVectorFeatureStateManager> &featureStateManager)
        : Tiled2dMapVectorTile(mapInterface, tileInfo, description, layerConfig, tileCallbackInterface, featureStateManager), usedKeys(std::move(description->getUsedKeys())) {
    isStyleZoomDependant = usedKeys.find(Tiled2dMapVectorStyleParser::zoomExpression) != usedKeys.end();
    isStyleFeatureStateDependant = usedKeys.find(Tiled2dMapVectorStyleParser::featureStateExpression) != usedKeys.end();
}

void Tiled2dMapVectorPolygonTile::updateVectorLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                                         const Tiled2dMapVectorTileDataVector &tileData) {
    Tiled2dMapVectorTile::updateVectorLayerDescription(description, tileData);
    const auto newUsedKeys = description->getUsedKeys();
    bool usedKeysContainsNewUsedKeys = true;
    for (const auto &key : newUsedKeys ) {
        if (usedKeys.count(key) == 0) {
            usedKeysContainsNewUsedKeys = false;
            break;
        }
    }
    isStyleZoomDependant = usedKeys.find(Tiled2dMapVectorStyleParser::zoomExpression) != usedKeys.end();
    isStyleFeatureStateDependant = usedKeys.find(Tiled2dMapVectorStyleParser::featureStateExpression) != usedKeys.end();
    lastZoom = std::nullopt;
    lastAlpha = std::nullopt;

    if (usedKeysContainsNewUsedKeys) {
        auto selfActor = WeakActor(mailbox, shared_from_this()->weak_from_this());
        selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorPolygonTile::update);

        tileCallbackInterface.message(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady, tileInfo, description->identifier, WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this()));
    } else {
        usedKeys = std::move(newUsedKeys);
        featureGroups.clear();
        styleHashToGroupMap.clear();
        hitDetectionPolygons.clear();
        toClear.insert(toClear.begin(), polygons.begin(), polygons.end());
        polygons.clear();
        shaders.clear();
        setVectorTileData(tileData);
    }
}

void Tiled2dMapVectorPolygonTile::update() {
    if (shaders.empty()) {
        return;
    }

    if (!toClear.empty()) {
        for (auto const &polygon: toClear) {
            if (polygon->getPolygonObject()->isReady()) polygon->getPolygonObject()->clear();
        }
        toClear.clear();
    }

    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!mapInterface || !camera) {
        return;
    }

    double zoomIdentifier = layerConfig->getZoomIdentifier(camera->getZoom());
    zoomIdentifier = std::max(zoomIdentifier, (double) tileInfo.zoomIdentifier);

    auto polygonDescription = std::static_pointer_cast<PolygonVectorLayerDescription>(description);
    bool inZoomRange = polygonDescription->maxZoom >= zoomIdentifier && polygonDescription->minZoom <= zoomIdentifier;
    
    if (lastZoom &&
        ((isStyleZoomDependant && *lastZoom == zoomIdentifier) || !isStyleZoomDependant) &&
        lastAlpha == alpha &&
        (lastInZoomRange && *lastInZoomRange == inZoomRange) &&
        !isStyleFeatureStateDependant) {
        return;
    }
    lastZoom = zoomIdentifier;
    lastAlpha = alpha;
    lastInZoomRange = inZoomRange;

    size_t numStyleGroups = featureGroups.size();
    for (int styleGroupId = 0; styleGroupId < numStyleGroups; styleGroupId++) {
        std::vector<float> shaderStyles;
        shaderStyles.reserve(featureGroups.at(styleGroupId).size() * 5);
        for (auto const &[hash, feature]: featureGroups.at(styleGroupId)) {
            const auto& ec = EvaluationContext(zoomIdentifier, feature, featureStateManager);
            const auto& color = inZoomRange ? polygonDescription->style.getFillColor(ec) : Color(0.0, 0.0, 0.0, 0.0);
            const auto& opacity = inZoomRange ? polygonDescription->style.getFillOpacity(ec) : 0.0;
            shaderStyles.push_back(color.r);
            shaderStyles.push_back(color.g);
            shaderStyles.push_back(color.b);
            shaderStyles.push_back(color.a);
            shaderStyles.push_back(opacity * alpha);
        }
        auto s = SharedBytes((int64_t)shaderStyles.data(), (int32_t)featureGroups.at(styleGroupId).size(), 5 * (int32_t)sizeof(float));
        shaders[styleGroupId]->setStyles(s);
    }
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
    auto mapInterface = this->mapInterface.lock();
    const auto &shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
    if (!shaderFactory) {
        return;
    }

    const std::string layerName = description->sourceLayer;

    const auto indicesLimit = std::numeric_limits<uint16_t>::max();

    if (!tileData->empty()) {

        bool anyInteractable = false;

        std::unordered_map<int, std::vector<ObjectDescriptions>> styleGroupNewPolygonsMap;
        std::unordered_map<int, int32_t> styleIndicesOffsets;

        for (auto featureIt = tileData->begin(); featureIt != tileData->end(); featureIt++) {

            const auto [featureContext, geometryHandler] = *featureIt;

            if (featureContext->geomType != vtzero::GeomType::POLYGON) { continue; }

            EvaluationContext evalContext = EvaluationContext(tileInfo.zoomIdentifier, featureContext, featureStateManager);
            if (description->filter == nullptr || description->filter->evaluateOr(evalContext, true)) {

                int styleIndex = -1;
                int styleGroupIndex = -1;
                
                {
                    auto const hash = featureContext->getStyleHash(usedKeys);

                    auto indexPair = styleHashToGroupMap.find(hash);
                    if (indexPair != styleHashToGroupMap.end()) {
                        styleGroupIndex = indexPair->second.first;
                        styleIndex = indexPair->second.second;
                    }

                    if (styleIndex == -1) {
                        if (!featureGroups.empty() && featureGroups.back().size() < maxStylesPerGroup) {
                            styleGroupIndex = (int) featureGroups.size() - 1;
                            styleIndex = (int) featureGroups.back().size();
                            featureGroups.at(styleGroupIndex).emplace_back(hash, featureContext);
                        } else {
                            styleGroupIndex = (int) featureGroups.size();
                            styleIndex = 0;
                            auto shader = shaderFactory->createPolygonGroupShader();
                            auto polygonDescription = std::static_pointer_cast<PolygonVectorLayerDescription>(description);
                            shader->asShaderProgramInterface()->setBlendMode(polygonDescription->style.getBlendMode(EvaluationContext(0.0, std::make_shared<FeatureContext>(), featureStateManager)));
                            shaders.push_back(shader);
                            featureGroups.push_back(std::vector<std::tuple<size_t, std::shared_ptr<FeatureContext>>>{{hash, featureContext}});
                            styleGroupNewPolygonsMap[styleGroupIndex].push_back({{},{}});
                            styleIndicesOffsets[styleGroupIndex] = 0;
                        }
                        styleHashToGroupMap.insert({hash, {styleGroupIndex, styleIndex}});
                    }
                }
                const auto &polygons = geometryHandler->getPolygons();

                for (int i = 0; i < polygons.size(); i++) {
                    const auto &polygon = polygons[i];

                    size_t verticesCount = polygon.coordinates.size();

                    // more complex polygons may need to be simplified on-device to render them correctly
                    if (verticesCount >= indicesLimit) {
                        LogError <<= "Too many vertices in a polygon to use 16bit indices: " + std::to_string(verticesCount);
                        continue;
                    }

                    // check overflow
                    int32_t indexOffset = styleIndicesOffsets.at(styleGroupIndex);
                    size_t new_size = indexOffset + verticesCount;
                    if (new_size >= indicesLimit) {
                        styleGroupNewPolygonsMap[styleGroupIndex].push_back({{},{}});
                        styleIndicesOffsets[styleGroupIndex] = 0;
                        indexOffset = 0;
                    }

                    for (auto const &index: polygon.indices) {
                        styleGroupNewPolygonsMap.at(styleGroupIndex).back().indices.push_back(indexOffset + index);
                    }

                    for (auto const &coordinate: polygon.coordinates) {
                        styleGroupNewPolygonsMap.at(styleGroupIndex).back().vertices.push_back(coordinate.x);
                        styleGroupNewPolygonsMap.at(styleGroupIndex).back().vertices.push_back(coordinate.y);
                        styleGroupNewPolygonsMap.at(styleGroupIndex).back().vertices.push_back(styleIndex);
                    }

                    styleIndicesOffsets.at(styleGroupIndex) += verticesCount;

                    bool interactable = description->isInteractable(evalContext);
                    if (interactable) {
                        anyInteractable = true;
                        hitDetectionPolygons.emplace_back(polygon, featureContext);
                    }
                }
            }
        }

        if (anyInteractable) {
            tileCallbackInterface.message(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsInteractable, description->identifier);
        }

        addPolygons(styleGroupNewPolygonsMap);
    } else {
        auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
        tileCallbackInterface.message(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady, tileInfo, description->identifier, selfActor);
    }
}

void Tiled2dMapVectorPolygonTile::addPolygons(const std::unordered_map<int, std::vector<ObjectDescriptions>> &styleGroupNewPolygonsMap) {

    if (styleGroupNewPolygonsMap.empty()) {
        auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
        tileCallbackInterface.message(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady, tileInfo, description->identifier, selfActor);
        return;
    }

    auto mapInterface = this->mapInterface.lock();
    auto objectFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
    auto converter = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;

    if (!mapInterface || !objectFactory || !converter || shaders.empty()) {
        return;
    }

    std::vector<std::shared_ptr<PolygonGroup2dLayerObject>> polygonGroupObjects;
    std::vector<std::shared_ptr<GraphicsObjectInterface>> newGraphicObjects;

    for (auto const &[styleGroupIndex, polygonDescs]: styleGroupNewPolygonsMap) {
        const auto &shader = shaders.at(styleGroupIndex);
        for (const auto &polygonDesc: polygonDescs) {
            const auto polygonObject = objectFactory->createPolygonGroup(shader->asShaderProgramInterface());

            auto layerObject = std::make_shared<PolygonGroup2dLayerObject>(converter, polygonObject, shader);
            layerObject->setVertices(polygonDesc.vertices, polygonDesc.indices);

            polygonGroupObjects.emplace_back(layerObject);
            newGraphicObjects.push_back(polygonObject->asGraphicsObject());
        }
    }

    polygons = polygonGroupObjects;

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
    auto strongSelectionDelegate = selectionDelegate.lock();
    if (!camera || !strongSelectionDelegate || !converter) {
        return false;
    }
    auto point = camera->coordFromScreenPosition(posScreen);

    for (auto const &[polygon, featureContext]: hitDetectionPolygons) {
        if (VectorTileGeometryHandler::isPointInTriangulatedPolygon(point, polygon, converter)) {
            strongSelectionDelegate->didSelectFeature(featureContext->getFeatureInfo(), description->identifier,
                                                converter->convert(CoordinateSystemIdentifiers::EPSG4326(), point));
            return true;
        }
    }

    return false;
}
