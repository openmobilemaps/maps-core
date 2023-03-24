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

class Tiled2dMapVectorSource : public Tiled2dMapSource<djinni::DataRef, DataLoaderResult, Tiled2dMapVectorTileInfo::FeatureMap>  {
public:
    Tiled2dMapVectorSource(const MapConfig &mapConfig,
                           const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                           const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                           const std::shared_ptr<SchedulerInterface> &scheduler,
                           const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders,
                           const WeakActor<Tiled2dMapVectorSourceListener> &listener,
                           const std::unordered_set<std::string> &layersToDecode,
                           const std::string &sourceName,
                           float screenDensityPpi,
                           bool tileBasedRendering);

    std::unordered_set<Tiled2dMapVectorTileInfo> getCurrentTiles();

    virtual void pause() override;

    virtual void resume() override;

    virtual void notifyTilesUpdates() override;

    LoaderStatus getLoaderStatus(const DataLoaderResult &loaderResult) override;

    std::optional<std::string> getErrorCode(const DataLoaderResult &loaderResult) override;

protected:
    
    virtual void cancelLoad(Tiled2dMapTileInfo tile, size_t loaderIndex) override;
    
    virtual ::djinni::Future<DataLoaderResult> loadDataAsync(Tiled2dMapTileInfo tile, size_t loaderIndex) override;
    
    virtual Tiled2dMapVectorTileInfo::FeatureMap postLoadingTask(const DataLoaderResult &loadedData, const Tiled2dMapTileInfo &tile) override;

private:
    const std::vector<std::shared_ptr<::LoaderInterface>> loaders;
    const std::unordered_set<std::string> layersToDecode;
    
    const WeakActor<Tiled2dMapVectorSourceListener> listener;
    
    const std::string sourceName;

    const bool tileBasedRendering;
};
