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

#include "Tiled2dMapRasterSource.h"
#include <mutex>
#include <unordered_map>

class TiledDisplacedRasterSource : public Tiled2dMapRasterSource {
public:
    TiledDisplacedRasterSource(const MapConfig &mapConfig,
                               const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                               const std::shared_ptr<Tiled2dMapLayerConfig> &elevationConfig,
                               const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                               const std::shared_ptr<SchedulerInterface> &scheduler,
                               const std::vector<std::shared_ptr<::LoaderInterface>> & loaders,
                               const WeakActor<Tiled2dMapRasterSourceListener> &listener,
                               float screenDensityPpi,
                               std::string layerName);

protected:
    void notifyTilesUpdates() override;

    void cancelLoad(Tiled2dMapTileInfo tile, size_t loaderIndex) override;

    ::djinni::Future<std::shared_ptr<TextureLoaderResult>> loadDataAsync(Tiled2dMapTileInfo tile, size_t loaderIndex) override;

protected:
    const std::shared_ptr<Tiled2dMapLayerConfig> elevationConfig;
    std::unordered_map<Tiled2dMapTileInfo, std::shared_ptr<TextureHolderInterface>> elevationTextureHolders;
    std::mutex elevationTextureHoldersMutex;
};
