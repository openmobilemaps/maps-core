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

#include "TiledDisplacedRasterSource.h"
#include "Tiled2dMapRasterLayer.h"


class TiledDisplacedRasterLayer : public Tiled2dMapRasterLayer {
public:
    TiledDisplacedRasterLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                              const std::shared_ptr<::Tiled2dMapLayerConfig> &elevationConfig,
                              const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders,
                              bool registerToTouchHandler = true);

    void onAdded(const std::shared_ptr<::MapInterface> &mapInterface, int32_t layerIndex) override;

    void setupTiles() override;

protected:
    const std::shared_ptr<Tiled2dMapLayerConfig> elevationConfig;

    Actor<TiledDisplacedRasterSource> rasterSource;
};
