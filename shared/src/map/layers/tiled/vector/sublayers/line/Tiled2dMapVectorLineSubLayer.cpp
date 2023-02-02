/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorLineSubLayer.h"
#include <map>
#include "MapInterface.h"
#include "RenderObject.h"
#include "RenderPass.h"
#include "LambdaTask.h"
#include "VectorTileGeometryHandler.h"
#include "Tiled2dMapVectorRasterSubLayerConfig.h"
#include "MapCamera2dInterface.h"
#include "LineHelper.h"

Tiled2dMapVectorLineSubLayer::Tiled2dMapVectorLineSubLayer(const std::shared_ptr<LineVectorLayerDescription> &description)
        : description(description),
          usedKeys(description->getUsedKeys()) {}

void Tiled2dMapVectorLineSubLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface, int32_t layerIndex) {
    Tiled2dMapVectorSubLayer::onAdded(mapInterface, layerIndex);
}

void Tiled2dMapVectorLineSubLayer::onRemoved() {
    Tiled2dMapVectorSubLayer::onRemoved();
}

void Tiled2dMapVectorLineSubLayer::pause() {
    Tiled2dMapVectorSubLayer::pause();

    std::unordered_set<Tiled2dMapTileInfo> tilesToInsertIntoSetup;
    std::vector<std::shared_ptr<GraphicsObjectInterface>> linesToClear;
    {
        std::lock_guard<std::recursive_mutex> lock(lineMutex);
        for (const auto &tileGroup : tileLinesMap) {
            tilesToInsertIntoSetup.insert(tileGroup.first);
            for (const auto &line : tileGroup.second) {
                linesToClear.push_back(line->getLineObject());
            }
        }
    }

    {
        std::lock_guard<std::recursive_mutex> lock(tilesInSetupMutex);
        for (const auto &line : tilesToInsertIntoSetup) {
            tilesInSetup.insert(line);
        }
    }


    for (const auto &line : linesToClear) {
        line->clear();
    }
}

void Tiled2dMapVectorLineSubLayer::resume() {
    Tiled2dMapVectorSubLayer::resume();
    const auto &context = mapInterface->getRenderingContext();

    std::vector<std::shared_ptr<GraphicsObjectInterface>> linesToSetup;
    std::unordered_set<Tiled2dMapTileInfo> tilesToEraseFromSetup;

    {
        std::lock_guard<std::recursive_mutex> lock(lineMutex);
        for (const auto &tileGroup : tileLinesMap) {
            for (const auto &line : tileGroup.second) {
                linesToSetup.push_back(line->getLineObject());
            }
            tilesToEraseFromSetup.insert(tileGroup.first);
        }
    }

    for(auto const &line: linesToSetup) {
        line->setup(context);
    }

    {
        std::lock_guard<std::recursive_mutex> lock(tilesInSetupMutex);
        for(auto const &line: tilesToEraseFromSetup) {
            tilesInSetup.erase(line);
        }
    }

    if (auto delegate = readyDelegate.lock()) {
        for(auto const &line: tilesToEraseFromSetup) {
            delegate->tileIsReady(line);
        }
    }
}

void Tiled2dMapVectorLineSubLayer::hide() {
    Tiled2dMapVectorSubLayer::hide();
}

void Tiled2dMapVectorLineSubLayer::show() {
    Tiled2dMapVectorSubLayer::show();
}

void Tiled2dMapVectorLineSubLayer::update() {
    std::lock_guard<std::recursive_mutex> lock(featureGroupsMutex);
    if (shaders.empty()) {
        return;
    }
    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(mapInterface->getCamera()->getZoom());

    size_t numStyleGroups = featureGroups.size();
    for (int styleGroupId = 0; styleGroupId < numStyleGroups; styleGroupId++) {
        int i = 0;
        bool needsUpdate = false;
        for (auto const &[key, feature]: featureGroups.at(styleGroupId)) {
            auto const &context = EvaluationContext(zoomIdentifier, feature);
            auto &style = reusableLineStyles.at(styleGroupId).at(i);
            auto normalColor = description->style.getLineColor(context);
            if (normalColor != style.color.normal) {
                style.color.normal = normalColor;
                needsUpdate = true;
            }
            float opacity = description->style.getLineOpacity(context);
            if (opacity != style.opacity) {
                style.opacity = opacity * alpha;
                needsUpdate = true;
            }
            float blur = description->style.getLineBlur(context);
            if (blur != style.blur) {
                style.blur = blur;
                needsUpdate = true;
            }
            auto widthType = SizeType::SCREEN_PIXEL;
            if (widthType != style.widthType) {
                style.widthType = widthType;
                needsUpdate = true;
            }
            float width = description->style.getLineWidth(context);
            if (width != style.width) {
                style.width = width;
                needsUpdate = true;
            }
            auto dashArray = description->style.getLineDashArray(context);
            if (dashArray != style.dashArray) {
                style.dashArray = dashArray;
                needsUpdate = true;
            }
            auto lineCap = description->style.getLineCap(context);
            if (lineCap != style.lineCap) {
                style.lineCap = lineCap;
                needsUpdate = true;
            }


            i++;
        }

        if (needsUpdate) {
            shaders.at(styleGroupId)->setStyles(reusableLineStyles.at(styleGroupId));
        }
    }
}

