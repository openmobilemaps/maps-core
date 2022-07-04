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
#include "VectorMapDescription.h"
#include "FontLoaderInterface.h"
#include "PolygonMaskObject.h"
#include "Tiled2dMapVectorLayerReadyInterface.h"


class Tiled2dMapVectorLayer : public Tiled2dMapLayer, public Tiled2dMapVectorLayerInterface, public Tiled2dMapVectorLayerReadyInterface {
public:
    Tiled2dMapVectorLayer(const std::shared_ptr<VectorMapDescription> & mapDescription,
                          const std::shared_ptr<::LoaderInterface> & tileLoader,
                          const std::shared_ptr<::FontLoaderInterface> & fontLoader);

    virtual std::shared_ptr<::LayerInterface> asLayerInterface() override;

    virtual void update() override;

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() override;

    virtual void onAdded(const std::shared_ptr<::MapInterface> &mapInterface) override;

    virtual void onRemoved() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void onTilesUpdated() override;

    virtual void tileIsReady(const Tiled2dMapTileInfo &tile) override;

private:
    virtual void updateMaskObjects(const std::vector<const std::shared_ptr<MaskingObjectInterface>> &obsoleteMaskObjects);

    std::shared_ptr<FontLoaderInterface> fontLoader;

    std::shared_ptr<LoaderInterface> vectorTileLoader;
    std::shared_ptr<Tiled2dMapVectorSource> vectorTileSource;

    std::recursive_mutex updateMutex;
    std::unordered_set<Tiled2dMapVectorTileInfo> tileSet;

    std::recursive_mutex tilesReadyMutex;
    std::unordered_set<Tiled2dMapTileInfo> tilesReady;

    std::recursive_mutex tilesReadyCountMutex;
    std::unordered_map<Tiled2dMapTileInfo, int> tilesReadyCount;

    std::unordered_map<Tiled2dMapTileInfo, std::shared_ptr<PolygonMaskObject>> tileMaskMap;

    std::vector<std::shared_ptr<LayerInterface>> sublayers;
    std::unordered_map<std::string, std::vector<std::shared_ptr<Tiled2dMapVectorSubLayer>>> sourceLayerMap;
};


