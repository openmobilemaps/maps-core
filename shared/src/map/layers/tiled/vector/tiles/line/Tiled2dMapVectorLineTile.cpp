/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorLineTile.h"
#include "MapCameraInterface.h"
#include "RenderObject.h"
#include "LineHelper.h"
#include "Tiled2dMapVectorLayerConfig.h"
#include "Tiled2dMapVectorStyleParser.h"

Tiled2dMapVectorLineTile::Tiled2dMapVectorLineTile(const std::weak_ptr<MapInterface> &mapInterface,
                                                         const Tiled2dMapVersionedTileInfo &tileInfo,
                                                         const WeakActor<Tiled2dMapVectorLayerTileCallbackInterface> &tileCallbackInterface,
                                                         const std::shared_ptr<LineVectorLayerDescription> &description,
                                                   const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                                   const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager)
        : Tiled2dMapVectorTile(mapInterface, tileInfo, description, layerConfig, tileCallbackInterface, featureStateManager),
          usedKeys(description->getUsedKeys()), selectionSizeFactor(description->selectionSizeFactor) {
    isStyleZoomDependant = usedKeys.usedKeys.contains(Tiled2dMapVectorStyleParser::zoomExpression);
    isStyleStateDependant = usedKeys.isStateDependant();

    isSimpleLine = description->style.isSimpleLine();
}

void Tiled2dMapVectorLineTile::updateVectorLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
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
        selfActor.message(MailboxExecutionEnvironment::graphics, MFN(&Tiled2dMapVectorLineTile::update));
        
        tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady), tileInfo, description->identifier, WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this()));
    } else {
        reusableLineStyles.clear();
        featureGroups.clear();
        styleHashToGroupMap.clear();
        hitDetection.clear();
        toClear.insert(toClear.begin(), lines.begin(), lines.end());
        lines.clear();
        shaders.clear();
        setVectorTileData(tileData);
    }
}

