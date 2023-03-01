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

#include "Tiled2dMapVectorTile.h"
#include "MapInterface.h"
#include "PolygonVectorLayerDescription.h"
#include "PolygonGroup2dLayerObject.h"

class Tiled2dMapVectorPolygonTile : public Tiled2dMapVectorTile, public std::enable_shared_from_this<Tiled2dMapVectorPolygonTile> {
public:
    using Tiled2dMapVectorTile::Tiled2dMapVectorTile;

    Tiled2dMapVectorPolygonTile(const std::weak_ptr<MapInterface> &mapInterface,
                                const Tiled2dMapTileInfo &tileInfo,
                                const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                const PolygonVectorLayerDescription &description);

    void update() override;

    virtual std::vector<std::shared_ptr<RenderPassInterface>> buildRenderPasses() override;

    virtual void clear() override;

    virtual void setup() override;

    virtual void setScissorRect(const std::optional<::RectI> &scissorRect) override;

    virtual void setTileData(const std::shared_ptr<MaskingObjectInterface> &tileMask,
                             const std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>> &layerFeatures) override;

    virtual void updateTileMask(const std::shared_ptr<MaskingObjectInterface> &tileMask) override;

protected:
    virtual void preGenerateRenderPasses() override;

private:
    void addPolygons(const std::vector<std::tuple<std::vector<std::tuple<std::vector<Coord>, int>>, std::vector<int32_t>>> &polygons);

    void setupPolygons(const std::vector<std::shared_ptr<GraphicsObjectInterface>> &newPolygonObjects);

    PolygonVectorLayerDescription description;

    std::shared_ptr<PolygonGroupShaderInterface> shader;

    std::vector<std::shared_ptr<PolygonGroup2dLayerObject>> polygons;
    std::vector<std::tuple<size_t, FeatureContext>> featureGroups;
    std::unordered_set<std::string> usedKeys;

    std::shared_ptr<MaskingObjectInterface> tileMask;
    std::optional<::RectI> scissorRect = std::nullopt;
    std::vector<std::shared_ptr<RenderPassInterface>> renderPasses;

    float alpha = 1.0;
};
