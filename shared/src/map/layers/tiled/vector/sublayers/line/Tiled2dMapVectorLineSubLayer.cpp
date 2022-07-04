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

Tiled2dMapVectorLineSubLayer::Tiled2dMapVectorLineSubLayer(const std::shared_ptr<LineVectorLayerDescription> &description)
        : description(description) {}

void Tiled2dMapVectorLineSubLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface) {
    Tiled2dMapVectorSubLayer::onAdded(mapInterface);
    shader = mapInterface->getShaderFactory()->createLineGroupShader();
}

void Tiled2dMapVectorLineSubLayer::onRemoved() {
    Tiled2dMapVectorSubLayer::onRemoved();
}

void Tiled2dMapVectorLineSubLayer::pause() {
    Tiled2dMapVectorSubLayer::pause();
    std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(tilesInSetupMutex, lineMutex);
    for (const auto &tileGroup : tileLinesMap) {
        tilesInSetup.insert(tileGroup.first);
        for (const auto &line : tileGroup.second) {
            line->getLineObject()->clear();
        }
    }
}

void Tiled2dMapVectorLineSubLayer::resume() {
    Tiled2dMapVectorSubLayer::resume();
    std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(tilesInSetupMutex, lineMutex);
    const auto &context = mapInterface->getRenderingContext();
    for (const auto &tileGroup : tileLinesMap) {
        for (const auto &line : tileGroup.second) {
            line->getLineObject()->setup(context);
        }
        tilesInSetup.erase(tileGroup.first);
        if (auto delegate = readyDelegate.lock()) {
            delegate->tileIsReady(tileGroup.first);
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
    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(mapInterface->getCamera()->getZoom());

    std::vector<LineStyle> styles;
    styles.reserve(featureGroups.size());
    for (auto const &feature: featureGroups) {
        auto const &context = EvaluationContext(zoomIdentifier, feature);
        styles.push_back(LineStyle(ColorStateList(description->style.color->evaluate(context), Color(0, 0, 0, 0)),
                                   ColorStateList(Color(0, 0, 0, 0), Color(0, 0, 0, 0)),
                                   description->style.opacity ? description->style.opacity->evaluate(context) : 1.0,
                                   SizeType::SCREEN_PIXEL,
                                   description->style.width ? description->style.width->evaluate(context) : 0.0,
                                   description->style.dashArray ? description->style.dashArray->evaluate(context) : std::vector<float>{},
                                   description->style.capType));
    }

    shader->setStyles(styles);
}

void
Tiled2dMapVectorLineSubLayer::updateTileData(const Tiled2dMapTileInfo &tileInfo, const std::shared_ptr<MaskingObjectInterface> &tileMask,
                                             const std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>> &layerFeatures) {
    Tiled2dMapVectorSubLayer::updateTileData(tileInfo, tileMask, layerFeatures);
    if (!mapInterface) {
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
        int subGroupCoordCount = 0;
        std::vector<std::vector<std::tuple<std::vector<Coord>, int>>> newLines;
        std::vector<std::tuple<std::vector<Coord>, int>> lineSubGroup;

        for (const auto &feature : layerFeatures) {
            const FeatureContext &featureContext = std::get<0>(feature);
            if ((description->filter == nullptr || description->filter->evaluate(EvaluationContext(-1, featureContext)))) {
                int styleIndex = 0;
                {
                    std::lock_guard<std::recursive_mutex> lock(featureGroupsMutex);
                    auto position = std::find(std::begin(featureGroups), std::end(featureGroups), featureContext);

                    if (position == featureGroups.end()) {
                        featureGroups.push_back(featureContext);
                        position = std::find(std::begin(featureGroups), std::end(featureGroups), featureContext);
                    }
                    styleIndex = (int) std::distance(featureGroups.begin(), position);
                }

                const VectorTileGeometryHandler &geometryHandler = std::get<1>(feature);
                for (const auto &lineCoordinates : geometryHandler.getLineCoordinates()) {
                    if (lineCoordinates.empty()) { continue; }

                    size_t numCoords = lineCoordinates.size();
                    if (subGroupCoordCount + numCoords > maxNumLinePoints && !lineSubGroup.empty()) {
                        newLines.push_back(lineSubGroup);
                        lineSubGroup = std::vector<std::tuple<std::vector<Coord>, int>>();
                        subGroupCoordCount = 0;
                    }

                    lineSubGroup.push_back({lineCoordinates, styleIndex});
                    subGroupCoordCount += numCoords;
                }

                featureNum++;
            }
        }

        if (!lineSubGroup.empty() && subGroupCoordCount > 0) newLines.push_back(lineSubGroup);

        addLines(tileInfo, newLines);

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
                                       const std::vector<std::vector<std::tuple<std::vector<Coord>, int>>> &lines) {
    if (lines.empty()) {
        if (auto delegate = readyDelegate.lock()) {
            delegate->tileIsReady(tileInfo);
        }
        return;
    }

    const auto &objectFactory = mapInterface->getGraphicsObjectFactory();

    std::vector<std::shared_ptr<LineGroup2dLayerObject>> lineGroupObjects;
    std::vector<std::shared_ptr<GraphicsObjectInterface>> newGraphicObjects;

    for (const auto &lineSubGroup : lines) {
        auto lineGroupGraphicsObject = objectFactory->createLineGroup(shader->asShaderProgramInterface());

        auto lineGroupObject = std::make_shared<LineGroup2dLayerObject>(mapInterface->getCoordinateConverterHelper(),
                                                                        lineGroupGraphicsObject, shader);
        lineGroupObject->setLines(lineSubGroup);

        lineGroupObjects.push_back(lineGroupObject);
        newGraphicObjects.push_back(lineGroupGraphicsObject->asGraphicsObject());
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

    {
        std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(tilesInSetupMutex, lineMutex);
        if (tileLinesMap.count(tileInfo) == 0) {
            if (auto delegate = readyDelegate.lock()) {
                delegate->tileIsReady(tileInfo);
            }
            return;
        }

        for (const auto &lineGraphicsObject : newLineGraphicsObjects) {
            if (!lineGraphicsObject->isReady()) lineGraphicsObject->setup(renderingContext);
        }

        tilesInSetup.erase(tileInfo);
    }

    if (auto delegate = readyDelegate.lock()) {
        delegate->tileIsReady(tileInfo);
    }
}

void Tiled2dMapVectorLineSubLayer::preGenerateRenderPasses() {
    std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(maskMutex, lineMutex);
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
        const auto &tileMask = tileMaskMap[tileLineTuple.first];
        for (const auto &passEntry : renderPassObjectMap) {
            std::shared_ptr<RenderPass> renderPass = std::make_shared<RenderPass>(RenderPassConfig(passEntry.first),
                                                                                  passEntry.second,
                                                                                  (tileMask
                                                                                   ? tileMask
                                                                                   : nullptr));
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