void Tiled2dMapVectorLineTile::update() {
    if (shaders.empty()) {
        return;
    }

    if (!toClear.empty()) {
        for (auto const &line: toClear) {
            if (line->getLineObject()->isReady()) line->getLineObject()->clear();
        }
        toClear.clear();
    }
    
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!mapInterface || !camera) {
        return;
    }

    const double cameraZoom = camera->getZoom();
    double zoomIdentifier = layerConfig->getZoomIdentifier(cameraZoom);

    if (!mapInterface->is3d()) {
        zoomIdentifier = std::max(zoomIdentifier, (double) tileInfo.tileInfo.zoomIdentifier);
    }


    auto zoom = layerConfig->getZoomFactorAtIdentifier(floor(zoomIdentifier));
    auto scalingFactor = (camera->asCameraInterface()->getScalingFactor() / cameraZoom) * zoom;
    
    auto lineDescription = std::static_pointer_cast<LineVectorLayerDescription>(description);
    bool inZoomRange = lineDescription->maxZoom >= zoomIdentifier && lineDescription->minZoom <= zoomIdentifier;

    if (inZoomRange != isVisible) {
        isVisible = inZoomRange;
        for (auto const &object : renderObjects) {
            object->setHidden(!inZoomRange);
        }
    }

    if (!inZoomRange) {
        return;
    }

    for (auto const &line: lines) {
        line->setScalingFactor(scalingFactor);
    }

    if (lastAlpha == alpha &&
        lastZoom &&
        ((isStyleZoomDependant && *lastZoom == zoomIdentifier) || !isStyleZoomDependant) && !isStyleStateDependant) {
        return;
    }

    lastZoom = zoomIdentifier;
    lastAlpha = alpha;

    size_t numStyleGroups = featureGroups.size();
    for (int styleGroupId = 0; styleGroupId < numStyleGroups; styleGroupId++) {
        int i = 0;
        bool needsUpdate = false;
        for (auto const &[key, feature]: featureGroups[styleGroupId]) {
            auto const &context = EvaluationContext(zoomIdentifier, dpFactor, feature, featureStateManager);
            if (isSimpleLine) {
                auto &style = reusableSimpleLineStyles[styleGroupId][i];

                // color
                auto color = inZoomRange ? lineDescription->style.getLineColor(context) : Color(0.0, 0.0, 0.0, 0.0);
                if (color.r != style.colorR || color.g != style.colorG || color.b != style.colorB || color.a != style.colorA) {
                    style.colorR = toHalfFloat(color.r);
                    style.colorG = toHalfFloat(color.g);
                    style.colorB = toHalfFloat(color.b);
                    style.colorA = toHalfFloat(color.a);
                    needsUpdate = true;
                }

                // opacity
                float opacity = inZoomRange ? lineDescription->style.getLineOpacity(context) * alpha : 0.0;
                if (opacity != style.opacity) {
                    style.opacity = toHalfFloat(opacity);
                    needsUpdate = true;
                }

                // width type
                auto widthType = SizeType::SCREEN_PIXEL;
                auto widthAsPixel = (widthType == SizeType::SCREEN_PIXEL ? 1 : 0);
                if (widthAsPixel != style.widthAsPixel) {
                    style.widthAsPixel = toHalfFloat(widthAsPixel);
                    needsUpdate = true;
                }

                // width
                float width = inZoomRange ? lineDescription->style.getLineWidth(context) : 0.0;
                if (width != style.width) {
                    style.width = toHalfFloat(width);
                    needsUpdate = true;
                }

                // line caps
                auto lineCap = lineDescription->style.getLineCap(context);

                auto cap = 1;
                switch(lineCap){
                    case LineCapType::BUTT: { cap = 0; break; }
                    case LineCapType::ROUND: { cap = 1; break; }
                    case LineCapType::SQUARE: { cap = 2; break; }
                    default: { cap = 1; }
                }

                if (cap != style.lineCap) {
                    style.lineCap = toHalfFloat(cap);
                    needsUpdate = true;
                }
            } else {
                auto &style = reusableLineStyles[styleGroupId][i];

                // color
                auto color = inZoomRange ? lineDescription->style.getLineColor(context) : Color(0.0, 0.0, 0.0, 0.0);
                if (color.r != style.colorR || color.g != style.colorG || color.b != style.colorB || color.a != style.colorA) {
                    style.colorR = toHalfFloat(color.r);
                    style.colorG = toHalfFloat(color.g);
                    style.colorB = toHalfFloat(color.b);
                    style.colorA = toHalfFloat(color.a);
                    needsUpdate = true;
                }

                // opacity
                float opacity = inZoomRange ? lineDescription->style.getLineOpacity(context) * alpha : 0.0;
                if (opacity != style.opacity) {
                    style.opacity = toHalfFloat(opacity);
                    needsUpdate = true;
                }

                // blue
                float blur = inZoomRange ? lineDescription->style.getLineBlur(context) : 0.0;
                if (blur != style.blur) {
                    style.blur = toHalfFloat(blur);
                    needsUpdate = true;
                }

                // width type
                auto widthType = SizeType::SCREEN_PIXEL;
                auto widthAsPixel = (widthType == SizeType::SCREEN_PIXEL ? 1 : 0);
                if (widthAsPixel != style.widthAsPixel) {
                    style.widthAsPixel = toHalfFloat(widthAsPixel);
                    needsUpdate = true;
                }

                // width
                float width = inZoomRange ? lineDescription->style.getLineWidth(context) : 0.0;
                if (width != style.width) {
                    style.width = toHalfFloat(width);
                    needsUpdate = true;
                }

                // dashes
                auto dashArray = inZoomRange ? lineDescription->style.getLineDashArray(context) : std::vector<float>{};
                auto dn = dashArray.size();
                auto dValue0 = dn > 0 ? dashArray[0] : 0.0;
                auto dValue1 = (dn > 1 ? dashArray[1] : 0.0) + dValue0;
                auto dValue2 = (dn > 2 ? dashArray[2] : 0.0) + dValue1;
                auto dValue3 = (dn > 3 ? dashArray[3] : 0.0) + dValue2;

                if (style.numDashValue != dn || dValue0 != style.dashValue0 || dValue1 != style.dashValue1 || dValue2 != style.dashValue2 || dValue3 != style.dashValue3) {
                    style.numDashValue = dn;
                    style.dashValue0 = toHalfFloat(dValue0);
                    style.dashValue1 = toHalfFloat(dValue1);
                    style.dashValue2 = toHalfFloat(dValue2);
                    style.dashValue3 = toHalfFloat(dValue3);

                    needsUpdate = true;
                }

                // line caps
                auto lineCap = lineDescription->style.getLineCap(context);

                auto cap = 1;
                switch(lineCap){
                    case LineCapType::BUTT: { cap = 0; break; }
                    case LineCapType::ROUND: { cap = 1; break; }
                    case LineCapType::SQUARE: { cap = 2; break; }
                    default: { cap = 1; }
                }

                if (cap != style.lineCap) {
                    style.lineCap = toHalfFloat(cap);
                    needsUpdate = true;
                }

                // offset
                auto offset = inZoomRange ? lineDescription->style.getLineOffset(context, width) : 0.0;
                if(offset != style.offset) {
                    style.offset = toHalfFloat(offset);
                    needsUpdate = true;
                }

                // dotted
                bool lineDotted = lineDescription->style.getLineDotted(context);

                auto dotted = lineDotted ? 1 : 0;
                if(dotted != style.dotted) {
                    style.dotted = toHalfFloat(dotted);
                    needsUpdate = true;
                }

                // dotted skew
                auto dottedSkew = lineDescription->style.getLineDottedSkew(context);
                if(dottedSkew != style.dottedSkew) {
                    style.dottedSkew = toHalfFloat(dottedSkew);
                    needsUpdate = true;
                }
            }
            
            i++;
        }

        if (needsUpdate) {
            if (isSimpleLine) {
                auto &styles = reusableSimpleLineStyles[styleGroupId];
                auto buffer = SharedBytes((int64_t)styles.data(), (int)styles.size(), sizeof(ShaderSimpleLineStyle));
                shaders[styleGroupId]->setStyles(buffer);
            } else {
                auto &styles = reusableLineStyles[styleGroupId];
                auto buffer = SharedBytes((int64_t)styles.data(), (int)styles.size(), sizeof(ShaderLineStyle));
                shaders[styleGroupId]->setStyles(buffer);
            }
        }
    }
}

