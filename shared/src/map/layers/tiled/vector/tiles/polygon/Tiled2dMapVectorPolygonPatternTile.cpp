/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorPolygonPatternTile.h"
#include "Tiled2dMapVectorLayerConfig.h"
#include "RenderObject.h"
#include "MapCameraInterface.h"
#include "PolygonGroupShaderInterface.h"
#include "PolygonPatternGroup2dLayerObject.h"
#include "earcut.hpp"
#include "Logger.h"
#include "PolygonHelper.h"
#include "TextureHolderInterface.h"
#include "Tiled2dMapVectorStyleParser.h"

Tiled2dMapVectorPolygonPatternTile::Tiled2dMapVectorPolygonPatternTile(const std::weak_ptr<MapInterface> &mapInterface,
                                                                       const Tiled2dMapVersionedTileInfo &tileInfo,
                                                                       const WeakActor<Tiled2dMapVectorLayerTileCallbackInterface> &tileCallbackInterface,
                                                                       const std::shared_ptr<PolygonVectorLayerDescription> &description,
                                                                       const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                                                       const std::shared_ptr<SpriteData> &spriteData,
                                                                       const std::shared_ptr<TextureHolderInterface> &spriteTexture,
                                                                       const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager)
        : Tiled2dMapVectorTile(mapInterface, tileInfo, description, layerConfig, tileCallbackInterface, featureStateManager),
          spriteData(spriteData), spriteTexture(spriteTexture), usedKeys(description->getUsedKeys()), fadeInPattern(description->style.fadeInPattern) {
    isStyleZoomDependant = usedKeys.containsUsedKey(Tiled2dMapVectorStyleParser::zoomExpression);
    isStyleStateDependant = usedKeys.isStateDependant();
}

void Tiled2dMapVectorPolygonPatternTile::updateVectorLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                                         const Tiled2dMapVectorTileDataVector &tileData) {
    Tiled2dMapVectorTile::updateVectorLayerDescription(description, tileData);
    const auto newUsedKeys = description->getUsedKeys();
    bool usedKeysContainsNewUsedKeys = usedKeys.covers(newUsedKeys);
    isStyleZoomDependant = newUsedKeys.containsUsedKey(Tiled2dMapVectorStyleParser::zoomExpression);
    isStyleStateDependant = newUsedKeys.isStateDependant();
    usedKeys = newUsedKeys;
    lastZoom = std::nullopt;
    lastAlpha = std::nullopt;

    if (usedKeysContainsNewUsedKeys) {
        auto selfActor = WeakActor(mailbox, shared_from_this()->weak_from_this());
        selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorPolygonPatternTile::update);
        
        tileCallbackInterface.message(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady, tileInfo, description->identifier, WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this()));
    } else {
        usedKeys = std::move(newUsedKeys);
        featureGroups.clear();
        styleHashToGroupMap.clear();
        hitDetectionPolygons.clear();
        for (const auto polygons: styleGroupPolygonsMap) {
            toClear.insert(toClear.begin(), polygons.second.begin(), polygons.second.end());
        }
        styleGroupPolygonsMap.clear();
        polygonRenderOrder.clear();
        shaders.clear();
        setVectorTileData(tileData);
    }
}

