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
#include "PolygonVectorLayerDescription.h"
#include "RenderVerticesDescription.h"
#include "LineInfo.h"
#include "Coord.h"
#include "PolygonGroup2dInterface.h"
#include "PolygonGroupShaderInterface.h"
#include "PolygonGroup2dLayerObject.h"
#include "SimpleTouchInterface.h"

class Tiled2dMapVectorPolygonSubLayer : public Tiled2dMapVectorSubLayer,
                                        public SimpleTouchInterface,
                                        public std::enable_shared_from_this<Tiled2dMapVectorPolygonSubLayer> {
public:
    Tiled2dMapVectorPolygonSubLayer(const std::shared_ptr<PolygonVectorLayerDescription> &description);

    ~Tiled2dMapVectorPolygonSubLayer() {}

    virtual void update() override;

    virtual void onAdded(const std::shared_ptr<MapInterface> &mapInterface, int32_t layerIndex) override;

    virtual void onRemoved() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void hide() override;

    virtual void show() override;

    virtual void updateTileData(const Tiled2dMapTileInfo &tileInfo, const std::shared_ptr<MaskingObjectInterface> &tileMask,
                                const std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>> &layerFeatures) override;

    void updateTileMask(const Tiled2dMapTileInfo &tileInfo, const std::shared_ptr<MaskingObjectInterface> &tileMask) override;

    virtual void clearTileData(const Tiled2dMapTileInfo &tileInfo) override;

    virtual void setScissorRect(const std::optional<::RectI> &scissorRect) override;

    virtual std::string getLayerDescriptionIdentifier() override;

    virtual bool onClickConfirmed(const ::Vec2F &posScreen) override;
protected:

    void setupPolygons(const Tiled2dMapTileInfo &tileInfo, const std::vector<std::shared_ptr<GraphicsObjectInterface>> &newPolygonObjects);

    void preGenerateRenderPasses();

private:
    std::optional<::RectI> scissorRect = std::nullopt;
                                         
    std::shared_ptr<PolygonVectorLayerDescription> description;

    void addPolygons(const Tiled2dMapTileInfo &tileInfo, const std::vector<std::tuple<std::vector<std::tuple<std::vector<Coord>, int>>, std::vector<int32_t>>> &polygons);

    std::shared_ptr<PolygonGroupShaderInterface> shader;

    std::recursive_mutex polygonMutex;
    std::unordered_map<Tiled2dMapTileInfo, std::vector<std::shared_ptr<PolygonGroup2dLayerObject>>> tilePolygonMap;

    std::recursive_mutex featureGroupsMutex;
    std::vector<std::tuple<size_t, FeatureContext>> featureGroups;

    const std::unordered_set<std::string> usedKeys;

    std::unordered_map<Tiled2dMapTileInfo, std::vector<std::tuple<PolygonCoord, FeatureContext>>> hitDetectionPolygonMap;
};