void Tiled2dMapVectorLineTile::clear() {
    for (auto const &line: lines) {
        if (line->getLineObject()->isReady()) line->getLineObject()->clear();
    }
}

void Tiled2dMapVectorLineTile::setup() {
    auto mapInterface = this->mapInterface.lock();
    if (!mapInterface) {
        return;
    }
    const auto &context = mapInterface->getRenderingContext();
    for (auto const &line: lines) {
        auto const &lineObject = line->getLineObject();
        if (!lineObject->isReady()) lineObject->setup(context);
    }
    auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
    tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady), tileInfo, description->identifier, selfActor);
}

void Tiled2dMapVectorLineTile::setVectorTileData(const Tiled2dMapVectorTileDataVector &tileData) {

    auto mapInterface = this->mapInterface.lock();
    const auto &shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
    if (!shaderFactory) {
        return;
    }

    bool is3d = mapInterface->is3d();

    if (!tileData->empty()) {
        std::unordered_map<int, int> subGroupCoordCount;
        std::vector<std::vector<std::vector<std::tuple<std::vector<Vec2D>, int>>>> styleGroupNewLinesVector;
        std::vector<std::vector<std::tuple<std::vector<Vec2D>, int>>> styleGroupLineSubGroupVector;

        bool anyInteractable = false;

        auto lineDescription = std::dynamic_pointer_cast<LineVectorLayerDescription>(description);

        for (auto featureIt = tileData->rbegin(); featureIt != tileData->rend(); ++featureIt) {
            std::shared_ptr<FeatureContext> featureContext = std::get<0>(*featureIt);
            
            if (featureContext->geomType != vtzero::GeomType::POLYGON && featureContext->geomType != vtzero::GeomType::LINESTRING) { continue; }

            EvaluationContext evalContext = EvaluationContext(tileInfo.tileInfo.zoomIdentifier, dpFactor, featureContext, featureStateManager);
            if ((description->filter == nullptr || description->filter->evaluateOr(evalContext, false))) {
                int styleGroupIndex = -1;
                int styleIndex = -1;
                {
                    auto const hash = usedKeys.getHash(evalContext);

                    auto indexPair = styleHashToGroupMap.find(hash);
                    if (indexPair != styleHashToGroupMap.end()) {
                        styleGroupIndex = indexPair->second.first;
                        styleIndex = indexPair->second.second;
                    }

                    if (styleIndex == -1) {

                        auto lineCapValue = lineDescription->style.lineCapEvaluator.getValue();
                        LineCapType capType = LineCapType::BUTT;
                        if (lineCapValue) {
                            capType = lineCapValue->evaluateOr(evalContext, capType);
                        }

                        auto lineJoinValue = lineDescription->style.lineJoinEvaluator.getValue();
                        LineJoinType joinType = LineJoinType::MITER;
                        if (lineJoinValue) {
                            joinType = lineJoinValue->evaluateOr(evalContext, joinType);
                        }

                        if (!featureGroups.empty() && featureGroups.back().size() < maxStylesPerGroup) {
                            styleGroupIndex = (int) featureGroups.size() - 1;
                            styleIndex = (int) featureGroups.back().size();
                            featureGroups.at(styleGroupIndex).push_back({ hash, featureContext });
                            if (isSimpleLine) {
                                reusableSimpleLineStyles.at(styleGroupIndex).push_back( ShaderSimpleLineStyle {0});
                            } else {
                                reusableLineStyles.at(styleGroupIndex).push_back(ShaderLineStyle {0});
                            }
                        } else {
                            styleGroupIndex = (int) featureGroups.size();
                            styleIndex = 0;
                            auto shader = isSimpleLine ? (is3d ? shaderFactory->createUnitSphereSimpleLineGroupShader() : shaderFactory->createSimpleLineGroupShader()) : (is3d ? shaderFactory->createUnitSphereLineGroupShader() : shaderFactory->createLineGroupShader());
                            auto lineDescription = std::static_pointer_cast<LineVectorLayerDescription>(description);
                            shader->asShaderProgramInterface()->setBlendMode(lineDescription->style.getBlendMode(EvaluationContext(0.0, dpFactor, std::make_shared<FeatureContext>(), featureStateManager)));
                            shaders.push_back(shader);
                            capTypes.push_back(capType);
                            joinTypes.push_back(joinType);
                            if (isSimpleLine) {
                                reusableSimpleLineStyles.push_back({  ShaderSimpleLineStyle {0} });
                            } else {
                                reusableLineStyles.push_back({ ShaderLineStyle {0} });
                            }
                            styleGroupLineSubGroupVector.push_back(std::vector<std::tuple<std::vector<Vec2D>, int>>());
                            styleGroupNewLinesVector.push_back({});
                            featureGroups.push_back(std::vector<std::tuple<size_t, std::shared_ptr<FeatureContext>>>{{hash, featureContext}});
                        }
                        styleHashToGroupMap.insert({hash, {styleGroupIndex, styleIndex}});
                    }
                }

                if (subGroupCoordCount.count(styleGroupIndex) == 0) {
                    subGroupCoordCount.try_emplace(styleGroupIndex, 0);
                }

                const std::shared_ptr<VectorTileGeometryHandler> geometryHandler = std::get<1>(*featureIt);
                bool isInteractable = description->isInteractable(evalContext);
                std::vector<std::vector<::Vec2D>> lineCoordinatesVector;

                for (const auto &lineCoordinates: geometryHandler->getLineCoordinates()) {
                    if (lineCoordinates.empty()) { continue; }

                    auto maxSegmentLength = std::abs(tileInfo.tileInfo.bounds.bottomRight.x - tileInfo.tileInfo.bounds.topLeft.x) / 2.0;

                    const auto &coordinates = is3d ? LineHelper::subdividePolyline(lineCoordinates, maxSegmentLength) : lineCoordinates;

                    int numCoords = (int)coordinates.size();
                    int coordCount = subGroupCoordCount[styleGroupIndex];
                    if (coordCount + numCoords > maxNumLinePoints
                        && !styleGroupLineSubGroupVector[styleGroupIndex].empty()) {
                        styleGroupNewLinesVector[styleGroupIndex].push_back(styleGroupLineSubGroupVector[styleGroupIndex]);
                        styleGroupLineSubGroupVector.push_back(std::vector<std::tuple<std::vector<Vec2D>, int>>());
                        subGroupCoordCount[styleGroupIndex] = 0;
                    }

                    styleGroupLineSubGroupVector[styleGroupIndex].push_back({coordinates, std::min(maxStylesPerGroup - 1, styleIndex)});
                    subGroupCoordCount[styleGroupIndex] = (int)subGroupCoordCount[styleGroupIndex] + numCoords;
                    if (isInteractable) {
                        lineCoordinatesVector.push_back(coordinates);
                    }
                }

                if (isInteractable) {
                    anyInteractable = true;
                    hitDetection.push_back({lineCoordinatesVector, featureContext});
                }
            }
        }

        for (int styleGroupIndex = 0; styleGroupIndex < styleGroupLineSubGroupVector.size(); styleGroupIndex++) {
            const auto &lineSubGroup = styleGroupLineSubGroupVector[styleGroupIndex];
            if (!lineSubGroup.empty() && subGroupCoordCount[styleGroupIndex] > 0) {
                styleGroupNewLinesVector[styleGroupIndex].push_back(lineSubGroup);
            }
        }

        if (anyInteractable) {
            tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsInteractable), description->identifier);
        }

        addLines(styleGroupNewLinesVector);
    } else {
        auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
        tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady), tileInfo, description->identifier, selfActor);
    }
}