void
Tiled2dMapVectorLineSubLayer::updateTileData(const Tiled2dMapTileInfo &tileInfo, const std::shared_ptr<MaskingObjectInterface> &tileMask,
                                             const std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>> &layerFeatures) {
    Tiled2dMapVectorSubLayer::updateTileData(tileInfo, tileMask, layerFeatures);
    auto mapInterface = this->mapInterface;
    const auto &shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
    if (!shaderFactory) {
        return;
    }


    std::string layerName = std::string(description->sourceId);
    //LogDebug <<= "    layer: " + layerName + " - v" + std::to_string(data.version()) + " - extent: " +
    //             std::to_string(extent);
    std::string defIdPrefix = std::to_string(tileInfo.x) + "/" + std::to_string(tileInfo.y) + "_" + layerName + "_";
    if (!layerFeatures.empty() &&
        description->minZoom <= tileInfo.zoomIdentifier &&
        description->maxZoom >= tileInfo.zoomIdentifier) {
        //LogDebug <<= "      with " + std::to_string(data.num_features()) + " features";
        int featureNum = 0;
        std::unordered_map<int, int> subGroupCoordCount;
        std::unordered_map<int, std::vector<std::vector<std::tuple<std::vector<Coord>, int>>>> styleGroupNewLinesMap;
        std::unordered_map<int, std::vector<std::tuple<std::vector<Coord>, int>>> styleGroupLineSubGroupMap;

        for (auto featureIt = layerFeatures.rbegin(); featureIt != layerFeatures.rend(); ++featureIt) {
            const FeatureContext &featureContext = std::get<0>(*featureIt);
            if ((description->filter == nullptr || description->filter->evaluateOr(EvaluationContext(-1, featureContext), true))) {
                int styleGroupIndex = -1;
                int styleIndex = -1;
                {
                    auto const hash = featureContext.getStyleHash(usedKeys);
                    std::lock_guard<std::recursive_mutex> lock(featureGroupsMutex);

                    auto indexPair = styleHashToGroupMap.find(hash);
                    if (indexPair != styleHashToGroupMap.end()) {
                        styleGroupIndex = indexPair->second.first;
                        styleIndex = indexPair->second.second;
                    }

                    if (styleIndex == -1) {
                        auto reusableStyle = LineStyle(ColorStateList(Color(0.0, 0.0, 0.0, 0.0), Color(0.0, 0.0, 0.0, 0.0)),
                                          ColorStateList(Color(0.0, 0.0, 0.0, 0.0), Color(0.0, 0.0, 0.0, 0.0)),
                                          0.0,
                                          0.0,
                                          SizeType::MAP_UNIT,
                                          0.0,
                                          {},
                                          LineCapType::BUTT);
                        if (!featureGroups.empty() && featureGroups.back().size() < maxStylesPerGroup) {
                            styleGroupIndex = (int) featureGroups.size() - 1;
                            styleIndex = (int) featureGroups.back().size();
                            featureGroups.at(styleGroupIndex).push_back({ hash, featureContext });
                            reusableLineStyles.at(styleGroupIndex).push_back(reusableStyle);
                        } else {
                            styleGroupIndex = (int) featureGroups.size();
                            styleIndex = 0;
                            shaders.push_back(shaderFactory->createLineGroupShader());
                            reusableLineStyles.push_back(std::vector<LineStyle>{reusableStyle});
                            featureGroups.push_back(std::vector<std::tuple<size_t, FeatureContext>>{{hash, featureContext}});
                        }
                        styleHashToGroupMap.insert({hash, {styleGroupIndex, styleIndex}});
                    }
                }

                if (subGroupCoordCount.count(styleGroupIndex) == 0) {
                    subGroupCoordCount[styleGroupIndex] = 0;
                }

                const VectorTileGeometryHandler &geometryHandler = std::get<1>(*featureIt);
                std::vector<std::vector<::Coord>> lineCoordinatesVector;

                for (const auto &lineCoordinates: geometryHandler.getLineCoordinates()) {
                    if (lineCoordinates.empty()) { continue; }

                    int numCoords = (int)lineCoordinates.size();
                    int coordCount = subGroupCoordCount[styleGroupIndex];
                    if (coordCount + numCoords > maxNumLinePoints
                        && !styleGroupLineSubGroupMap[styleGroupIndex].empty()) {
                        styleGroupNewLinesMap[styleGroupIndex].push_back(styleGroupLineSubGroupMap[styleGroupIndex]);
                        styleGroupLineSubGroupMap[styleGroupIndex] = std::vector<std::tuple<std::vector<Coord>, int>>();
                        subGroupCoordCount[styleGroupIndex] = 0;
                    }

                    styleGroupLineSubGroupMap[styleGroupIndex].push_back({lineCoordinates, std::min(maxStylesPerGroup - 1, styleIndex)});
                    subGroupCoordCount[styleGroupIndex] = (int)subGroupCoordCount[styleGroupIndex] + numCoords;
                    lineCoordinatesVector.push_back(lineCoordinates);
                }


                hitDetectionLineMap[tileInfo].push_back({lineCoordinatesVector, featureContext});

                featureNum++;
            }
        }

        for (const auto &[groupIndex, lineSubGroup] : styleGroupLineSubGroupMap) {
            if (!lineSubGroup.empty() && subGroupCoordCount[groupIndex] > 0) styleGroupNewLinesMap[groupIndex].push_back(lineSubGroup);
        }

        addLines(tileInfo, styleGroupNewLinesMap);

        preGenerateRenderPasses();
    } else {
        if (auto delegate = readyDelegate.lock()) {
            delegate->tileIsReady(tileInfo);
        }
    }
}

