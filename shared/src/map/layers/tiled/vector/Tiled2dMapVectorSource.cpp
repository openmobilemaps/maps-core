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
#include "Logger.h"
#include "PerformanceLogger.h"
#include "Tiled2dMapVectorLayer.h"
#include "Tiled2dMapVectorTileInfo.h"
#include "vtzero/vector_tile.hpp"

Tiled2dMapVectorSource::Tiled2dMapVectorSource(const MapConfig &mapConfig,
                                               const std::weak_ptr<Tiled2dMapVectorLayer> &vectorLayer,
                                               const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                               const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                               const std::shared_ptr<SchedulerInterface> &scheduler,
                                               const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders,
                                               const WeakActor<Tiled2dMapVectorSourceListener> &listener,
                                               const std::unordered_set<std::string> &layersToDecode,
                                               const std::string &sourceName,
                                               float screenDensityPpi,
                                               std::string layerName)
        : Tiled2dMapSource<std::shared_ptr<DataLoaderResult>, Tiled2dMapVectorTileInfo::FeatureMap>(mapConfig, layerConfig, conversionHelper, scheduler, screenDensityPpi, tileLoaders.size(), layerName),
loaders(tileLoaders), layersToDecode(layersToDecode), listener(listener), sourceName(sourceName), vectorLayer(vectorLayer) {}

::djinni::Future<std::shared_ptr<DataLoaderResult>> Tiled2dMapVectorSource::loadDataAsync(Tiled2dMapTileInfo tile, size_t loaderIndex) {
    {
        std::lock_guard<std::mutex> lock_guard(loadingTilesMutex);
        loadingTiles.insert(tile);
    }
    auto const url = layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier);
    auto promise = std::make_shared<::djinni::Promise<std::shared_ptr<DataLoaderResult>>>();
    loaders[loaderIndex]->loadDataAsync(url, std::nullopt).then([promise](::djinni::Future<::DataLoaderResult> result) {
        promise->setValue(std::make_shared<DataLoaderResult>(result.get()));
    });
    return promise->getFuture();
}

void Tiled2dMapVectorSource::cancelLoad(Tiled2dMapTileInfo tile, size_t loaderIndex) {
    {
        std::lock_guard<std::mutex> lock_guard(loadingTilesMutex);
        loadingTiles.erase(tile);
    }
    auto const url = layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier);
    loaders[loaderIndex]->cancel(url);
}

bool Tiled2dMapVectorSource::hasExpensivePostLoadingTask() {
    return true;
}

static void internAllLayerKeys(const vtzero::layer &layer,
                               StringInterner &stringTable,
                               std::vector<std::string> &outKeys,
                               std::vector<InternedString> &outInternedKeys)
{
    outKeys.clear();
    outKeys.reserve(layer.key_table_size());
    for (auto &k : layer.key_table()) {
        outKeys.push_back(std::string{k});
    }
    outInternedKeys.clear();
    outInternedKeys.reserve(outKeys.size());
    stringTable.add(outKeys.begin(), outKeys.end(), std::back_inserter(outInternedKeys));
}

static std::shared_ptr<FeatureContext> convertToFeatureContext(const vtzero::feature &feature, const vtzero::layer &layer,
                                                               std::vector<InternedString> internedLayerKeys) {
    FeatureContext::mapType propertiesMap;
    propertiesMap.reserve(feature.num_properties());
    feature.for_each_property_indexes([&](vtzero::index_value_pair property) {
        auto key = internedLayerKeys[property.key().value()];
        auto value = vtzero::convert_property_value<ValueVariant, property_value_mapping>(layer.value(property.value().value())); // value! gaah
        propertiesMap.emplace_back(key, std::move(value));
        return true;
    });

    uint64_t identifier;
    if (feature.has_id()) {
        identifier = feature.id();
    } else {
        size_t hash = 0;
        for (auto const &[key, val] : propertiesMap) {
            std::hash_combine(hash, std::hash<FeatureContext::valueType>{}(val));
        }
        identifier = hash;
    }
    return std::make_shared<FeatureContext>(feature.geometry_type(), std::move(propertiesMap), identifier);
}