void Tiled2dMapVectorLineTile::addLines(const std::vector<std::vector<std::vector<std::tuple<std::vector<Vec2D>, int>>>> &styleIdLinesVector) {
    if (styleIdLinesVector.empty()) {
        auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
        tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady), tileInfo, description->identifier, selfActor);
        return;
    }

    auto mapInterface = this->mapInterface.lock();
    const auto &objectFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
    const auto &coordinateConverterHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    if (!objectFactory || !coordinateConverterHelper) {
        return;
    }

    std::vector<std::shared_ptr<LineGroup2dLayerObject>> lineGroupObjects;
    std::vector<std::shared_ptr<GraphicsObjectInterface>> newGraphicObjects;

    bool is3d = mapInterface->is3d();
    auto convertedTileBounds = mapInterface->getCoordinateConverterHelper()->convertRectToRenderSystem(tileInfo.tileInfo.bounds);
    double cx = (convertedTileBounds.bottomRight.x + convertedTileBounds.topLeft.x) / 2.0;
    double cy = (convertedTileBounds.bottomRight.y + convertedTileBounds.topLeft.y) / 2.0;
    double rx = is3d ? 1.0 * sin(cy) * cos(cx) : cx;
    double ry = is3d ? 1.0 * cos(cy) : cy;
    double rz = is3d ? -1.0 * sin(cy) * sin(cx) : 0.0;
    auto origin = Vec3D(rx, ry, rz);


    for (int styleGroupIndex = 0; styleGroupIndex < styleIdLinesVector.size(); styleGroupIndex++) {
        for (const auto &lineSubGroup: styleIdLinesVector[styleGroupIndex]) {
            const auto &shader = shaders.at(styleGroupIndex);
            const auto capType = capTypes.at(styleGroupIndex);
            const auto joinType = joinTypes.at(styleGroupIndex);
            auto lineGroupGraphicsObject = objectFactory->createLineGroup(shader->asShaderProgramInterface());

#if DEBUG
            lineGroupGraphicsObject->asGraphicsObject()->setDebugLabel(description->identifier + "_" + tileInfo.tileInfo.to_string_short());
#endif
            auto lineGroupObject = std::make_shared<LineGroup2dLayerObject>(coordinateConverterHelper,
                                                                            lineGroupGraphicsObject,
                                                                            shader,
                                                                            is3d);

            lineGroupObject->setLines(lineSubGroup, tileInfo.tileInfo.bounds.topLeft.systemIdentifier, origin, capType, joinType);

            lineGroupObjects.push_back(lineGroupObject);
            newGraphicObjects.push_back(lineGroupGraphicsObject->asGraphicsObject());
        }
    }

    lines = lineGroupObjects;

    std::vector<std::shared_ptr<RenderObjectInterface>> newRenderObjects;
    for (auto const &object : lines) {
        for (const auto &config : object->getRenderConfig()) {
            newRenderObjects.push_back(std::make_shared<RenderObject>(config->getGraphicsObject()));
        }
    }
    renderObjects = newRenderObjects;

