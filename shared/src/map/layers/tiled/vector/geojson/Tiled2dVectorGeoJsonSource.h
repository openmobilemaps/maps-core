/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */


#pragma once

#include "Tiled2dMapSource.h"
#include "DataLoaderResult.h"
#include "LoaderInterface.h"
#include "Tiled2dMapVectorTileInfo.h"
#include "Tiled2dMapVectorSourceListener.h"
#include <unordered_map>
#include <vector>
#include "DataRef.hpp"
#include "geojsonvt.hpp"
#include "Tiled2dMapVectorSource.h"

class Tiled2dVectorGeoJsonSource : public Tiled2dMapVectorSource  {
public:
    Tiled2dVectorGeoJsonSource(const MapConfig &mapConfig,
                               const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                               const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                               const std::shared_ptr<SchedulerInterface> &scheduler,
                               const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders,
                               const WeakActor<Tiled2dMapVectorSourceListener> &listener,
                               const std::unordered_set<std::string> &layersToDecode,
                               const std::string &sourceName,
                               float screenDensityPpi,
                               std::shared_ptr<GeoJSONVT> geoJson) :
    Tiled2dMapVectorSource(mapConfig, layerConfig, conversionHelper, scheduler, tileLoaders, listener, layersToDecode, sourceName, screenDensityPpi),
    geoJson(geoJson) {}

    std::unordered_set<Tiled2dMapVectorTileInfo> getCurrentTiles() {
        std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos;
        std::transform(currentTiles.begin(), currentTiles.end(), std::inserter(currentTileInfos, currentTileInfos.end()), [](const auto& tilePair) {
                const auto& [tileInfo, tileWrapper] = tilePair;
                return Tiled2dMapVectorTileInfo(std::move(tileInfo), std::move(tileWrapper.result), std::move(tileWrapper.masks), std::move(tileWrapper.state));
            }
        );
        return currentTileInfos;
    }

    virtual void notifyTilesUpdates() override {
        listener.message(MailboxDuplicationStrategy::replaceNewest, &Tiled2dMapVectorSourceListener::onTilesUpdated, sourceName, getCurrentTiles());
    };
protected:

    virtual void cancelLoad(Tiled2dMapTileInfo tile, size_t loaderIndex) override {};

    virtual ::djinni::Future<std::shared_ptr<DataLoaderResult>> loadDataAsync(Tiled2dMapTileInfo tile, size_t loaderIndex) override {
        auto promise = ::djinni::Promise<std::shared_ptr<DataLoaderResult>>();
        promise.setValue(std::make_shared<DataLoaderResult>(std::nullopt, std::nullopt, LoaderStatus::OK, std::nullopt));
        return promise.getFuture();
    };

    virtual bool hasExpensivePostLoadingTask() override { return false; };

    virtual Tiled2dMapVectorTileInfo::FeatureMap postLoadingTask(const std::shared_ptr<DataLoaderResult> &loadedData, const Tiled2dMapTileInfo &tile) override {
        const auto &geoJsonTile = geoJson->getTile(tile.zoomIdentifier, tile.x, tile.y);
        Tiled2dMapVectorTileInfo::FeatureMap featureMap = std::make_shared<std::unordered_map<std::string, std::shared_ptr<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>>>>();
        std::shared_ptr<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>> features = std::make_shared<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>>();
        for (const auto &feature: geoJsonTile.features) {
            features->push_back({feature->featureContext, std::make_shared<VectorTileGeometryHandler>(feature, tile.bounds, conversionHelper)});
        }
        featureMap->insert({
            "", features
        });
        return featureMap;
    }

private:
    const std::shared_ptr<GeoJSONVT> geoJson;
};
