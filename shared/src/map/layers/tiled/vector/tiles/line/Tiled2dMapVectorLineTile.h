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
#include "LineVectorLayerDescription.h"
#include "LineGroup2dLayerObject.h"

class Tiled2dMapVectorLineTile
        : public Tiled2dMapVectorTile,
          public std::enable_shared_from_this<Tiled2dMapVectorLineTile> {
public:
    Tiled2dMapVectorLineTile(const std::weak_ptr<MapInterface> &mapInterface,
                                const Tiled2dMapTileInfo &tileInfo,
                                const WeakActor<Tiled2dMapVectorLayerTileCallbackInterface> &tileCallbackInterface,
                                const std::shared_ptr<LineVectorLayerDescription> &description);

    void updateVectorLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                const Tiled2dMapVectorTileDataVector &tileData) override;

    void update() override;

    virtual std::vector<std::shared_ptr<RenderObjectInterface>> generateRenderObjects() override;

    virtual void clear() override;

    virtual void setup() override;

    virtual void setVectorTileData(const Tiled2dMapVectorTileDataVector &tileData) override;

    bool onClickConfirmed(const Vec2F &posScreen) override;

private:
    void addLines(const std::unordered_map<int, std::vector<std::vector<std::tuple<std::vector<Coord>, int>>>> &styleIdLinesMap);

    void setupLines(const std::vector<std::shared_ptr<GraphicsObjectInterface>> &newLineGraphicsObjects,
                    const std::vector<std::shared_ptr<GraphicsObjectInterface>> &oldLineGraphicsObjects);


    static const int maxNumLinePoints = std::numeric_limits<uint16_t>::max() / 4 + 1; // 4 vertices per line coord, only 2 at the start/end
    static const int maxStylesPerGroup = 48;

    std::vector<std::shared_ptr<LineGroupShaderInterface>> shaders;

    std::vector<std::shared_ptr<LineGroup2dLayerObject>> lines;

    std::vector<std::vector<std::tuple<size_t, FeatureContext>>> featureGroups;

    std::vector<std::tuple<std::vector<std::vector<::Coord>>, FeatureContext>> hitDetection;

    std::unordered_set<std::string> usedKeys;

    std::vector<std::vector<LineStyle>> reusableLineStyles;
    std::unordered_map<size_t, std::pair<int, int>> styleHashToGroupMap;
};
