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
#include "MapCameraInterface.h"
#include "earcut.hpp"
#include "Logger.h"
#include "PolygonHelper.h"
#include "CoordinateSystemIdentifiers.h"
#include "Tiled2dMapVectorStyleParser.h"
#include "Tiled2dMapVectorLayerConstants.h"

Tiled2dMapVectorPolygonTile::Tiled2dMapVectorPolygonTile(const std::weak_ptr<MapInterface> &mapInterface,
                                                         const Tiled2dMapVersionedTileInfo &tileInfo,
                                                         const WeakActor<Tiled2dMapVectorLayerTileCallbackInterface> &tileCallbackInterface,
                                                         const std::shared_ptr<PolygonVectorLayerDescription> &description,
                                                         const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                                         const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager)
        : Tiled2dMapVectorTile(mapInterface, tileInfo, description, layerConfig, tileCallbackInterface, featureStateManager),
          usedKeys(std::move(description->getUsedKeys())), isStriped(description->style.isStripedPotentially()) {
    isStyleZoomDependant = usedKeys.containsUsedKey(Tiled2dMapVectorStyleParser::zoomExpression);
    isStyleStateDependant = usedKeys.isStateDependant();
}

void Tiled2dMapVectorPolygonTile::updateVectorLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                                         const Tiled2dMapVectorTileDataVector &tileData) {
    Tiled2dMapVectorTile::updateVectorLayerDescription(description, tileData);
    auto newUsedKeys = description->getUsedKeys();
    bool usedKeysContainsNewUsedKeys = usedKeys.covers(newUsedKeys);
    isStyleZoomDependant = newUsedKeys.containsUsedKey(Tiled2dMapVectorStyleParser::zoomExpression);
    isStyleStateDependant = newUsedKeys.isStateDependant();
    usedKeys = std::move(newUsedKeys);
    lastZoom = std::nullopt;
    lastAlpha = std::nullopt;

    if (usedKeysContainsNewUsedKeys) {
        auto selfActor = WeakActor(mailbox, shared_from_this()->weak_from_this());
        selfActor.message(MailboxExecutionEnvironment::graphics, MFN(&Tiled2dMapVectorPolygonTile::update));

        tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady), tileInfo, description->identifier, WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this()));
    } else {
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
    if (!mapInterface->is3d()) {
        zoomIdentifier = std::max(zoomIdentifier, (double) tileInfo.tileInfo.zoomIdentifier);
    }

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
        ((isStyleZoomDependant && *lastZoom == zoomIdentifier) || !isStyleZoomDependant) &&
        lastAlpha == alpha &&
        !isStyleStateDependant) {
        return;
    }
    lastZoom = zoomIdentifier;
    lastAlpha = alpha;

    size_t numStyleGroups = featureGroups.size();
    for (int styleGroupId = 0; styleGroupId < numStyleGroups; styleGroupId++) {
        std::vector<float> shaderStyles;
        shaderStyles.reserve(featureGroups.at(styleGroupId).size() * (isStriped ? 7 : 5));
        for (auto const &[hash, feature]: featureGroups.at(styleGroupId)) {
            const auto& ec = EvaluationContext(zoomIdentifier, dpFactor, feature, featureStateManager);
            const auto& color = polygonDescription->style.getFillColor(ec);
            const auto& opacity = polygonDescription->style.getFillOpacity(ec);
            shaderStyles.push_back(color.r);
            shaderStyles.push_back(color.g);
            shaderStyles.push_back(color.b);
            shaderStyles.push_back(color.a);
            shaderStyles.push_back(opacity * alpha);
            if (isStriped) {
                const auto stripeWidth = polygonDescription->style.getStripeWidth(ec);
                shaderStyles.push_back(stripeWidth[0]);
                shaderStyles.push_back(stripeWidth[1]);
#ifndef __APPLE__
                // Padding to have the style size as a multiple of 16 bytes (OpenGL uniform buffer padding due to std140)
                shaderStyles.push_back(0.0);
#endif
            } else {
#ifndef __APPLE__
                // Padding to have the style size as a multiple of 16 bytes (OpenGL uniform buffer padding due to std140)
                shaderStyles.push_back(0.0);
                shaderStyles.push_back(0.0);
                shaderStyles.push_back(0.0);
#endif
            }
        }

#ifndef __APPLE__
        int32_t numAttributesPerStyle = 8;
#else
        int32_t numAttributesPerStyle = isStriped ? 7 : 5;
#endif
        auto s = SharedBytes((int64_t)shaderStyles.data(), (int32_t)featureGroups.at(styleGroupId).size(), numAttributesPerStyle * (int32_t)sizeof(float));
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
    tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady), tileInfo, description->identifier, selfActor);

}

