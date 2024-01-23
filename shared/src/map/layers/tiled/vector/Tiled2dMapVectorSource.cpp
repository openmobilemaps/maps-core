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
                                               const std::vector<std::tuple<std::string, std::shared_ptr<Tiled2dMapLayerConfig>>> &layerConfigs,
                                               const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                               const std::shared_ptr<SchedulerInterface> &scheduler,
                                               const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders,
                                               const std::shared_ptr<Tiled2dMapSourceListenerInterface> &listener,
                                               const std::unordered_map<std::string, std::unordered_set<std::string>> &layersToDecode,
                                               float screenDensityPpi)
: Tiled2dMapSource<DataHolderInterface, IntermediateResult, FinalResult>(mapConfig, std::get<1>(*layerConfigs.begin()), conversionHelper, scheduler,
                                                                              listener, screenDensityPpi, tileLoaders.size()), loaders(tileLoaders), layersToDecode(layersToDecode), layerConfigs(layerConfigs) {}

IntermediateResult Tiled2dMapVectorSource::loadTile(Tiled2dMapTileInfo tile, size_t loaderIndex) {
    std::unordered_map<std::string, DataLoaderResult> results;
    for(auto const &[source, config]: layerConfigs) {
        if (!layersToDecode.empty() && layersToDecode.count(source) == 0) {
            continue;
        }
        results.insert({source, loaders[loaderIndex]->loadData(config->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier), std::nullopt)});

        if (!(results.at(source).status == LoaderStatus::OK || results.at(source).status == LoaderStatus::ERROR_404)) {
            return IntermediateResult(results, results.at(source).status, results.at(source).errorCode);
        }
    }
    return IntermediateResult(results, LoaderStatus::OK, std::nullopt);
}
FinalResult Tiled2dMapVectorSource::postLoadingTask(const IntermediateResult &loadedData, const Tiled2dMapTileInfo &tile) {
    FinalResult resultMap;

    for(auto const &[source, data_]: loadedData.results) {
        if (!isTileVisible(tile)) return FinalResult();
        
        if (data_.status == LoaderStatus::ERROR_404) continue;

        auto layerFeatureMap = std::make_shared<std::unordered_map<std::string, std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>>>>();

        const auto &data = data_.data->getData();
        try {
            vtzero::vector_tile tileData((char *) data.data(), data.size());

            while (auto layer = tileData.next_layer()) {
                std::string sourceLayerName = std::string(layer.name());
                if (layersToDecode.count(source) == 0  || (layersToDecode.at(source).count(sourceLayerName) > 0 && !layer.empty())) {
                    int extent = (int) layer.extent();
                    layerFeatureMap->emplace(sourceLayerName, std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>>());
                    layerFeatureMap->at(sourceLayerName).reserve(layer.num_features());
                    while (const auto &feature = layer.next_feature()) {
                        auto const featureContext = FeatureContext(feature);
                        try {
                            VectorTileGeometryHandler geometryHandler = VectorTileGeometryHandler(tile.bounds, extent, layerConfig->getVectorSettings());
                            vtzero::decode_geometry(feature.geometry(), geometryHandler);
                            layerFeatureMap->at(sourceLayerName).push_back({featureContext, geometryHandler});
                        } catch (vtzero::geometry_exception &geometryException) {
                            LogError <<= "geometryException for tile " + std::to_string(tile.zoomIdentifier) + "/" + std::to_string(tile.x) + "/" + std::to_string(tile.y);
                            continue;
                        }
                    }
                    if (layerFeatureMap->at(sourceLayerName).empty()) {
                        layerFeatureMap->erase(sourceLayerName);
                    }
                }
                if (!isTileVisible(tile)) return FinalResult();
            }

            if (!isTileVisible(tile)) return FinalResult();
        }
        catch (protozero::invalid_tag_exception tagException) {
            LogError <<= "Invalid tag exception for tile " + std::to_string(tile.zoomIdentifier) + "/" +
            std::to_string(tile.x) + "/" + std::to_string(tile.y);
        }
        catch (protozero::unknown_pbf_wire_type_exception typeException) {
            LogError <<= "Unknown wire type exception for tile " + std::to_string(tile.zoomIdentifier) + "/" +
            std::to_string(tile.x) + "/" + std::to_string(tile.y);
        }

        resultMap[source] = layerFeatureMap;
    }

    return resultMap;
}

std::unordered_set<Tiled2dMapVectorTileInfo> Tiled2dMapVectorSource::getCurrentTiles() {
    std::lock_guard<std::recursive_mutex> lock(currentTilesMutex);
    std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos;
    for (const auto &[tileInfo, tileWrapper] : currentTiles) {
        if (tileWrapper.isVisible) {
            currentTileInfos.insert(Tiled2dMapVectorTileInfo(tileInfo, tileWrapper.result, tileWrapper.masks));
        }
    }
    return currentTileInfos;
}

void Tiled2dMapVectorSource::pause() {
    // TODO: Stop loading tiles
}

void Tiled2dMapVectorSource::resume() {
    // TODO: Reload textures of current tiles
}