Tiled2dMapVectorTileInfo::FeatureMap Tiled2dMapVectorSource::postLoadingTask(std::shared_ptr<DataLoaderResult> loadedData, Tiled2dMapTileInfo tile) {
    PERF_LOG_START(sourceName + "_postLoadingTask");
    auto layerFeatureMap = std::make_shared<std::unordered_map<std::string, std::shared_ptr<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>>>>();
    
    auto strongVectorLayer = vectorLayer.lock();
    if (!strongVectorLayer) {
        return layerFeatureMap;
    }
    StringInterner &stringTable = strongVectorLayer->getStringInterner();

    if (!loadedData->data.has_value()) {
        LogError <<= "postLoadingTask, but data has no value for " + layerConfig->getLayerName() + ": " + std::to_string(tile.zoomIdentifier) + "/" +
        std::to_string(tile.x) + "/" + std::to_string(tile.y);
        return layerFeatureMap;
    }

    try {
        vtzero::vector_tile tileData((char*)loadedData->data->buf(), loadedData->data->len());

        std::vector<std::string> layerKeys;
        std::vector<InternedString> internedLayerKeys;
        mapbox::detail::Earcut<uint16_t> earcutter;
        while (auto layer = tileData.next_layer()) {
            std::string sourceLayerName = std::string(layer.name());
            if ((layersToDecode.empty() || layersToDecode.count(sourceLayerName) > 0) && !layer.empty()) {
                int extent = (int) layer.extent();
                layerFeatureMap->emplace(sourceLayerName, std::make_shared<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>>());
                layerFeatureMap->at(sourceLayerName)->reserve(layer.num_features());

                internAllLayerKeys(layer, stringTable, layerKeys, internedLayerKeys);
                while (const auto &feature = layer.next_feature()) {

                    {
                        std::lock_guard<std::mutex> lock_guard(loadingTilesMutex);
                        if (loadingTiles.find(tile) == loadingTiles.end()) {
                            return std::make_shared<std::unordered_map<std::string, std::shared_ptr<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>>>>();
                        }
                    }

                    auto const featureContext = convertToFeatureContext(feature, layer, internedLayerKeys);
                    PERF_LOG_START(sourceLayerName + "_decode");
                    try {
                        std::shared_ptr<VectorTileGeometryHandler> geometryHandler = std::make_shared<VectorTileGeometryHandler>(tile.bounds, extent, layerConfig->getVectorSettings(), conversionHelper);
                        vtzero::decode_geometry(feature.geometry(), *geometryHandler);
                        size_t polygonCount = geometryHandler->beginTriangulatePolygons();
                        for (size_t i = 0; i < polygonCount; i++) {
                            {
                                std::lock_guard<std::mutex> lock_guard(loadingTilesMutex);
                                if (loadingTiles.find(tile) == loadingTiles.end()) {
                                    return std::make_shared<std::unordered_map<std::string, std::shared_ptr<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>>>>();
                                }
                            }
                            geometryHandler->triangulatePolygons(i, earcutter);
                        }

                        geometryHandler->endTringulatePolygons();
                        layerFeatureMap->at(sourceLayerName)->push_back({featureContext, geometryHandler});
                    } catch (const vtzero::geometry_exception &geometryException) {
                        LogError <<= "geometryException for tile " + std::to_string(tile.zoomIdentifier) + "/" + std::to_string(tile.x) + "/" + std::to_string(tile.y);
                        continue;
                    }
                    PERF_LOG_END(sourceLayerName + "_decode");
                }
                if (layerFeatureMap->at(sourceLayerName)->empty()) {
                    layerFeatureMap->erase(sourceLayerName);
                }
            }
        }
    }
    catch (const protozero::invalid_tag_exception &tagException) {
        LogError <<= "Invalid tag exception for tile " + std::to_string(tile.zoomIdentifier) + "/" +
        std::to_string(tile.x) + "/" + std::to_string(tile.y);
    }
    catch (const protozero::unknown_pbf_wire_type_exception &typeException) {
        LogError <<= "Unknown wire type exception for tile " + std::to_string(tile.zoomIdentifier) + "/" +
        std::to_string(tile.x) + "/" + std::to_string(tile.y);
    }
    PERF_LOG_START(sourceName + "_postLoadingTask");

    {
        std::lock_guard<std::mutex> lock_guard(loadingTilesMutex);
        loadingTiles.erase(tile);
    }

    return layerFeatureMap;
}

void Tiled2dMapVectorSource::notifyTilesUpdates() {
    listener.message(MFN(&Tiled2dMapVectorSourceListener::onTilesUpdated), sourceName, getCurrentTiles());
}

VectorSet<Tiled2dMapVectorTileInfo> Tiled2dMapVectorSource::getCurrentTiles() {
    VectorSet<Tiled2dMapVectorTileInfo> currentTileInfos;
    currentTileInfos.reserve(currentTiles.size() + outdatedTiles.size());
    
    for (auto it = currentTiles.begin(); it != currentTiles.end(); it++) {
        const auto& [tileInfo, tileWrapper] = *it;
        currentTileInfos.insert(Tiled2dMapVectorTileInfo(Tiled2dMapVersionedTileInfo(std::move(tileInfo), (size_t)tileWrapper.result.get()), std::move(tileWrapper.result), std::move(tileWrapper.masks), std::move(tileWrapper.state)));
    }
    for (auto it = outdatedTiles.begin(); it != outdatedTiles.end(); it++) {
        const auto& [tileInfo, tileWrapper] = *it;
        currentTileInfos.insert(Tiled2dMapVectorTileInfo(Tiled2dMapVersionedTileInfo(std::move(tileInfo), (size_t)tileWrapper.result.get()), std::move(tileWrapper.result), std::move(tileWrapper.masks), std::move(tileWrapper.state)));
    }
    return currentTileInfos;
}

void Tiled2dMapVectorSource::pause() {
    // TODO: Stop loading tiles
}

void Tiled2dMapVectorSource::resume() {
    // TODO: Reload textures of current tiles
}

std::string Tiled2dMapVectorSource::getSourceName() {
    return sourceName;
}

#include "Tiled2dMapSourceImpl.h"
template class Tiled2dMapSource<std::shared_ptr<DataLoaderResult>, Tiled2dMapVectorTileInfo::FeatureMap>;

