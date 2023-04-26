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
                                               const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                               const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                               const std::shared_ptr<SchedulerInterface> &scheduler,
                                               const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders,
                                               const WeakActor<Tiled2dMapVectorSourceListener> &listener,
                                               const std::unordered_set<std::string> &layersToDecode,
                                               const std::string &sourceName,
                                               float screenDensityPpi)
        : Tiled2dMapSource<djinni::DataRef, DataLoaderResult, Tiled2dMapVectorTileInfo::FeatureMap>(mapConfig, layerConfig, conversionHelper, scheduler, screenDensityPpi, tileLoaders.size()),
loaders(tileLoaders), layersToDecode(layersToDecode), listener(listener), sourceName(sourceName) {}

::djinni::Future<DataLoaderResult> Tiled2dMapVectorSource::loadDataAsync(Tiled2dMapTileInfo tile, size_t loaderIndex) {
    auto const url = layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier);
    return loaders[loaderIndex]->loadDataAsync(url, std::nullopt);
}

void Tiled2dMapVectorSource::cancelLoad(Tiled2dMapTileInfo tile, size_t loaderIndex) {
    auto const url = layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier);
    loaders[loaderIndex]->cancel(url);
}

Tiled2dMapVectorTileInfo::FeatureMap Tiled2dMapVectorSource::postLoadingTask(const DataLoaderResult &loadedData, const Tiled2dMapTileInfo &tile) {
    auto layerFeatureMap = std::make_shared<std::unordered_map<std::string, std::shared_ptr<std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>>>>>();
    try {
        vtzero::vector_tile tileData((char*)loadedData.data->buf(), loadedData.data->len());

        while (auto layer = tileData.next_layer()) {
            std::string sourceLayerName = std::string(layer.name());
            if ((layersToDecode.count(sourceLayerName) > 0 && !layer.empty())) {
                int extent = (int) layer.extent();
                layerFeatureMap->emplace(sourceLayerName, std::make_shared<std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>>>());
                layerFeatureMap->at(sourceLayerName)->reserve(layer.num_features());
                while (const auto &feature = layer.next_feature()) {
                    auto const featureContext = FeatureContext(feature);
                    try {
                        VectorTileGeometryHandler geometryHandler = VectorTileGeometryHandler(tile.bounds, extent, layerConfig->getVectorSettings());
                        vtzero::decode_geometry(feature.geometry(), geometryHandler);
                        geometryHandler.limitHoles(500);
                        layerFeatureMap->at(sourceLayerName)->push_back({featureContext, geometryHandler});
                    } catch (vtzero::geometry_exception &geometryException) {
                        LogError <<= "geometryException for tile " + std::to_string(tile.zoomIdentifier) + "/" + std::to_string(tile.x) + "/" + std::to_string(tile.y);
                        continue;
                    }
                }
                if (layerFeatureMap->at(sourceLayerName)->empty()) {
                    layerFeatureMap->erase(sourceLayerName);
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

    return layerFeatureMap;
}

void Tiled2dMapVectorSource::notifyTilesUpdates() {
    listener.message(MailboxDuplicationStrategy::replaceNewest, &Tiled2dMapVectorSourceListener::onTilesUpdated, sourceName, getCurrentTiles());
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
