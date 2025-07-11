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
#include "StringInterner.h"
#include "Tiled2dMapVectorTileInfo.h"
#include "Tiled2dMapVectorSourceListener.h"
#include <vector>

class Tiled2dMapVectorLayer;

class Tiled2dMapVectorSource : public Tiled2dMapSource<std::shared_ptr<DataLoaderResult>,
                                                       Tiled2dMapVectorTileInfo::FeatureMap> {
public:
    Tiled2dMapVectorSource(const MapConfig &mapConfig,
                           const std::weak_ptr<Tiled2dMapVectorLayer> &vectorLayer,
                           const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                           const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                           const std::shared_ptr<SchedulerInterface> &scheduler,
                           const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders,
                           const WeakActor<Tiled2dMapVectorSourceListener> &listener,
                           const std::unordered_set<std::string> &layersToDecode,
                           const std::string &sourceName,
                           float screenDensityPpi,
                           std::string layerName);

    VectorSet<Tiled2dMapVectorTileInfo> getCurrentTiles();

    virtual void pause() override;

    virtual void resume() override;

    virtual void notifyTilesUpdates() override;

    std::string getSourceName();
protected:
    
    virtual void cancelLoad(Tiled2dMapTileInfo tile, size_t loaderIndex) override;
    
    virtual ::djinni::Future<std::shared_ptr<DataLoaderResult>> loadDataAsync(Tiled2dMapTileInfo tile, size_t loaderIndex) override;

    virtual bool hasExpensivePostLoadingTask() override;
    
    virtual Tiled2dMapVectorTileInfo::FeatureMap postLoadingTask(std::shared_ptr<DataLoaderResult> loadedData, Tiled2dMapTileInfo tile) override;

    const std::vector<std::shared_ptr<::LoaderInterface>> loaders;
    const std::unordered_set<std::string> layersToDecode;
    
    const WeakActor<Tiled2dMapVectorSourceListener> listener;
    
    const std::string sourceName;

    std::weak_ptr<Tiled2dMapVectorLayer> vectorLayer;
};