#ifdef __APPLE__
    setupLines(newGraphicObjects);
#else
    auto selfActor = WeakActor(mailbox, weak_from_this());
    selfActor.message(MailboxExecutionEnvironment::graphics, MFN(&Tiled2dMapVectorLineTile::setupLines), newGraphicObjects);
#endif

}

void Tiled2dMapVectorLineTile::setupLines(const std::vector<std::shared_ptr<GraphicsObjectInterface>> &newLineGraphicsObjects) {
    auto mapInterface = this->mapInterface.lock();
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) {
        return;
    }

    for (auto const &line: newLineGraphicsObjects) {
        if (!line->isReady()) line->setup(renderingContext);
    }


    auto selfActor = WeakActor<Tiled2dMapVectorTile>(mailbox, shared_from_this());
    tileCallbackInterface.message(MFN(&Tiled2dMapVectorLayerTileCallbackInterface::tileIsReady), tileInfo, description->identifier, selfActor);
}


std::vector<std::shared_ptr<RenderObjectInterface>> Tiled2dMapVectorLineTile::generateRenderObjects() {
    return renderObjects;
}

bool Tiled2dMapVectorLineTile::onClickConfirmed(const Vec2F &posScreen) {
    const auto mapInterface = this->mapInterface.lock();
    const auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!camera) {
        return false;
    }

    auto point = camera->coordFromScreenPosition(posScreen);
    return performClick(point);
}

