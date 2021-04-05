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
#include "VectorTileLoaderResult.h"
#include "TileLoaderInterface.h"
#include "VectorTileHolderInterface.h"
#include "Tiled2dMapVectorTileInfo.h"

class Tiled2dMapVectorSource : public Tiled2dMapSource<VectorTileHolderInterface, VectorTileLoaderResult>  {
public:
    Tiled2dMapVectorSource(const MapConfig &mapConfig, const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                           const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                           const std::shared_ptr<SchedulerInterface> &scheduler,
                           const std::shared_ptr<::TileLoaderInterface> & tileLoader,
                           const std::shared_ptr<Tiled2dMapSourceListenerInterface> &listener);

    std::unordered_set<Tiled2dMapVectorTileInfo> getCurrentTiles();

    virtual void pause() override;

    virtual void resume() override;

protected:
    virtual VectorTileLoaderResult loadTile(Tiled2dMapTileInfo tile) override;

private:
    const std::shared_ptr<TileLoaderInterface> loader;
};