void Tiled2dMapVectorLineSubLayer::clearTileData(const Tiled2dMapTileInfo &tileInfo) {
    auto mapInterface = this->mapInterface;
    if (!mapInterface) {
        return;
    }
    hitDetectionLineMap.erase(tileInfo);

    std::vector<std::shared_ptr<GraphicsObjectInterface>> objectsToClear;
    Tiled2dMapVectorSubLayer::clearTileData(tileInfo);
    {
        std::lock_guard<std::recursive_mutex> lock(lineMutex);
        if (tileLinesMap.count(tileInfo) != 0) {
            for (const auto &lineObject : tileLinesMap[tileInfo]) {
                if (lineObject->getLineObject()) objectsToClear.push_back(lineObject->getLineObject());
            }
            tileLinesMap.erase(tileInfo);
            //LogDebug <<= "Erased tile " + std::to_string(tileInfo.zoomIdentifier) + "/" + std::to_string(tileInfo.x) + "/" +
            //             std::to_string(tileInfo.y);
        }
    }
    if (objectsToClear.empty()) return;

    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
            TaskConfig("LineGroupTile_clear_" + std::to_string(tileInfo.zoomIdentifier) + "/" + std::to_string(tileInfo.x) + "/" +
                       std::to_string(tileInfo.y), 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
            [objectsToClear] {
                for (const auto &lineObject : objectsToClear) {
                    if (lineObject->isReady()) lineObject->clear();
                }
            }));
}

void
Tiled2dMapVectorLineSubLayer::addLines(const Tiled2dMapTileInfo &tileInfo,
                                       const std::unordered_map<int, std::vector<std::vector<std::tuple<std::vector<Coord>, int>>>> &styleIdLinesMap) {

    if (styleIdLinesMap.empty()) {
        if (auto delegate = readyDelegate.lock()) {
            delegate->tileIsReady(tileInfo);
        }
        return;
    }

    auto mapInterface = this->mapInterface;
    const auto &objectFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
    const auto &coordinateConverterHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    if (!objectFactory || !coordinateConverterHelper) {
        return;
    }

    std::vector<std::shared_ptr<LineGroup2dLayerObject>> lineGroupObjects;
    std::vector<std::shared_ptr<GraphicsObjectInterface>> newGraphicObjects;

    for (const auto &[styleGroupIndex, lines] : styleIdLinesMap) {
        for (const auto &lineSubGroup: lines) {
            auto lineGroupGraphicsObject = objectFactory->createLineGroup(shaders.at(styleGroupIndex)->asShaderProgramInterface());

            auto lineGroupObject = std::make_shared<LineGroup2dLayerObject>(coordinateConverterHelper,
                                                                            lineGroupGraphicsObject,
                                                                            shaders.at(styleGroupIndex));
            lineGroupObject->setLines(lineSubGroup);

            lineGroupObjects.push_back(lineGroupObject);
            newGraphicObjects.push_back(lineGroupGraphicsObject->asGraphicsObject());
        }
    }

    {
        std::lock_guard<std::recursive_mutex> lock(lineMutex);
        tileLinesMap[tileInfo] = lineGroupObjects;
        //LogDebug <<= "Added tile " + std::to_string(tileInfo.zoomIdentifier) + "/" + std::to_string(tileInfo.x) + "/" +
        //             std::to_string(tileInfo.y);
    }

    {
        std::lock_guard<std::recursive_mutex> lock(tilesInSetupMutex);
        tilesInSetup.insert(tileInfo);
    }

    std::weak_ptr<Tiled2dMapVectorLineSubLayer> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapVectorLineSubLayer>(
            shared_from_this());
    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
            TaskConfig("LineLayer_setup", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
            [weakSelfPtr, tileInfo, newGraphicObjects] {
                auto selfPtr = weakSelfPtr.lock();
                if (selfPtr) selfPtr->setupLines(tileInfo, newGraphicObjects);
            }));
}

