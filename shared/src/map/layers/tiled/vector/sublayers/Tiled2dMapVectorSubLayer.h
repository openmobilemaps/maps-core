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


#include <mutex>
#include <unordered_map>
#include <vtzero/vector_tile.hpp>
#include "LayerInterface.h"
#include "Tiled2dMapVectorTileInfo.h"
#include "PolygonInfo.h"
#include "Polygon2dLayerObject.h"

class Tiled2dMapVectorSubLayer : public LayerInterface, public std::enable_shared_from_this<Tiled2dMapVectorSubLayer> {
public:
    Tiled2dMapVectorSubLayer(const Color &fillColor);

    virtual void update() override;

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() override;

    void preGenerateRenderPasses();

    virtual void onAdded(const std::shared_ptr<MapInterface> & mapInterface) override;

    virtual void onRemoved() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void hide() override;

    virtual void show() override;

    void updateTileData(const Tiled2dMapVectorTileInfo &tileInfo, vtzero::layer data);

    void clearTileData(const Tiled2dMapVectorTileInfo &tileInfo);

protected:
    void addPolygons(const Tiled2dMapVectorTileInfo &tileInfo, const std::vector<const PolygonInfo> &polygons);

private:
    std::shared_ptr<MapInterface> mapInterface;

    Color fillColor;

    std::recursive_mutex updateMutex;
    std::unordered_map<Tiled2dMapVectorTileInfo, std::vector<PolygonInfo>> tilePolygonMap;
    std::unordered_map<std::string, std::shared_ptr<Polygon2dLayerObject>> polygonObjectMap;
    // TODO: add line and other graphic-primitives support

    std::vector<std::shared_ptr<::RenderPassInterface>> renderPasses;
};
