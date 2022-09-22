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
#include "SymbolVectorLayerDescription.h"
#include "RenderVerticesDescription.h"
#include "LineInfo.h"
#include "Coord.h"
#include "Vec2D.h"
#include "PolygonGroup2dInterface.h"
#include "TextShaderInterface.h"
#include "TextLayerObject.h"
#include "TextInfo.h"
#include "FontLoaderInterface.h"
#include "FontLoaderResult.h"
#include "OBB2D.h"
#include "SymbolInfo.h"
#include "SpriteData.h"
#include "LoaderInterface.h"
#include "Quad2dInterface.h"
#include "SimpleTouchInterface.h"

//#define DRAW_TEXT_BOUNDING_BOXES
//#define DRAW_COLLIDED_TEXT_BOUNDING_BOXES

struct Tiled2dMapVectorSymbolFeatureWrapper {
    FeatureContext featureContext;
    std::shared_ptr<SymbolInfo> textInfo;
    std::shared_ptr<TextLayerObject> textObject;

    int64_t symbolSortKey;

    std::vector<float> modelMatrix;

    std::vector<float> iconModelMatrix;

    bool collides = true;

    std::shared_ptr<Quad2dInterface> symbolObject;
    std::shared_ptr<AlphaShaderInterface> symbolShader;

    OBB2D orientedBoundingBox = OBB2D(Quad2dD(Vec2D(0.0, 0.0), Vec2D(0.0, 0.0), Vec2D(0.0, 0.0), Vec2D(0.0, 0.0)));

    std::optional<Quad2dD> projectedTextQuad = std::nullopt;

#ifdef DRAW_TEXT_BOUNDING_BOXES
    std::shared_ptr<Quad2dInterface> boundingBox = nullptr;
#endif

    Tiled2dMapVectorSymbolFeatureWrapper() {};

    Tiled2dMapVectorSymbolFeatureWrapper(const Tiled2dMapVectorSymbolFeatureWrapper& a) : featureContext(a.featureContext), textInfo(a.textInfo), textObject(a.textObject), symbolSortKey(a.symbolSortKey), collides(a.collides), modelMatrix(a.modelMatrix), iconModelMatrix(a.iconModelMatrix), symbolObject(a.symbolObject), symbolShader(a.symbolShader)
#ifdef DRAW_TEXT_BOUNDING_BOXES
    ,boundingBox(a.boundingBox)
#endif
    { };

    Tiled2dMapVectorSymbolFeatureWrapper(const FeatureContext &featureContext, const std::shared_ptr<SymbolInfo> &textInfo, const std::shared_ptr<TextLayerObject> &textObject, const int64_t symbolSortKey) : featureContext(featureContext), textInfo(textInfo), textObject(textObject), symbolSortKey(symbolSortKey),  modelMatrix(16, 0), iconModelMatrix(16, 0) { };
};


struct Tiled2dMapVectorSymbolSubLayerPositioningWrapper {
    double angle;
    ::Coord centerPosition;

    Tiled2dMapVectorSymbolSubLayerPositioningWrapper(double angle, ::Coord centerPosition): angle(angle), centerPosition(centerPosition) {}
};

class Tiled2dMapVectorSymbolSubLayer : public Tiled2dMapVectorSubLayer,
                                     public SimpleTouchInterface,
                                     public std::enable_shared_from_this<Tiled2dMapVectorSymbolSubLayer> {
public:
    Tiled2dMapVectorSymbolSubLayer(const std::shared_ptr<FontLoaderInterface> &fontLoader, const std::shared_ptr<SymbolVectorLayerDescription> &description);

    ~Tiled2dMapVectorSymbolSubLayer() {}

    virtual void update() override;

    virtual void onAdded(const std::shared_ptr<MapInterface> &mapInterface) override;

    virtual void onRemoved() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void hide() override;

    virtual void show() override;

    virtual void updateTileData(const Tiled2dMapTileInfo &tileInfo, const std::shared_ptr<MaskingObjectInterface> &tileMask, const std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>> &layerFeatures) override;

    virtual void clearTileData(const Tiled2dMapTileInfo &tileInfo) override;

    virtual std::vector<std::shared_ptr<RenderPassInterface>> buildRenderPasses(const std::unordered_set<Tiled2dMapTileInfo> &tiles) override;

    bool isDirty();

    void collisionDetection(std::vector<OBB2D> &placements);

    void setSprites(std::shared_ptr<TextureHolderInterface> spriteTexture, std::shared_ptr<SpriteData> spriteData);

    virtual void setScissorRect(const std::optional<::RectI> &scissorRect) override;

    virtual bool onClickConfirmed(const ::Vec2F &posScreen) override;

    virtual std::string getLayerDescriptionIdentifier() override;

protected:
    void addTexts(const Tiled2dMapTileInfo &tileInfo,
                  const std::vector< std::tuple<const FeatureContext, std::shared_ptr<SymbolInfo>>> &texts);

    void setupTexts(const Tiled2dMapTileInfo &tileInfo,
                    const std::vector<Tiled2dMapVectorSymbolFeatureWrapper> texts);

    FontLoaderResult loadFont(const Font &font);

private:

    std::optional<::RectI> scissorRect = std::nullopt;

    std::optional<Tiled2dMapVectorSymbolSubLayerPositioningWrapper> getPositioning(std::vector<::Coord>::const_iterator &iterator, const std::vector<::Coord> & collection);

    Quad2dD getProjectedFrame(const RectCoord &boundingBox, const float &padding, const std::vector<float> &modelMatrix);
                                         
    const std::shared_ptr<FontLoaderInterface> fontLoader;
    const std::shared_ptr<SymbolVectorLayerDescription> description;

    std::recursive_mutex fontResultsMutex;
    std::unordered_map<std::string, FontLoaderResult> fontLoaderResults;

    std::recursive_mutex symbolMutex;
    std::unordered_map<Tiled2dMapTileInfo, std::vector<Tiled2dMapVectorSymbolFeatureWrapper>> tileTextMap;

    std::shared_ptr<TextureHolderInterface> spriteTexture;
    std::shared_ptr<SpriteData> spriteData;

    std::recursive_mutex tileTextPositionMapMutex;
    std::unordered_map<Tiled2dMapTileInfo, std::unordered_map<std::string, std::vector<Coord>>> tileTextPositionMap;

    std::recursive_mutex dirtyMutex;
    double lastZoom = 0.0;
    double lastRotation = 0.0;
    bool hasFreshData = false;

    std::vector<float> topLeftProj = { 0.0, 0.0, 0.0, 0.0 };
    std::vector<float> topRightProj = { 0.0, 0.0, 0.0, 0.0 };
    std::vector<float> bottomRightProj = { 0.0, 0.0, 0.0, 0.0 };
    std::vector<float> bottomLeftProj = { 0.0, 0.0, 0.0, 0.0 };
};
