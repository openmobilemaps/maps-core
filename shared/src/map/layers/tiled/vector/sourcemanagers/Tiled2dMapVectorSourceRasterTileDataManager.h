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

#include "Tiled2dMapVectorSourceTileDataManager.h"
#include "Tiled2dMapRasterSource.h"

class Tiled2dMapVectorSourceRasterTileDataManager : public Tiled2dMapVectorSourceTileDataManager {
public:
    Tiled2dMapVectorSourceRasterTileDataManager(const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                                const std::shared_ptr<VectorMapDescription> &mapDescription,
                                                const std::string &source,
                                                const WeakActor<Tiled2dMapRasterSource> &rasterSource);

    void onRasterTilesUpdated(const std::string &layerName, std::unordered_set<Tiled2dMapRasterTileInfo> currentTileInfos) override;

protected:
    void onTileCompletelyReady(const Tiled2dMapTileInfo tileInfo) override;

private:
    const WeakActor<Tiled2dMapRasterSource> rasterSource;
};
