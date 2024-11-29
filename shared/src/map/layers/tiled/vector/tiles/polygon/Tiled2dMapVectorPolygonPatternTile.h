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
#include "PolygonPatternGroup2dLayerObject.h"
#include "PolygonCoord.h"
#include "SpriteData.h"

class Tiled2dMapVectorPolygonPatternTile
        : public Tiled2dMapVectorTile,
          public std::enable_shared_from_this<Tiled2dMapVectorPolygonPatternTile> {
public:
    Tiled2dMapVectorPolygonPatternTile(const std::weak_ptr<MapInterface> &mapInterface,
                                       const Tiled2dMapVersionedTileInfo &tileInfo,
                                       const WeakActor<Tiled2dMapVectorLayerTileCallbackInterface> &tileCallbackInterface,
                                       const std::shared_ptr<PolygonVectorLayerDescription> &description,
                                       const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                       const std::shared_ptr<SpriteData> &spriteData,
                                       const std::shared_ptr<TextureHolderInterface> &spriteTexture,
                                       const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager);

    void updateVectorLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                const Tiled2dMapVectorTileDataVector &layerFeatures) override;

    void update() override;

    virtual std::vector<std::shared_ptr<RenderObjectInterface>> generateRenderObjects() override;

    virtual void clear() override;

    virtual void setup() override;

    virtual void setVectorTileData(const Tiled2dMapVectorTileDataVector &tileData) override;

    void setSpriteData(const std::shared_ptr<SpriteData> &spriteData, const std::shared_ptr<TextureHolderInterface> &spriteTexture) override;

    bool onClickConfirmed(const Vec2F &posScreen) override;

    bool performClick(const Coord &coord) override;

private:
    struct ObjectDescriptions {
        std::vector<float> vertices;
        std::vector<uint16_t> indices;
    };

    void addPolygons(const std::vector<std::vector<ObjectDescriptions>> &styleGroupNewPolygonsVector, const Vec3D & origin);

    void setupPolygons(const std::vector<std::shared_ptr<GraphicsObjectInterface>> &newPolygonObjects);

    void setupTextureCoordinates();

#ifdef OPENMOBILEMAPS_GL
    static const int maxStylesPerGroup = 16;
#else
    static const int maxStylesPerGroup = 256;
#endif

    std::vector<std::shared_ptr<PolygonPatternGroupShaderInterface>> shaders;
    std::unordered_map<int, std::vector<std::shared_ptr<PolygonPatternGroup2dLayerObject>>> styleGroupPolygonsMap;
    std::vector<std::shared_ptr<PolygonPatternGroup2dLayerObject>> polygonRenderOrder;
    std::vector<std::vector<std::tuple<size_t, std::shared_ptr<FeatureContext>>>> featureGroups;
    std::unordered_map<size_t, std::pair<int, int>> styleHashToGroupMap;
    UsedKeysCollection usedKeys;
    bool isStyleZoomDependant = true;
    bool isStyleStateDependant = true;
    std::optional<double> lastZoom = std::nullopt;
    std::optional<bool> lastInZoomRange = std::nullopt;

    bool fadeInPattern = false;

    std::shared_ptr<SpriteData> spriteData;
    std::shared_ptr<TextureHolderInterface> spriteTexture;

    std::vector<std::vector<float>> opacities;
    std::vector<std::vector<float>> textureCoordinates;

    std::vector<std::tuple<VectorTileGeometryHandler::TriangulatedPolygon, std::shared_ptr<FeatureContext>>> hitDetectionPolygons;

    std::vector<std::shared_ptr<PolygonPatternGroup2dLayerObject>> toClear;
};
