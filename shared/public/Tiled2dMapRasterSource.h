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

#include "LoaderInterface.h"
#include "MapConfig.h"
#include "TextureLoaderResult.h"
#include "Tiled2dMapRasterTileInfo.h"
#include "Tiled2dMapSource.h"
#include "Tiled2dMapRasterSourceListener.h"

class Tiled2dMapRasterSource
    : public Tiled2dMapSource<
TextureHolderInterface,
std::pair<TextureLoaderResult, std::optional<TextureLoaderResult>>,
std::pair<std::shared_ptr<::TextureHolderInterface>, std::shared_ptr<::TextureHolderInterface>>
> {
  public:
    Tiled2dMapRasterSource(const MapConfig &mapConfig,
                           const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                           const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                           const std::shared_ptr<SchedulerInterface> &scheduler,
                           const std::vector<std::shared_ptr<::LoaderInterface>> & loaders,
                           const WeakActor<Tiled2dMapRasterSourceListener> &listener,
                           float screenDensityPpi,
                           const std::shared_ptr<Tiled2dMapLayerConfig> &heightLayerConfig);

    std::unordered_set<Tiled2dMapRasterTileInfo> getCurrentTiles();

    virtual void notifyTilesUpdates() override;

    LoaderStatus getLoaderStatus(const std::pair<TextureLoaderResult, std::optional<TextureLoaderResult>> &loaderResult) override;
    std::optional<std::string> getErrorCode(const std::pair<TextureLoaderResult, std::optional<TextureLoaderResult>> &loaderResult) override;
        
  protected:
    virtual void cancelLoad(Tiled2dMapTileInfo tile, size_t loaderIndex) override;
        
    virtual ::djinni::Future<std::pair<TextureLoaderResult, std::optional<TextureLoaderResult>>> loadDataAsync(Tiled2dMapTileInfo tile, size_t loaderIndex) override;

    virtual std::pair<std::shared_ptr<::TextureHolderInterface>, std::shared_ptr<::TextureHolderInterface>> postLoadingTask(const std::pair<TextureLoaderResult, std::optional<TextureLoaderResult>> &loadedData,
                                                                      const Tiled2dMapTileInfo &tile) override;


  private:
    const std::vector<std::shared_ptr<::LoaderInterface>> loaders;
        
    const WeakActor<Tiled2dMapRasterSourceListener> rasterLayerActor;

    const std::shared_ptr<Tiled2dMapLayerConfig> heightLayerConfig;
};