bool Tiled2dMapVectorLineTile::performClick(const Coord &coord) {
    const auto mapInterface = this->mapInterface.lock();
    const auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    const auto coordinateConverter = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    auto strongSelectionDelegate = selectionDelegate.lock();
    if (!camera || !strongSelectionDelegate || !coordinateConverter) {
        return false;
    }

    double zoomIdentifier = layerConfig->getZoomIdentifier(camera->getZoom());

    auto lineDescription = std::static_pointer_cast<LineVectorLayerDescription>(description);
    
    std::vector<VectorLayerFeatureInfo> featureInfos;
    for (auto const &[lineCoordinateVector, featureContext]: hitDetection) {
        for (auto const &coordinates: lineCoordinateVector) {
            auto lineWidth = lineDescription->style.getLineWidth(EvaluationContext(zoomIdentifier, dpFactor, featureContext, featureStateManager));
            lineWidth *= selectionSizeFactor;
            auto lineWidthInMapUnits = camera->mapUnitsFromPixels(lineWidth);
            if (LineHelper::pointWithin(coordinates, coord, tileInfo.tileInfo.bounds.topLeft.systemIdentifier, lineWidthInMapUnits, coordinateConverter)) {
                if (multiselect) {
                    featureInfos.push_back(featureContext->getFeatureInfo());
                } else if (strongSelectionDelegate->didSelectFeature(featureContext->getFeatureInfo(), description->identifier, coord)) {
                    return true;
                }
            }
        }
    }

    if (multiselect && !featureInfos.empty()) {
        return strongSelectionDelegate->didMultiSelectLayerFeatures(featureInfos, description->identifier, coord);
    }

    return false;
}
