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

#include "Tiled2dMapVectorSubLayer.h"
#include "LineVectorLayerDescription.h"
#include "LineGroup2dLayerObject.h"
#include "Line2dLayerObject.h"
#include "LineInfo.h"
#include "LineGroupShaderInterface.h"


class Tiled2dMapVectorLineSubLayer : public Tiled2dMapVectorSubLayer,
                                     public std::enable_shared_from_this<Tiled2dMapVectorLineSubLayer> {
public:
    Tiled2dMapVectorLineSubLayer(const std::shared_ptr<LineVectorLayerDescription> &description);

    ~Tiled2dMapVectorLineSubLayer() {}

    virtual void update() override;

    virtual void onAdded(const std::shared_ptr<MapInterface> &mapInterface) override;

    virtual void onRemoved() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void hide() override;

    virtual void show() override;

    virtual void updateTileData(const Tiled2dMapTileInfo &tileInfo, const std::shared_ptr<MaskingObjectInterface> &tileMask,
                                const std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>> &layerFeatures) override;

    void updateTileMask(const Tiled2dMapTileInfo &tileInfo, const std::shared_ptr <MaskingObjectInterface> &tileMask) override;

    virtual void clearTileData(const Tiled2dMapTileInfo &tileInfo) override;

    virtual void setScissorRect(const std::optional<::RectI> &scissorRect) override;

    virtual std::string getLayerDescriptionIdentifier() override;

protected:
    void addLines(const Tiled2dMapTileInfo &tileInfo, const std::unordered_map<int, std::vector<std::vector<std::tuple<std::vector<Coord>, int>>>> &styleIdLinesMap);

    void setupLines(const Tiled2dMapTileInfo &tileInfo, const std::vector<std::shared_ptr<GraphicsObjectInterface>> &newLineGraphicsObjects);

    void preGenerateRenderPasses();

private:
    static const int maxNumLinePoints = std::numeric_limits<uint16_t>::max() / 4 + 1; // 4 vertices per line coord, only 2 at the start/end
    static const int maxStylesPerGroup = 48;

    std::shared_ptr<LineVectorLayerDescription> description;

    std::vector<std::shared_ptr<LineGroupShaderInterface>> shaders;

    std::recursive_mutex lineMutex;
    std::unordered_map<Tiled2dMapTileInfo, std::vector<std::shared_ptr<LineGroup2dLayerObject>>> tileLinesMap;

    std::recursive_mutex featureGroupsMutex;
    std::vector<std::vector<std::tuple<size_t, FeatureContext>>> featureGroups;
    std::vector<std::vector<LineStyle>> reusableLineStyles;
    std::unordered_map<size_t, std::pair<int, int>> styleHashToGroupMap;

    const std::unordered_set<std::string> usedKeys;

    std::optional<::RectI> scissorRect = std::nullopt;
};

