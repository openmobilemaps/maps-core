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
#include "ShaderLineStyle.h"

class Tiled2dMapVectorLineTile
        : public Tiled2dMapVectorTile,
          public std::enable_shared_from_this<Tiled2dMapVectorLineTile> {
public:
    Tiled2dMapVectorLineTile(const std::weak_ptr<MapInterface> &mapInterface,
                                const Tiled2dMapVersionedTileInfo &tileInfo,
                                const WeakActor<Tiled2dMapVectorLayerTileCallbackInterface> &tileCallbackInterface,
                                const std::shared_ptr<LineVectorLayerDescription> &description,
                             const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                             const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager);

    void updateVectorLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                const Tiled2dMapVectorTileDataVector &tileData) override;

    void update() override;

    virtual std::vector<std::shared_ptr<RenderObjectInterface>> generateRenderObjects() override;

    virtual void clear() override;

    virtual void setup() override;

    virtual void setVectorTileData(const Tiled2dMapVectorTileDataVector &tileData) override;

    bool onClickConfirmed(const Vec2F &posScreen) override;

    bool performClick(const Coord &coord) override;

private:
    void addLines(const std::vector<std::vector<std::vector<std::tuple<std::vector<Coord>, int>>>> &styleIdLinesVector);

    void setupLines(const std::vector<std::shared_ptr<GraphicsObjectInterface>> &newLineGraphicsObjects);


    static const int maxNumLinePoints = std::numeric_limits<uint16_t>::max() / 4 + 1; // 4 vertices per line coord, only 2 at the start/end
#ifdef OPENMOBILEMAPS_GL
    static const int maxStylesPerGroup = 32;
#else
    static const int maxStylesPerGroup = 256;
#endif

    std::vector<std::shared_ptr<LineGroupShaderInterface>> shaders;

    std::vector<std::shared_ptr<LineGroup2dLayerObject>> lines;

    std::vector<std::vector<std::tuple<size_t, std::shared_ptr<FeatureContext>>>> featureGroups;

    std::vector<std::tuple<std::vector<std::vector<::Coord>>, std::shared_ptr<FeatureContext>>> hitDetection;

    UsedKeysCollection usedKeys;
    bool isStyleZoomDependant = true;
    bool isStyleStateDependant = true;
    std::optional<double> lastZoom = std::nullopt;
    std::optional<bool> lastInZoomRange = std::nullopt;

    std::vector<std::vector<ShaderLineStyle>> reusableLineStyles;
    std::unordered_map<size_t, std::pair<int, int>> styleHashToGroupMap;

    std::vector<std::shared_ptr<LineGroup2dLayerObject>> toClear;
};
