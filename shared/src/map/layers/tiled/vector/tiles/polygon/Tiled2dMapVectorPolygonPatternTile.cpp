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
#include "TrigonometryLUT.h"

Tiled2dMapVectorPolygonPatternTile::Tiled2dMapVectorPolygonPatternTile(const std::weak_ptr<MapInterface> &mapInterface,
                                                                       const std::weak_ptr<Tiled2dMapVectorLayer> &vectorLayer,
                                                                       const Tiled2dMapVersionedTileInfo &tileInfo,
                                                                       const WeakActor<Tiled2dMapVectorLayerTileCallbackInterface> &tileCallbackInterface,
                                                                       const std::shared_ptr<PolygonVectorLayerDescription> &description,
                                                                       const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                                                       const std::shared_ptr<SpriteData> &spriteData,
                                                                       const std::shared_ptr<TextureHolderInterface> &spriteTexture,
                                                                       const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager)
        : Tiled2dMapVectorTile(mapInterface, vectorLayer, tileInfo, description, layerConfig, tileCallbackInterface, featureStateManager),
          spriteData(spriteData), spriteTexture(spriteTexture), usedKeys(description->getUsedKeys()), fadeInPattern(description->style.hasFadeInPattern()) {
    isStyleZoomDependant = usedKeys.containsUsedKey(ValueKeys::ZOOM);
    isStyleStateDependant = usedKeys.isStateDependant();
}

void Tiled2dMapVectorPolygonPatternTile::updateVectorLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                                         const Tiled2dMapVectorTileDataVector &tileData) {
    Tiled2dMapVectorTile::updateVectorLayerDescription(description, tileData);
    auto newUsedKeys = description->getUsedKeys();
    bool usedKeysContainsNewUsedKeys = usedKeys.covers(newUsedKeys);
    isStyleZoomDependant = newUsedKeys.containsUsedKey(ValueKeys::ZOOM);
    isStyleStateDependant = newUsedKeys.isStateDependant();
    usedKeys = std::move(newUsedKeys);
    lastZoom = std::nullopt;
    lastAlpha = std::nullopt;

    if (usedKeysContainsNewUsedKeys) {
        auto selfActor = WeakActor(mailbox, shared_from_this()->weak_from_this());
        selfActor.message(MailboxExecutionEnvironment::graphics, MFN(&Tiled2dMapVectorPolygonPatternTile::update));
        
        tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady), tileInfo, description->identifier, WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this()));
    } else {
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

    if (inZoomRange != isVisible) {
        isVisible = inZoomRange;
        for (auto const &object : renderObjects) {
            object->setHidden(!inZoomRange);
        }
    }

    if (!inZoomRange) {
        return;
    }

    if (lastZoom &&
        ((isStyleZoomDependant && *lastZoom == zoomIdentifier) || !isStyleZoomDependant)
        && !isStyleStateDependant) {
        for (const auto &[styleGroupId, polygons] : styleGroupPolygonsMap) {
            for (const auto &polygon: polygons) {
                polygon->setScalingFactor(scalingFactor);
            }
        }
        return;
    }

    lastZoom = zoomIdentifier;

    opacities.clear();
    size_t numStyleGroups = featureGroups.size();
    for (int styleGroupId = 0; styleGroupId < numStyleGroups; styleGroupId++) {
        opacities.push_back(std::vector<HalfFloat>(featureGroups.at(styleGroupId).size()));
        int index = 0;
        for (const auto &[hash, feature]: featureGroups.at(styleGroupId)) {
            const auto &ec = EvaluationContext(zoomIdentifier, dpFactor, feature, featureStateManager);
            const auto &opacity = polygonDescription->style.getFillOpacity(ec);
            opacities[styleGroupId][index] = toHalfFloat(alpha * opacity);
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
    tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady), tileInfo, description->identifier, selfActor);
}