void Tiled2dMapVectorLineSubLayer::setupLines(const Tiled2dMapTileInfo &tileInfo, const std::vector<std::shared_ptr<GraphicsObjectInterface>> &newLineGraphicsObjects) {
    auto mapInterface = this->mapInterface;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext)
    {
        return;
    }

    bool shouldReturnAndSetReady = false;
    {
        std::lock_guard<std::recursive_mutex> lock(lineMutex);
        if (tileLinesMap.count(tileInfo) == 0) {
            shouldReturnAndSetReady = true;
        }
    }

    if (shouldReturnAndSetReady) {
        if (auto delegate = readyDelegate.lock()) {
            delegate->tileIsReady(tileInfo);
        }
        return;
    }

    for (const auto &lineGraphicsObject : newLineGraphicsObjects) {
        if (!lineGraphicsObject->isReady()) lineGraphicsObject->setup(renderingContext);
    }

    {
        std::lock_guard<std::recursive_mutex> lock(tilesInSetupMutex);
        tilesInSetup.erase(tileInfo);
    }

    if (auto delegate = readyDelegate.lock()) {
        delegate->tileIsReady(tileInfo);
    }
}

void Tiled2dMapVectorLineSubLayer::preGenerateRenderPasses() {
    std::lock_guard<std::recursive_mutex> lock(lineMutex);
    std::unordered_map<Tiled2dMapTileInfo, std::vector<std::shared_ptr<RenderPassInterface>>> newRenderPasses;
    for (auto const &tileLineTuple : tileLinesMap) {
        std::vector<std::shared_ptr<RenderPassInterface>> newTileRenderPasses;
        std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> renderPassObjectMap;
        for (const auto &line : tileLineTuple.second) {
            for (const auto &config : line->getRenderConfig()) {
                renderPassObjectMap[config->getRenderIndex()].push_back(
                        std::make_shared<RenderObject>(config->getGraphicsObject()));
            }
        }
        std::shared_ptr<MaskingObjectInterface> tileMask;
        {
            std::lock_guard<std::recursive_mutex> maskLock(maskMutex);
            tileMask = tileMaskMap[tileLineTuple.first];
        }
        for (const auto &passEntry : renderPassObjectMap) {
            std::shared_ptr<RenderPass> renderPass = std::make_shared<RenderPass>(RenderPassConfig(description->renderPassIndex.value_or(passEntry.first)),
                                                                                  passEntry.second,
                                                                                  (tileMask
                                                                                   ? tileMask
                                                                                   : nullptr));
            renderPass->setScissoringRect(scissorRect);
            newTileRenderPasses.push_back(renderPass);
        }
        newRenderPasses.insert({tileLineTuple.first, newTileRenderPasses});
    }
    renderPasses = newRenderPasses;
}

void Tiled2dMapVectorLineSubLayer::updateTileMask(const Tiled2dMapTileInfo &tileInfo,
                                                  const std::shared_ptr<MaskingObjectInterface> &tileMask) {
    Tiled2dMapVectorSubLayer::updateTileMask(tileInfo, tileMask);
    preGenerateRenderPasses();
}

void Tiled2dMapVectorLineSubLayer::setScissorRect(const std::optional<::RectI> &scissorRect) {
    this->scissorRect = scissorRect;
    preGenerateRenderPasses();
}

std::string Tiled2dMapVectorLineSubLayer::getLayerDescriptionIdentifier() {
    return description->identifier;
}

bool Tiled2dMapVectorLineSubLayer::onClickConfirmed(const ::Vec2F &posScreen) {
    auto selectionDelegate = this->selectionDelegate.lock();
    auto mapInterface = this->mapInterface;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto coordinateConverter = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    if (!camera || !coordinateConverter || !selectionDelegate) {
        return false;
    }

    auto point = camera->coordFromScreenPosition(posScreen);
    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(camera->getZoom());

    for (auto const &[tileInfo, lineTuples] : hitDetectionLineMap) {
        for (auto const &[lineCoordinateVector, featureContext]: lineTuples) {
            for (auto const &coordinates: lineCoordinateVector) {
                auto lineWidth = description->style.getLineWidth(EvaluationContext(zoomIdentifier, featureContext));
                if (LineHelper::pointWithin(coordinates, point, lineWidth, coordinateConverter)) {
                    if (selectionDelegate->didSelectFeature(featureContext, description, point)) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}