void Tiled2dMapVectorPolygonPatternTile::update() {
    if (shaders.empty()) {
        return;
    }

    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!mapInterface || !camera) {
        return;
    }

    double cameraZoom = camera->getZoom();
    double zoomIdentifier = layerConfig->getZoomIdentifier(cameraZoom);
    if (!mapInterface->is3d()) {
        zoomIdentifier = std::max(zoomIdentifier, (double) tileInfo.tileInfo.zoomIdentifier);
    }

    auto zoom = layerConfig->getZoomFactorAtIdentifier(floor(zoomIdentifier));
    auto scalingFactor = (camera->asCameraInterface()->getScalingFactor() / cameraZoom) * zoom;

    auto polygonDescription = std::static_pointer_cast<PolygonVectorLayerDescription>(description);
    bool inZoomRange = polygonDescription->maxZoom >= zoomIdentifier && polygonDescription->minZoom <= zoomIdentifier;
    
    if (lastZoom &&
        ((isStyleZoomDependant && *lastZoom == zoomIdentifier) || !isStyleZoomDependant)
        && (lastInZoomRange && *lastInZoomRange == inZoomRange) &&
        !isStyleStateDependant) {
        for (const auto &[styleGroupId, polygons] : styleGroupPolygonsMap) {
            for (const auto &polygon: polygons) {
                polygon->setScalingFactor(scalingFactor);
            }
        }
        return;
    }

    lastZoom = zoomIdentifier;
    lastInZoomRange = inZoomRange;

    opacities.clear();
    size_t numStyleGroups = featureGroups.size();
    for (int styleGroupId = 0; styleGroupId < numStyleGroups; styleGroupId++) {
        opacities.push_back(std::vector<float>(featureGroups.at(styleGroupId).size()));
        int index = 0;
        for (const auto &[hash, feature]: featureGroups.at(styleGroupId)) {
            if (inZoomRange) {
                const auto &ec = EvaluationContext(zoomIdentifier, dpFactor, feature, featureStateManager);
                const auto &opacity = polygonDescription->style.getFillOpacity(ec);
                opacities[styleGroupId][index] = alpha * opacity;
            } else {
                opacities[styleGroupId][index] = 0.0;
            }
            index++;
        }
        for (const auto &polygon: styleGroupPolygonsMap.at(styleGroupId)) {
            polygon->setOpacities(opacities[styleGroupId]);
            polygon->setScalingFactor(scalingFactor);
        }
    }
}

void Tiled2dMapVectorPolygonPatternTile::clear() {
    for (const auto &[styleGroupId, polygons] : styleGroupPolygonsMap) {
        for (const auto &polygon: polygons) {
            if (polygon->getPolygonObject()->isReady()) polygon->getPolygonObject()->clear();
        }
    }
}

void Tiled2dMapVectorPolygonPatternTile::setup() {
    auto mapInterface = this->mapInterface.lock();
    if (!mapInterface) {
        return;
    }
    const auto &context = mapInterface->getRenderingContext();
    const auto &spriteTexture = this->spriteTexture;
    for (const auto &[styleGroupId, polygons] : styleGroupPolygonsMap) {
        for (const auto &polygon: polygons) {
            if (!polygon->getPolygonObject()->isReady()) {
                polygon->getPolygonObject()->setup(context);
                if (spriteTexture) {
                    polygon->loadTexture(context, spriteTexture);
                }
            }
        }
    }

    auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
    tileCallbackInterface.message(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady, tileInfo, description->identifier, selfActor);
}