void Tiled2dMapVectorPolygonPatternTile::setVectorTileData(const Tiled2dMapVectorTileDataVector &tileData) {
    auto mapInterface = this->mapInterface.lock();
    const auto &shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
    if (!shaderFactory) {
        return;
    }
    bool is3d = mapInterface->is3d();

    const std::string layerName = description->sourceLayer;
    const auto indicesLimit = std::numeric_limits<uint16_t>::max();

    if (!tileData->empty()) {

        auto convertedTileBounds = mapInterface->getCoordinateConverterHelper()->convertRectToRenderSystem(tileInfo.tileInfo.bounds);
        double cx = (convertedTileBounds.bottomRight.x + convertedTileBounds.topLeft.x) / 2.0;
        double cy = (convertedTileBounds.bottomRight.y + convertedTileBounds.topLeft.y) / 2.0;
        double rx = is3d ? 1.0 * sin(cy) * cos(cx) : cx;
        double ry = is3d ? 1.0 * cos(cy) : cy;
        double rz = is3d ? -1.0 * sin(cy) * sin(cx) : 0.0;
        auto origin = Vec3D(rx, ry, rz);

        bool anyInteractable = false;

        std::vector<std::vector<ObjectDescriptions>> styleGroupNewPolygonsVector;
        std::unordered_map<int, int32_t> styleIndicesOffsets;

        std::unordered_map<size_t, bool> filterCache;
        
        for (auto featureIt = tileData->begin(); featureIt != tileData->end(); featureIt++) {

            const auto [featureContext, geometryHandler] = *featureIt;

            if (featureContext->geomType != vtzero::GeomType::POLYGON) { continue; }

            EvaluationContext evalContext = EvaluationContext(tileInfo.tileInfo.zoomIdentifier, dpFactor, featureContext, featureStateManager);

            bool inside = true;
            if (description->filter) {
                if (featureContext->hasCustomId) {
                    // Every ID is unique → no cache possible
                    inside = description->filter->evaluateOr(evalContext, false);
                } else {
                    auto hash = featureContext->identifier;
                    auto it = filterCache.find(hash);
                    if (it != filterCache.end()) {
                        inside = it->second;
                    } else {
                        inside = description->filter->evaluateOr(evalContext, false);
                        filterCache[hash] = inside;
                    }
                }
            }

            if (inside) {

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


                    double x,y,z;

                    for (auto const &coordinate: polygon.coordinates) {
                        if(is3d) {
                            double sinX, cosX, sinY, cosY;
                            lut::sincos(coordinate.y, sinY, cosY);
                            lut::sincos(coordinate.x, sinX, cosX);

                            x = 1.0 * sinY * cosX - rx;
                            y = 1.0 * cosY - ry;
                            z = -1.0 * sinY * sinX - rz;
                        } else {
                            x = coordinate.x - rx;
                            y = coordinate.y - ry;
                            z = 0.0;
                        }

                        styleGroupNewPolygonsVector[styleGroupIndex].back().vertices.push_back(x);
                        styleGroupNewPolygonsVector[styleGroupIndex].back().vertices.push_back(y);
                        styleGroupNewPolygonsVector[styleGroupIndex].back().vertices.push_back(z);
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
            tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsInteractable), description->identifier);
        }

        addPolygons(styleGroupNewPolygonsVector, origin);
    } else {
        auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
        tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady), tileInfo, description->identifier, selfActor);
    }
}

void Tiled2dMapVectorPolygonPatternTile::addPolygons(const std::vector<std::vector<ObjectDescriptions>> &styleGroupNewPolygonsVector, const Vec3D & origin) {

    if (styleGroupNewPolygonsVector.empty()) {
        auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
        tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady), tileInfo, description->identifier, selfActor);
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
            layerObject->setVertices(polygonDesc.vertices, polygonDesc.indices, origin);

            polygonGroupObjectsMap[styleGroupIndex].push_back(layerObject);
            polygonsRenderOrder.push_back(layerObject);
            newGraphicObjects.push_back(polygonObject->asGraphicsObject());
        }
    }

    styleGroupPolygonsMap = polygonGroupObjectsMap;
    polygonRenderOrder = polygonsRenderOrder;

    std::vector<std::shared_ptr<RenderObjectInterface>> newRenderObjects;
    for (const auto &polygon: polygonRenderOrder) {
        for (const auto &config: polygon->getRenderConfig()) {
            newRenderObjects.push_back(std::make_shared<RenderObject>(config->getGraphicsObject()));
        }
    }
    renderObjects = newRenderObjects;

#ifdef __APPLE__
    setupPolygons(newGraphicObjects);
#else
    auto selfActor = WeakActor(mailbox, shared_from_this()->weak_from_this());
    selfActor.message(MailboxExecutionEnvironment::graphics, MFN(&Tiled2dMapVectorPolygonPatternTile::setupPolygons), newGraphicObjects);
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
    tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady), tileInfo, description->identifier, selfActor);
}

std::vector<std::shared_ptr<RenderObjectInterface>> Tiled2dMapVectorPolygonPatternTile::generateRenderObjects() {
    return renderObjects;
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
    auto strongVectorLayer = vectorLayer.lock();
    if (!strongSelectionDelegate || !converter || !strongVectorLayer) {
        return false;
    }
    const StringInterner& stringTable = strongVectorLayer->getStringInterner();

    std::vector<VectorLayerFeatureInfo> featureInfos;
    for (auto const &[polygon, featureContext]: hitDetectionPolygons) {
        if (VectorTileGeometryHandler::isPointInTriangulatedPolygon(coord, polygon, converter)) {
            if (multiselect) {
                featureInfos.push_back(featureContext->getFeatureInfo(stringTable));
            } else if (strongSelectionDelegate->didSelectFeature(featureContext->getFeatureInfo(stringTable), description->identifier,
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
