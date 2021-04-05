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
#include "PolygonLayerInterface.h"
#include "PolygonInfo.h"
#include "VectorTilePolygonHandler.h"

Tiled2dMapVectorLayer::Tiled2dMapVectorLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                             const std::shared_ptr<::TileLoaderInterface> &tileLoader)
        : Tiled2dMapLayer(layerConfig), vectorTileLoader(tileLoader) {
    polygonLayer = std::static_pointer_cast<PolygonLayer>(PolygonLayerInterface::create());
}

std::shared_ptr<::LayerInterface> Tiled2dMapVectorLayer::asLayerInterface() {
    return shared_from_this();
}

void Tiled2dMapVectorLayer::update() {
    polygonLayer->update();
}

std::vector<std::shared_ptr<::RenderPassInterface>> Tiled2dMapVectorLayer::buildRenderPasses() {
    auto passes = polygonLayer->buildRenderPasses();
    LogDebug <<= "Renderpass objects: " + std::to_string(passes.empty() ? 0 : passes[0]->getRenderObjects().size());
    return passes;
}

void Tiled2dMapVectorLayer::onAdded(const std::shared_ptr<::MapInterface> &mapInterface) {
    vectorTileSource = std::make_shared<Tiled2dMapVectorSource>(mapInterface->getMapConfig(), layerConfig,
                                                                mapInterface->getCoordinateConverterHelper(),
                                                                mapInterface->getScheduler(), vectorTileLoader, shared_from_this());
    setSourceInterface(vectorTileSource);
    Tiled2dMapLayer::onAdded(mapInterface);
    mapInterface->getTouchHandler()->addListener(shared_from_this());

    polygonLayer->onAdded(mapInterface);
}

void Tiled2dMapVectorLayer::onRemoved() {
    Tiled2dMapLayer::onRemoved();
    mapInterface->getTouchHandler()->removeListener(shared_from_this());
    pause();

    polygonLayer->onRemoved();
}

void Tiled2dMapVectorLayer::pause() {
    vectorTileSource->pause();
    polygonLayer->pause();
}

void Tiled2dMapVectorLayer::resume() {
    vectorTileSource->resume();
    polygonLayer->resume();
}

void Tiled2dMapVectorLayer::onTilesUpdated() {
    auto currentTileInfos = vectorTileSource->getCurrentTiles();

    {
        std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);

        std::unordered_set<Tiled2dMapVectorTileInfo> tilesToAdd;
        for (const auto &vectorTileInfo : currentTileInfos) {
            if (tileObjectMap.count(vectorTileInfo) == 0) {
                tilesToAdd.insert(vectorTileInfo);
            }
        }

        std::unordered_set<Tiled2dMapVectorTileInfo> tilesToRemove;
        for (const auto &tileEntry : tileObjectMap) {
            if (currentTileInfos.count(tileEntry.first) == 0)
                tilesToRemove.insert(tileEntry.first);
        }

        for (const auto &tile : tilesToAdd) {
            auto data = tile.vectorTileHolder->getData();
            std::vector<std::shared_ptr<PolygonInfo>> polygons;
            try {
                vtzero::vector_tile tileData(std::string(data.begin(), data.end()));

                LogDebug <<= "tile " + std::to_string(tile.tileInfo.zoomIdentifier) + "/" + std::to_string(tile.tileInfo.x) + "/" +
                             std::to_string(tile.tileInfo.y);
                std::vector<std::string> geomTypes = {"UNKNOWN", "POINT", "LINESTRING", "POLYGON"};

                std::string tileTempIdPrefix = std::to_string(tile.tileInfo.x) + "/" + std::to_string(tile.tileInfo.y) + "_";
                while (auto layer = tileData.next_layer()) {
                    std::string layerName = std::string(layer.name());
                    uint32_t extent = layer.extent();
                    LogDebug <<= "    layer: " + layerName + " - v" + std::to_string(layer.version()) + " - extent: " +
                                 std::to_string(extent);
                    std::string layerTempIdPrefix = tileTempIdPrefix + layerName + "_";
                    if (layerName == "water" && !layer.empty()) {
                        LogDebug <<= "      with " + std::to_string(layer.num_features()) + " features";
                        int featureNum = 0;
                        std::vector<PolygonInfo> newPolygons;
                        while (auto feature = layer.next_feature()) {
                            if (feature.geometry_type() == vtzero::GeomType::POLYGON) {
                                auto polygonHandler = VectorTilePolygonHandler(tile.tileInfo.bounds, extent);
                                try {
                                    vtzero::decode_polygon_geometry(feature.geometry(), polygonHandler);
                                } catch (vtzero::geometry_exception geometryException) {
                                    continue;
                                }
                                std::string id = feature.has_id() ? std::to_string(feature.id()) : (layerTempIdPrefix +
                                                                                                    std::to_string(featureNum));
                                auto polygonCoordinates = polygonHandler.getCoordinates();
                                auto polygonHoles = polygonHandler.getHoles();
                                for (int i = 0; i < polygonCoordinates.size(); i++) {
                                    auto polygonInfo = std::make_shared<PolygonInfo>(id, polygonCoordinates[i],
                                                                                     polygonHoles[i],
                                                                                     false, Color(0.0, 0.0, 1.0, 0.5),
                                                                                     Color(0.0, 0.0, 1.0, 0.5));
                                    polygons.push_back(polygonInfo);
                                    newPolygons.push_back(*polygonInfo);
                                    featureNum++;
                                }
                            }
                        }
                        polygonLayer->addAll(newPolygons);
                    }
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
            tileObjectMap[tile] = polygons;
        }

        for (const auto &tile : tilesToRemove) {
            auto polygons = tileObjectMap[tile];
            for (const auto &polygon : polygons) {
                polygonLayer->remove(*polygon);
            }
            tileObjectMap.erase(tile);
        }
    }

    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
            TaskConfig("Tiled2dMapVectorLayer_onTilesUpdated", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS), [=] {
                auto renderingContext = mapInterface->getRenderingContext();
                std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
                // TODO
            }));
    mapInterface->invalidate();
}
