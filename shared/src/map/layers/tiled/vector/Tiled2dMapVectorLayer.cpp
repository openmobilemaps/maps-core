/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include <logger/Logger.h>
#include "vtzero/vector_tile.hpp"
#include "Tiled2dMapVectorLayer.h"

Tiled2dMapVectorLayer::Tiled2dMapVectorLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                             const std::shared_ptr<::TileLoaderInterface> &tileLoader)
        : Tiled2dMapLayer(layerConfig), vectorTileLoader(tileLoader) {
    sublayer = std::make_shared<Tiled2dMapVectorSubLayer>(Color(0.0, 0.0, 1.0, 0.5));
}

std::shared_ptr<::LayerInterface> Tiled2dMapVectorLayer::asLayerInterface() {
    return shared_from_this();
}

void Tiled2dMapVectorLayer::update() {
    sublayer->update();
}

std::vector<std::shared_ptr<::RenderPassInterface>> Tiled2dMapVectorLayer::buildRenderPasses() {
    auto passes = sublayer->buildRenderPasses();
    return passes;
}

void Tiled2dMapVectorLayer::onAdded(const std::shared_ptr<::MapInterface> &mapInterface) {
    vectorTileSource = std::make_shared<Tiled2dMapVectorSource>(mapInterface->getMapConfig(), layerConfig,
                                                                mapInterface->getCoordinateConverterHelper(),
                                                                mapInterface->getScheduler(), vectorTileLoader, shared_from_this());
    setSourceInterface(vectorTileSource);
    Tiled2dMapLayer::onAdded(mapInterface);
    mapInterface->getTouchHandler()->addListener(shared_from_this());

    sublayer->onAdded(mapInterface);
}

void Tiled2dMapVectorLayer::onRemoved() {
    Tiled2dMapLayer::onRemoved();
    mapInterface->getTouchHandler()->removeListener(shared_from_this());
    pause();

    sublayer->onRemoved();
}

void Tiled2dMapVectorLayer::pause() {
    vectorTileSource->pause();
    sublayer->pause();
}

void Tiled2dMapVectorLayer::resume() {
    vectorTileSource->resume();
    sublayer->resume();
}

void Tiled2dMapVectorLayer::onTilesUpdated() {
    auto currentTileInfos = vectorTileSource->getCurrentTiles();

    {
        std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);

        std::unordered_set<Tiled2dMapVectorTileInfo> tilesToAdd;
        for (const auto &vectorTileInfo : currentTileInfos) {
            if (tileSet.count(vectorTileInfo) == 0) {
                tilesToAdd.insert(vectorTileInfo);
            }
        }

        std::unordered_set<Tiled2dMapVectorTileInfo> tilesToRemove;
        for (const auto &tileEntry : tileSet) {
            if (currentTileInfos.count(tileEntry) == 0)
                tilesToRemove.insert(tileEntry);
        }

        for (const auto &tile : tilesToAdd) {
            auto data = tile.vectorTileHolder->getData();
            std::vector<std::shared_ptr<PolygonInfo>> polygons;
            try {
                vtzero::vector_tile tileData((char*)data.data(), data.size());

                LogDebug <<= "tile " + std::to_string(tile.tileInfo.zoomIdentifier) + "/" + std::to_string(tile.tileInfo.x) + "/" +
                             std::to_string(tile.tileInfo.y);
                std::vector<std::string> geomTypes = {"UNKNOWN", "POINT", "LINESTRING", "POLYGON"};

                while (auto layer = tileData.next_layer()) {
                    std::string layerName = std::string(layer.name());
                    if (layerName == "water" && !layer.empty()) sublayer->updateTileData(tile, layer);
                }
            }
            catch (protozero::invalid_tag_exception tagException) {
                LogError <<= "Invalid tag exception for tile " + std::to_string(tile.tileInfo.zoomIdentifier) + "/" +
                             std::to_string(tile.tileInfo.x) + "/" + std::to_string(tile.tileInfo.y);
            }
            catch (protozero::unknown_pbf_wire_type_exception typeException) {
                LogError <<= "Unknown wire type exception for tile " + std::to_string(tile.tileInfo.zoomIdentifier) + "/" +
                             std::to_string(tile.tileInfo.x) + "/" + std::to_string(tile.tileInfo.y);
            }
            tileSet.insert(tile);
        }

        for (const auto &tile : tilesToRemove) {
            sublayer->clearTileData(tile);
            tileSet.erase(tile);
        }

        if (!tilesToAdd.empty() || !tilesToRemove.empty()) sublayer->preGenerateRenderPasses();
    }

    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
            TaskConfig("Tiled2dMapVectorLayer_onTilesUpdated", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS), [=] {
                auto renderingContext = mapInterface->getRenderingContext();
                std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
                // TODO
            }));
    mapInterface->invalidate();
}
