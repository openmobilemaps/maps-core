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


struct IntermediateResult final {
    std::unordered_map<std::string, DataLoaderResult> results;
    LoaderStatus status;
    std::optional<std::string> errorCode;

    IntermediateResult(std::unordered_map<std::string, DataLoaderResult> results_,
                     LoaderStatus status_,
                     std::optional<std::string> errorCode_)
    : results(std::move(results_))
    , status(std::move(status_))
    , errorCode(std::move(errorCode_))
    {}
};

struct Tiled2dMapVectorSourceTileState final {
    ::djinni::Promise<IntermediateResult> promise;
    std::unordered_map<std::string, DataLoaderResult> results;
};


using FinalResult = std::unordered_map<std::string, std::shared_ptr<std::unordered_map<std::string, std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>>>>>;

class Tiled2dMapVectorSource : public Tiled2dMapSource<djinni::DataRef, IntermediateResult, FinalResult>  {
public:
    Tiled2dMapVectorSource(const MapConfig &mapConfig,
                           const std::unordered_map<std::string, std::shared_ptr<Tiled2dMapLayerConfig>> &layerConfigs,
                           const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                           const std::shared_ptr<SchedulerInterface> &scheduler,
                           const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders,
                           const WeakActor<Tiled2dMapVectorSourceListener> &listener,
                           const std::unordered_map<std::string, std::unordered_set<std::string>> &layersToDecode,
                           float screenDensityPpi);

    std::unordered_set<Tiled2dMapVectorTileInfo> getCurrentTiles();

    virtual void pause() override;

    virtual void resume() override;

    virtual void notifyTilesUpdates() override;
protected:
    
    virtual void cancelLoad(Tiled2dMapTileInfo tile, size_t loaderIndex) override;
    
    virtual ::djinni::Future<IntermediateResult> loadDataAsync(Tiled2dMapTileInfo tile, size_t loaderIndex) override;
    
    virtual FinalResult postLoadingTask(const IntermediateResult &loadedData, const Tiled2dMapTileInfo &tile) override;

    virtual std::tuple<LoaderStatus, std::optional<std::string>> mergeLoaderStatus(const Tiled2dMapVectorSourceTileState &tileStates);

private:
    const std::vector<std::shared_ptr<::LoaderInterface>> loaders;
    const std::unordered_map<std::string, std::unordered_set<std::string>> layersToDecode;
    const std::unordered_map<std::string, std::shared_ptr<Tiled2dMapLayerConfig>> layerConfigs;
    
    std::recursive_mutex loadingStateMutex;
    std::unordered_map<Tiled2dMapTileInfo, Tiled2dMapVectorSourceTileState> loadingStates;
    
    const WeakActor<Tiled2dMapVectorSourceListener> listener;
};