void Tiled2dMapVectorPolygonPatternTile::setVectorTileData(const Tiled2dMapVectorTileDataVector &tileData) {
    auto mapInterface = this->mapInterface.lock();
    const auto &shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
    if (!shaderFactory) {
        return;
    }

    const std::string layerName = description->sourceLayer;
    const auto indicesLimit = std::numeric_limits<uint16_t>::max();

    if (!tileData->empty()) {

        bool anyInteractable = false;

        std::vector<std::vector<ObjectDescriptions>> styleGroupNewPolygonsVector;
        std::unordered_map<int, int32_t> styleIndicesOffsets;

        std::int32_t indices_offset = 0;

        for (auto featureIt = tileData->begin(); featureIt != tileData->end(); featureIt++) {

            const auto [featureContext, geometryHandler] = *featureIt;

            if (featureContext->geomType != vtzero::GeomType::POLYGON) { continue; }

            EvaluationContext evalContext = EvaluationContext(tileInfo.tileInfo.zoomIdentifier, dpFactor, featureContext, featureStateManager);
            if (description->filter == nullptr || description->filter->evaluateOr(evalContext, false)) {

                std::vector<Coord> positions;

                int styleIndex = -1;
                int styleGroupIndex = -1;

                {
                    auto const hash = usedKeys.getHash(evalContext);

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
                            auto shader = shaderFactory->createPolygonPatternGroupShader(fadeInPattern, mapInterface->is3d());
                            auto polygonDescription = std::static_pointer_cast<PolygonVectorLayerDescription>(description);
                            shader->asShaderProgramInterface()->setBlendMode(polygonDescription->style.getBlendMode(EvaluationContext(0.0, dpFactor, std::make_shared<FeatureContext>(), featureStateManager)));
                            shaders.push_back(shader);
                            featureGroups.push_back(std::vector<std::tuple<size_t, std::shared_ptr<FeatureContext>>>{{hash, featureContext}});
                            styleGroupNewPolygonsVector.push_back({{{}, {}}});
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
                        styleGroupNewPolygonsVector[styleGroupIndex].push_back({{}, {}});
                        styleIndicesOffsets[styleGroupIndex] = 0;
                        indexOffset = 0;
                    }

                    for (auto const &index: polygon.indices) {
                        styleGroupNewPolygonsVector[styleGroupIndex].back().indices.push_back(indexOffset + index);
                    }

                    for (auto const &coordinate: polygon.coordinates) {
                        styleGroupNewPolygonsVector[styleGroupIndex].back().vertices.push_back(coordinate.x);
                        styleGroupNewPolygonsVector[styleGroupIndex].back().vertices.push_back(coordinate.y);
                        styleGroupNewPolygonsVector[styleGroupIndex].back().vertices.push_back(styleIndex);
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

        addPolygons(styleGroupNewPolygonsVector);
    } else {
        auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
        tileCallbackInterface.message(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady, tileInfo, description->identifier, selfActor);
    }
}

void Tiled2dMapVectorPolygonPatternTile::addPolygons(const std::vector<std::vector<ObjectDescriptions>> &styleGroupNewPolygonsVector) {

    if (styleGroupNewPolygonsVector.empty()) {
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

    std::unordered_map<int, std::vector<std::shared_ptr<PolygonPatternGroup2dLayerObject>>> polygonGroupObjectsMap;
    std::vector<std::shared_ptr<PolygonPatternGroup2dLayerObject>> polygonsRenderOrder;
    std::vector<std::shared_ptr<GraphicsObjectInterface>> newGraphicObjects;

    for (int styleGroupIndex = 0; styleGroupIndex < styleGroupNewPolygonsVector.size(); styleGroupIndex++) {
        const auto polygonDescs = styleGroupNewPolygonsVector[styleGroupIndex];
        const auto &shader = shaders.at(styleGroupIndex);
        for (const auto &polygonDesc: polygonDescs) {
            const auto polygonObject = objectFactory->createPolygonPatternGroup(shader->asShaderProgramInterface());
#if DEBUG
            polygonObject->asGraphicsObject()->setDebugLabel(description->identifier + "_" + tileInfo.tileInfo.to_string_short());
#endif

            auto layerObject = std::make_shared<PolygonPatternGroup2dLayerObject>(converter, polygonObject, shader);
            layerObject->setVertices(polygonDesc.vertices, polygonDesc.indices);

            polygonGroupObjectsMap[styleGroupIndex].push_back(layerObject);
            polygonsRenderOrder.push_back(layerObject);
            newGraphicObjects.push_back(polygonObject->asGraphicsObject());
        }
    }

    styleGroupPolygonsMap = polygonGroupObjectsMap;
    polygonRenderOrder = polygonsRenderOrder;

#ifdef __APPLE__
    setupPolygons(newGraphicObjects);
#else
    auto selfActor = WeakActor(mailbox, shared_from_this()->weak_from_this());
    selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorPolygonPatternTile::setupPolygons, newGraphicObjects);
#endif
}

void Tiled2dMapVectorPolygonPatternTile::setupPolygons(const std::vector<std::shared_ptr<GraphicsObjectInterface>> &newPolygonObjects) {
    auto mapInterface = this->mapInterface.lock();
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) {
        return;
    }

    setupTextureCoordinates();

    for (auto const &polygon: newPolygonObjects) {
        if (!polygon->isReady()) polygon->setup(renderingContext);
    }

    auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
    tileCallbackInterface.message(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady, tileInfo, description->identifier, selfActor);
}

std::vector<std::shared_ptr<RenderObjectInterface>> Tiled2dMapVectorPolygonPatternTile::generateRenderObjects() {
    std::vector<std::shared_ptr<RenderObjectInterface>> newRenderObjects;
    for (const auto &polygon: polygonRenderOrder) {
        for (const auto &config: polygon->getRenderConfig()) {
            newRenderObjects.push_back(std::make_shared<RenderObject>(config->getGraphicsObject()));
        }
    }

    return newRenderObjects;
}

void Tiled2dMapVectorPolygonPatternTile::setSpriteData(const std::shared_ptr<SpriteData> &spriteData,
                                                         const std::shared_ptr<TextureHolderInterface> &spriteTexture) {
    this->spriteData = spriteData;
    this->spriteTexture = spriteTexture;

    setupTextureCoordinates();
}

void Tiled2dMapVectorPolygonPatternTile::setupTextureCoordinates() {
    if (!spriteData || !spriteTexture) {
        return;
    }

    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto context = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!camera || !context) {
        return;
    }

    double cameraZoom = camera->getZoom();
    double zoomIdentifier = layerConfig->getZoomIdentifier(cameraZoom);
    if (!mapInterface->is3d()) {
        zoomIdentifier = std::max(zoomIdentifier, (double) tileInfo.tileInfo.zoomIdentifier);
    }

    auto polygonDescription = std::static_pointer_cast<PolygonVectorLayerDescription>(description);
    size_t numStyleGroups = featureGroups.size();
    textureCoordinates.clear();
    for (int styleGroupId = 0; styleGroupId < numStyleGroups; styleGroupId++) {
        const auto &styleGroupedFeatureGroups = featureGroups.at(styleGroupId);
        textureCoordinates.push_back(std::vector<float>(styleGroupedFeatureGroups.size() * 5));
        int index = 0;
        for (auto const &[hash, feature]: styleGroupedFeatureGroups) {
            const auto &ec = EvaluationContext(zoomIdentifier, dpFactor, feature, featureStateManager);
            const auto &patternName = polygonDescription->style.getFillPattern(ec);

            const auto spriteIt = spriteData->sprites.find(patternName);
            if (spriteIt != spriteData->sprites.end()) {

                int offset = index * 5;

                textureCoordinates[styleGroupId][offset + 0] = ((float) spriteIt->second.x) / spriteTexture->getImageWidth();
                textureCoordinates[styleGroupId][offset + 1] = ((float) spriteIt->second.y) / spriteTexture->getImageHeight();
                textureCoordinates[styleGroupId][offset + 2] = ((float) spriteIt->second.width) / spriteTexture->getImageWidth();
                textureCoordinates[styleGroupId][offset + 3] = ((float) spriteIt->second.height) / spriteTexture->getImageHeight();
                textureCoordinates[styleGroupId][offset + 4] = spriteIt->second.width + (spriteIt->second.height << 16);

            } else {
                LogError << "Unable to find sprite " <<= patternName;
            }
            index++;
        }
        for (auto const &polygon: styleGroupPolygonsMap.at(styleGroupId)) {
            polygon->setTextureCoordinates(textureCoordinates[styleGroupId]);
            polygon->loadTexture(context, spriteTexture);
        }
    }
}

bool Tiled2dMapVectorPolygonPatternTile::onClickConfirmed(const Vec2F &posScreen) {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!camera) {
        return false;
    }
    auto point = camera->coordFromScreenPosition(posScreen);
    return performClick(point);
}

bool Tiled2dMapVectorPolygonPatternTile::performClick(const Coord &coord) {
    auto mapInterface = this->mapInterface.lock();
    auto converter = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    auto strongSelectionDelegate = selectionDelegate.lock();
    if (!strongSelectionDelegate || !converter) {
        return false;
    }

    std::vector<VectorLayerFeatureInfo> featureInfos;
    for (auto const &[polygon, featureContext]: hitDetectionPolygons) {
        if (VectorTileGeometryHandler::isPointInTriangulatedPolygon(coord, polygon, converter)) {
            if (multiselect) {
                featureInfos.push_back(featureContext->getFeatureInfo());
            } else if (strongSelectionDelegate->didSelectFeature(featureContext->getFeatureInfo(), description->identifier,
                                                                 converter->convert(CoordinateSystemIdentifiers::EPSG4326(),coord))) {
                return true;
            }
        }
    }

    if (multiselect && !featureInfos.empty()) {
        return strongSelectionDelegate->didMultiSelectLayerFeatures(featureInfos, description->identifier, converter->convert(CoordinateSystemIdentifiers::EPSG4326(), coord));
    }

    return false;
}
