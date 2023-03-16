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

Tiled2dMapVectorSource::Tiled2dMapVectorSource(const MapConfig &mapConfig,
                                               const std::unordered_map<std::string, std::shared_ptr<Tiled2dMapLayerConfig>> &layerConfigs,
                                               const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                               const std::shared_ptr<SchedulerInterface> &scheduler,
                                               const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders,
                                               const WeakActor<Tiled2dMapVectorSourceListener> &listener,
                                               const std::unordered_map<std::string, std::unordered_set<std::string>> &layersToDecode,
                                               float screenDensityPpi)
        : Tiled2dMapSource<djinni::DataRef, IntermediateResult, Tiled2dMapVectorTileInfo::FeatureMap>(mapConfig, layerConfigs.begin()->second, conversionHelper, scheduler, screenDensityPpi, tileLoaders.size()),
loaders(tileLoaders), layersToDecode(layersToDecode), layerConfigs(layerConfigs), listener(listener) {}

::djinni::Future<IntermediateResult> Tiled2dMapVectorSource::loadDataAsync(Tiled2dMapTileInfo tile, size_t loaderIndex) {
    auto promise = ::djinni::Promise<IntermediateResult>();
    std::lock_guard<std::recursive_mutex> lock(loadingStateMutex);
    loadingStates.insert({
        tile,
        Tiled2dMapVectorSourceTileState {
            promise,
            std::unordered_map<std::string, DataLoaderResult>{}
            
        }
    });
    for(auto const &[source, config]: layerConfigs) {
        const std::string sourceName = source;
        auto const url = config->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier);
        std::weak_ptr<Tiled2dMapVectorSource> weakRef = std::static_pointer_cast<Tiled2dMapVectorSource>(shared_from_this());
        loaders[loaderIndex]->loadDataAsync(url, std::nullopt).then([weakRef, tile, sourceName](::djinni::Future<DataLoaderResult> result) {
            auto ref = weakRef.lock();
            if (!ref) {

                return;
            }
            std::lock_guard<std::recursive_mutex> lock(ref->loadingStateMutex);
            ref->loadingStates.at(tile).results.insert({sourceName, result.get()});
            
            if (ref->loadingStates.at(tile).results.size() == ref->layerConfigs.size()) {
                auto [mergedStatus, mergedErrorCode] = ref->mergeLoaderStatus(ref->loadingStates.at(tile));
                ref->loadingStates.at(tile).promise.setValue(IntermediateResult(ref->loadingStates.at(tile).results, mergedStatus, mergedErrorCode));
                ref->loadingStates.erase(tile);
            }
            
        });
    }
    return promise.getFuture();
}

void Tiled2dMapVectorSource::cancelLoad(Tiled2dMapTileInfo tile, size_t loaderIndex) {
    for(auto const &[source, config]: layerConfigs) {
        auto const url = config->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier);
        loaders[loaderIndex]->cancel(url);
    }
}

Tiled2dMapVectorTileInfo::FeatureMap Tiled2dMapVectorSource::postLoadingTask(const IntermediateResult &loadedData, const Tiled2dMapTileInfo &tile) {
    Tiled2dMapVectorTileInfo::FeatureMap resultMap = std::make_shared<std::unordered_map<std::string, std::unordered_map<std::string, std::shared_ptr<std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>>>>>>();

    for(auto const &[source, data_]: loadedData.results) {
        //if (!isTileVisible(tile)) return Tiled2dMapVectorTileInfo::FeatureMap();

        auto layerFeatureMap = std::unordered_map<std::string, std::shared_ptr<std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>>>>();

        try {
            
            vtzero::vector_tile tileData((char*)data_.data->buf(), data_.data->len());

            while (auto layer = tileData.next_layer()) {
                std::string sourceLayerName = std::string(layer.name());
                if (layersToDecode.count(source) == 0  || (layersToDecode.at(source).count(sourceLayerName) > 0 && !layer.empty())) {
                    int extent = (int) layer.extent();
                    layerFeatureMap.emplace(sourceLayerName, std::make_shared<std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>>>());
                    layerFeatureMap.at(sourceLayerName)->reserve(layer.num_features());
                    while (const auto &feature = layer.next_feature()) {
                        auto const featureContext = FeatureContext(feature);
                        try {
                            VectorTileGeometryHandler geometryHandler = VectorTileGeometryHandler(tile.bounds, extent, layerConfig->getVectorSettings());
                            vtzero::decode_geometry(feature.geometry(), geometryHandler);
                            layerFeatureMap.at(sourceLayerName)->push_back({featureContext, geometryHandler});
                        } catch (vtzero::geometry_exception &geometryException) {
                            LogError <<= "geometryException for tile " + std::to_string(tile.zoomIdentifier) + "/" + std::to_string(tile.x) + "/" + std::to_string(tile.y);
                            continue;
                        }
                    }
                    if (layerFeatureMap.at(sourceLayerName)->empty()) {
                        layerFeatureMap.erase(sourceLayerName);
                    }
                }
                if (!isTileVisible(tile)) return Tiled2dMapVectorTileInfo::FeatureMap();
            }

            if (!isTileVisible(tile)) return Tiled2dMapVectorTileInfo::FeatureMap();
        }
        catch (protozero::invalid_tag_exception tagException) {
            LogError <<= "Invalid tag exception for tile " + std::to_string(tile.zoomIdentifier) + "/" +
            std::to_string(tile.x) + "/" + std::to_string(tile.y);
        }
        catch (protozero::unknown_pbf_wire_type_exception typeException) {
            LogError <<= "Unknown wire type exception for tile " + std::to_string(tile.zoomIdentifier) + "/" +
            std::to_string(tile.x) + "/" + std::to_string(tile.y);
        }

        resultMap->insert({source, std::move(layerFeatureMap)});
    }

    return resultMap;
}

void Tiled2dMapVectorSource::notifyTilesUpdates() {
    listener.message(MailboxDuplicationStrategy::replaceNewest, &Tiled2dMapVectorSourceListener::onTilesUpdated, "testSourceId" /* TODO */, getCurrentTiles());
};

std::unordered_set<Tiled2dMapVectorTileInfo> Tiled2dMapVectorSource::getCurrentTiles() {
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

std::tuple<LoaderStatus, std::optional<std::string>> Tiled2dMapVectorSource::mergeLoaderStatus(const Tiled2dMapVectorSourceTileState &tileStates) {
    LoaderStatus status = LoaderStatus::OK;
    std::optional<std::string> errorCode = std::nullopt;
    for (const auto &[source, result] : tileStates.results) {
        const LoaderStatus &tileStatus = result.status;
        if (tileStatus > status) {
            status = tileStatus;
            errorCode = result.errorCode;
        }
    }
    return std::make_tuple(status, errorCode);
}
