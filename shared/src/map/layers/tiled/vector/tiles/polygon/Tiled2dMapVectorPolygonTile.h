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
#include "PolygonCoord.h"

class Tiled2dMapVectorPolygonTile
        : public Tiled2dMapVectorTile,
          public std::enable_shared_from_this<Tiled2dMapVectorPolygonTile> {
public:
    Tiled2dMapVectorPolygonTile(const std::weak_ptr<MapInterface> &mapInterface,
                                const Tiled2dMapTileInfo &tileInfo,
                                const WeakActor<Tiled2dMapVectorLayerTileCallbackInterface> &tileCallbackInterface,
                                const std::shared_ptr<PolygonVectorLayerDescription> &description);

    void updateVectorLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                const Tiled2dMapVectorTileDataVector &layerFeatures) override;

    void update() override;

    virtual std::vector<std::shared_ptr<RenderObjectInterface>> generateRenderObjects() override;

    virtual void clear() override;

    virtual void setup() override;

    virtual void setVectorTileData(const Tiled2dMapVectorTileDataVector &tileData) override;

    bool onClickConfirmed(const Vec2F &posScreen) override;

private:

    struct ObjectDescriptions {
        std::vector<std::tuple<std::vector<::Coord>, int>> vertices;
        std::vector<uint16_t> indices;
    };

    void addPolygons(const std::vector<ObjectDescriptions> &polygons);

    void setupPolygons(const std::vector<std::shared_ptr<GraphicsObjectInterface>> &newPolygonObjects);


    std::shared_ptr<PolygonGroupShaderInterface> shader;

    std::vector<std::shared_ptr<PolygonGroup2dLayerObject>> polygons;
    std::vector<std::tuple<size_t, FeatureContext>> featureGroups;
    std::unordered_set<std::string> usedKeys;

    std::unordered_map<Tiled2dMapTileInfo, std::vector<std::tuple<PolygonCoord, FeatureContext>>> hitDetectionPolygonMap;
};
