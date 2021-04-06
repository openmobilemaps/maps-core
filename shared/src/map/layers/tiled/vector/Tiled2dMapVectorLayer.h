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

#include "Tiled2dMapLayer.h"
#include "Tiled2dMapVectorLayerInterface.h"
#include "Tiled2dMapVectorSource.h"
#include "Tiled2dMapVectorSubLayer.h"

class Tiled2dMapVectorLayer : public Tiled2dMapLayer, public Tiled2dMapVectorLayerInterface {
public:
    Tiled2dMapVectorLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> & layerConfig, const std::shared_ptr<::TileLoaderInterface> & tileLoader);

    virtual std::shared_ptr<::LayerInterface> asLayerInterface() override;

    virtual void update() override;

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() override;

    virtual void onAdded(const std::shared_ptr<::MapInterface> &mapInterface) override;

    virtual void onRemoved() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void onTilesUpdated() override;

private:
    std::shared_ptr<TileLoaderInterface> vectorTileLoader;
    std::shared_ptr<Tiled2dMapVectorSource> vectorTileSource;

    std::recursive_mutex updateMutex;
    std::unordered_set<Tiled2dMapVectorTileInfo> tileSet;

    std::shared_ptr<Tiled2dMapVectorSubLayer> sublayer; // TODO: generate layers according to style json
};