void Tiled2dMapVectorPolygonTile::setVectorTileData(const Tiled2dMapVectorTileDataVector &tileData) {
    auto mapInterface = this->mapInterface.lock();
    const auto &shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
    if (!shaderFactory) {
        return;
    }

    bool is3d = mapInterface->is3d();

    const std::string layerName = description->sourceLayer;

    const auto indicesLimit = is3d ? std::numeric_limits<uint16_t>::max() / 2 : std::numeric_limits<uint16_t>::max();

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

        for (auto featureIt = tileData->begin(); featureIt != tileData->end(); featureIt++) {

            const auto [featureContext, geometryHandler] = *featureIt;

            if (featureContext->geomType != vtzero::GeomType::POLYGON) { continue; }

            EvaluationContext evalContext = EvaluationContext(tileInfo.tileInfo.zoomIdentifier, dpFactor, featureContext, featureStateManager);
            if (description->filter == nullptr || description->filter->evaluateOr(evalContext, false)) {

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
                            featureGroups[styleGroupIndex].emplace_back(hash, featureContext);
                        } else {
                            styleGroupIndex = (int) featureGroups.size();
                            styleIndex = 0;
                            auto shader = shaderFactory->createPolygonGroupShader(isStriped, mapInterface->is3d());
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

                    auto coordinates = polygon.coordinates;
                    auto indices = polygon.indices;

                    if (is3d) {
                        auto maxSegmentLength = std::min(std::abs(convertedTileBounds.bottomRight.x - convertedTileBounds.topLeft.x) / POLYGON_SUBDIVISION_FACTOR, (M_PI * 2.0) / POLYGON_SUBDIVISION_FACTOR);
                        PolygonHelper::subdivision(coordinates, indices, maxSegmentLength);
                    }

                    for (auto const &index: indices) {
                        styleGroupNewPolygonsVector[styleGroupIndex].back().indices.push_back(indexOffset + index);
                    }

                    for (auto const &coordinate: coordinates) {
                        double x = is3d ? 1.0 * sin(coordinate.y) * cos(coordinate.x) - rx : coordinate.x - rx;
                        double y = is3d ?  1.0 * cos(coordinate.y) - ry : coordinate.y - ry;
                        double z = is3d ? -1.0 * sin(coordinate.y) * sin(coordinate.x) - rz : 0.0;
                        styleGroupNewPolygonsVector[styleGroupIndex].back().vertices.push_back(x);
                        styleGroupNewPolygonsVector[styleGroupIndex].back().vertices.push_back(y);
                        styleGroupNewPolygonsVector[styleGroupIndex].back().vertices.push_back(z);
                        styleGroupNewPolygonsVector[styleGroupIndex].back().vertices.push_back(styleIndex);
                    }

                    styleIndicesOffsets[styleGroupIndex] += coordinates.size();

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

void Tiled2dMapVectorPolygonTile::addPolygons(const std::vector<std::vector<ObjectDescriptions>> &styleGroupNewPolygonsVector,
                                              const Vec3D & origin) {

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

    std::vector<std::shared_ptr<PolygonGroup2dLayerObject>> polygonGroupObjects;
    std::vector<std::shared_ptr<GraphicsObjectInterface>> newGraphicObjects;

    for (int styleGroupIndex = 0; styleGroupIndex < styleGroupNewPolygonsVector.size(); styleGroupIndex++) {
        const auto &shader = shaders.at(styleGroupIndex);
        for (const auto &polygonDesc: styleGroupNewPolygonsVector[styleGroupIndex]) {
            const auto polygonObject = objectFactory->createPolygonGroup(shader->asShaderProgramInterface());
#if DEBUG
            polygonObject->asGraphicsObject()->setDebugLabel(description->identifier + "_" + tileInfo.tileInfo.to_string_short());
#endif

            auto layerObject = std::make_shared<PolygonGroup2dLayerObject>(converter, polygonObject, shader);
            layerObject->setVertices(polygonDesc.vertices, polygonDesc.indices, origin);

            polygonGroupObjects.emplace_back(layerObject);
            newGraphicObjects.push_back(polygonObject->asGraphicsObject());
        }
    }

    polygons = polygonGroupObjects;

    std::vector<std::shared_ptr<RenderObjectInterface>> newRenderObjects;
    for (auto const &object : polygons) {
        for (const auto &config : object->getRenderConfig()) {
            newRenderObjects.push_back(std::make_shared<RenderObject>(config->getGraphicsObject()));
        }
    }
    renderObjects = newRenderObjects;

#ifdef __APPLE__
    setupPolygons(newGraphicObjects);
#else
    auto selfActor = WeakActor(mailbox, shared_from_this()->weak_from_this());
    selfActor.message(MailboxExecutionEnvironment::graphics, MFN(&Tiled2dMapVectorPolygonTile::setupPolygons), newGraphicObjects);
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
    tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady), tileInfo, description->identifier, selfActor);
}

std::vector<std::shared_ptr<RenderObjectInterface>> Tiled2dMapVectorPolygonTile::generateRenderObjects() {
    return renderObjects;
}

bool Tiled2dMapVectorPolygonTile::onClickConfirmed(const Vec2F &posScreen) {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!camera) {
        return false;
    }
    auto point = camera->coordFromScreenPosition(posScreen);
    return performClick(point);
}

bool Tiled2dMapVectorPolygonTile::performClick(const Coord &coord) {
    auto mapInterface = this->mapInterface.lock();
    auto converter = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    auto strongSelectionDelegate = selectionDelegate.lock();
    if (!strongSelectionDelegate || !converter) {
        return false;
    }

    std::vector<VectorLayerFeatureInfo> featureInfos;
    for (auto iter = hitDetectionPolygons.rbegin(); iter != hitDetectionPolygons.rend(); ++iter) {
        if (VectorTileGeometryHandler::isPointInTriangulatedPolygon(coord, std::get<0>(*iter), converter)) {
            if (multiselect) {
                featureInfos.push_back(std::get<1>(*iter)->getFeatureInfo());
            } else if (strongSelectionDelegate->didSelectFeature(std::get<1>(*iter)->getFeatureInfo(), description->identifier,
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
