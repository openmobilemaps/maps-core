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
#include "VectorMapSourceDescription.h"
#include "Tiled2dMapVectorGeoJSONLayerConfig.h"

class Tiled2dVectorGeoJsonSource : public Tiled2dMapVectorSource, public GeoJSONTileDelegate  {
public:
    Tiled2dVectorGeoJsonSource(const std::shared_ptr<::MapCameraInterface> &camera,
                               const MapConfig &mapConfig,
                               const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                               const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                               const std::shared_ptr<SchedulerInterface> &scheduler,
                               const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders,
                               const WeakActor<Tiled2dMapVectorSourceListener> &listener,
                               const std::unordered_set<std::string> &layersToDecode,
                               const std::string &sourceName,
                               float screenDensityPpi,
                               std::shared_ptr<GeoJSONVTInterface> geoJson,
                               std::string layerName) :
    Tiled2dMapVectorSource(mapConfig, layerConfig, conversionHelper, scheduler, tileLoaders, listener, layersToDecode, sourceName, screenDensityPpi, layerName),
    geoJson(geoJson), camera(camera) {}

    VectorSet<Tiled2dMapVectorTileInfo> getCurrentTiles() {
        VectorSet<Tiled2dMapVectorTileInfo> currentTileInfos;
        currentTileInfos.reserve(currentTiles.size() + outdatedTiles.size());

        for (auto it = currentTiles.begin(); it != currentTiles.end(); it++) {
            const auto& [tileInfo, tileWrapper] = *it;
            currentTileInfos.insert(Tiled2dMapVectorTileInfo(Tiled2dMapVersionedTileInfo(std::move(tileInfo), (size_t)tileWrapper.result.get()), std::move(tileWrapper.result), std::move(tileWrapper.masks), std::move(tileWrapper.state)));
        }
        for (auto it = outdatedTiles.begin(); it != outdatedTiles.end(); it++) {
            const auto& [tileInfo, tileWrapper] = *it;
            currentTileInfos.insert(Tiled2dMapVectorTileInfo(Tiled2dMapVersionedTileInfo(std::move(tileInfo), (size_t)tileWrapper.result.get()), std::move(tileWrapper.result), std::move(tileWrapper.masks), std::move(tileWrapper.state)));
        };
        return currentTileInfos;
    }

    virtual void notifyTilesUpdates() override {
        listener.message(MFN(&Tiled2dMapVectorSourceListener::onTilesUpdated), sourceName, getCurrentTiles());
    };

    void didLoad(uint8_t maxZoom) override {
        zoomLevelInfos = layerConfig->getZoomLevelInfos();
        if (auto camera = this->camera.lock()) {
            auto bounds = camera->getVisibleRect();
            auto zoom = camera->getZoom();
            // there is no concept of curT for geoJSON, therefore we just set it to 0
            onVisibleBoundsChanged(bounds, 0, zoom);
        }
    }

    void failedToLoad() override {
        loadFailed = true;
    }

    virtual ::LayerReadyState isReadyToRenderOffscreen() override {
        if (loadFailed) {
            return LayerReadyState::ERROR;
        }
        if (geoJson->isLoaded()) {
            return Tiled2dMapVectorSource::isReadyToRenderOffscreen();
        }
        return LayerReadyState::NOT_READY;
    }

protected:

    virtual void cancelLoad(Tiled2dMapTileInfo tile, size_t loaderIndex) override {};

    virtual ::djinni::Future<std::shared_ptr<DataLoaderResult>> loadDataAsync(Tiled2dMapTileInfo tile, size_t loaderIndex) override {
        auto promise = std::make_shared<::djinni::Promise<std::shared_ptr<DataLoaderResult>>>();
        geoJson->waitIfNotLoaded(promise);
        return promise->getFuture();
    };

    virtual bool hasExpensivePostLoadingTask() override { return false; };

    virtual Tiled2dMapVectorTileInfo::FeatureMap postLoadingTask(std::shared_ptr<DataLoaderResult> loadedData, Tiled2dMapTileInfo tile) override {
        const auto &geoJsonTile = geoJson->getTile(tile.zoomIdentifier, tile.x, tile.y);
        Tiled2dMapVectorTileInfo::FeatureMap featureMap = std::make_shared<std::unordered_map<std::string, std::shared_ptr<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>>>>();
        std::shared_ptr<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>> features = std::make_shared<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>>();
        for (const auto &feature: geoJsonTile.getFeatures()) {
            features->push_back({feature->featureContext, std::make_shared<VectorTileGeometryHandler>(feature, tile.bounds, conversionHelper)});
        }
        featureMap->insert({
            "", features
        });
        return featureMap;
    }

private:
    const std::shared_ptr<GeoJSONVTInterface> geoJson;

    bool loadFailed = false;
    const std::weak_ptr<::MapCameraInterface> camera;

};
