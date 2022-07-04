/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */


#include "Tiled2dMapVectorSource.h"
#include "vtzero/vector_tile.hpp"
#include "Logger.h"
#include "DataHolderInterface.h"

Tiled2dMapVectorSource::Tiled2dMapVectorSource(const MapConfig &mapConfig,
                                               const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                               const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                               const std::shared_ptr<SchedulerInterface> &scheduler,
                                               const std::shared_ptr<::LoaderInterface> & tileLoader,
                                               const std::shared_ptr<Tiled2dMapSourceListenerInterface> &listener,
                                               const std::unordered_set<std::string> &layersToDecode,
                                               float screenDensityPpi)
        : Tiled2dMapSource<DataHolderInterface, DataLoaderResult, std::shared_ptr<LayerFeatureMapType>>(mapConfig, layerConfig, conversionHelper, scheduler,
                                                                              listener, screenDensityPpi), loader(tileLoader), layersToDecode(layersToDecode) {}

DataLoaderResult Tiled2dMapVectorSource::loadTile(Tiled2dMapTileInfo tile) {
    return loader->loadData(layerConfig->getTileUrl(tile.x, tile.y, tile.zoomIdentifier), std::nullopt);
}
std::shared_ptr<LayerFeatureMapType> Tiled2dMapVectorSource::postLoadingTask(const DataLoaderResult &loadedData, const Tiled2dMapTileInfo &tile) {

    std::shared_ptr<LayerFeatureMapType> layerFeatureMap = std::make_shared<LayerFeatureMapType>();

    const auto &data = loadedData.data->getData();
    try {
        vtzero::vector_tile tileData((char *) data.data(), data.size());

        while (auto layer = tileData.next_layer()) {
            std::string sourceLayerName = std::string(layer.name());
            if (layersToDecode.count(sourceLayerName) > 0 && !layer.empty()) {
                int extent = (int) layer.extent();
                std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>> features;
                features.reserve(layer.num_features());
                while (const auto &feature = layer.next_feature()) {
                    auto const featureContext = FeatureContext(feature);
                    try {
                        VectorTileGeometryHandler geometryHandler = VectorTileGeometryHandler(tile.bounds, extent);
                        vtzero::decode_geometry(feature.geometry(), geometryHandler);
                        features.push_back({featureContext, geometryHandler});
                    } catch (vtzero::geometry_exception &geometryException) {
                        continue;
                    }
                }
                if (!features.empty()) {
                    layerFeatureMap->emplace(sourceLayerName, features);
                }
            }
            if (!isTileVisible(tile)) return std::make_shared<LayerFeatureMapType>();
        }

        if (!isTileVisible(tile)) return std::make_shared<LayerFeatureMapType>();
    }
    catch (protozero::invalid_tag_exception tagException) {
        LogError <<= "Invalid tag exception for tile " + std::to_string(tile.zoomIdentifier) + "/" +
                     std::to_string(tile.x) + "/" + std::to_string(tile.y);
    }
    catch (protozero::unknown_pbf_wire_type_exception typeException) {
        LogError <<= "Unknown wire type exception for tile " + std::to_string(tile.zoomIdentifier) + "/" +
                     std::to_string(tile.x) + "/" + std::to_string(tile.y);
    }

    return layerFeatureMap;
}

std::unordered_set<Tiled2dMapVectorTileInfo> Tiled2dMapVectorSource::getCurrentTiles() {
    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
    std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos;
    for (const auto &tileEntry : currentTiles) {
        currentTileInfos.insert(Tiled2dMapVectorTileInfo(tileEntry.first, tileEntry.second.result, tileEntry.second.mask));
    }
    return currentTileInfos;
}

void Tiled2dMapVectorSource::pause() {
    // TODO: Stop loading tiles
}

void Tiled2dMapVectorSource::resume() {
    // TODO: Reload textures of current tiles
}
