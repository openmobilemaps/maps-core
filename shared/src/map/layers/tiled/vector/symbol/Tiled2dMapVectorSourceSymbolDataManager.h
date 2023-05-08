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

#include "Tiled2dMapVectorSourceTileDataManager.h"
#include "Tiled2dMapVectorSource.h"
#include "Tiled2dMapVectorSourceDataManager.h"
#include "SymbolVectorLayerDescription.h"
#include "Tiled2dMapVectorSymbolSubLayerPositioningWrapper.h"
#include "Tiled2dMapVectorSymbolFeatureWrapper.h"
#include "TextHelper.h"
#include "SpriteData.h"
#include "FontLoaderResult.h"
#include "TextInstancedInterface.h"
#include "TextInstancedShaderInterface.h"

class Tiled2dMapVectorSourceSymbolDataManager: public Tiled2dMapVectorSourceDataManager,  public std::enable_shared_from_this<Tiled2dMapVectorSourceSymbolDataManager> {
public:
    Tiled2dMapVectorSourceSymbolDataManager(const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                          const std::shared_ptr<VectorMapDescription> &mapDescription,
                                          const std::string &source,
                                          const std::shared_ptr<FontLoaderInterface> &fontLoader);

    using LayerIndentifier = std::string;

    void onAdded(const std::weak_ptr< ::MapInterface> &mapInterface) override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void setAlpha(float alpha) override;

    void onVectorTilesUpdated(const std::string &sourceName, std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) override;

    void updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription,
                                int32_t legacyIndex,
                                bool needsTileReplace) override;

    void collisionDetection(std::vector<std::string> layerIdentifiers, std::shared_ptr<std::vector<OBB2D>> placements);

    void update();

    void setSprites(std::shared_ptr<SpriteData> spriteData, std::shared_ptr<TextureHolderInterface> spriteTexture);

    bool onClickUnconfirmed(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) override;

    bool onClickConfirmed(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) override;

    bool onDoubleClick(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) override;

    bool onLongPress(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) override;

    bool onTwoFingerClick(const std::unordered_set<std::string> &layers, const Vec2F &posScreen1, const Vec2F &posScreen2) override;

    void clearTouch() override;

private:

    std::vector<std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper>> createSymbols(const Tiled2dMapTileInfo &tileInfo, const LayerIndentifier &identifier, const Tiled2dMapVectorTileInfo::FeatureTuple &feature);

    std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper> createSymbolWrapper(const Tiled2dMapTileInfo &tileInfo, const LayerIndentifier &identifier, const std::shared_ptr<SymbolVectorLayerDescription> &description, const std::tuple<const FeatureContext, std::shared_ptr<SymbolInfo>> &symbolInfo);

    void setupTexts(const std::vector<std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper>> toSetup, const std::unordered_set<Tiled2dMapTileInfo> tilesToRemove);

    FontLoaderResult loadFont(const Font &font);

    void setupExistingSymbolWithSprite();

    void pregenerateRenderPasses();
    
    std::unordered_map<std::string, FontLoaderResult> fontLoaderResults;

    inline std::optional<Tiled2dMapVectorSymbolSubLayerPositioningWrapper> getPositioning(std::vector<::Coord>::const_iterator &iterator, const std::vector<::Coord> & collection);

    inline Quad2dD getProjectedFrame(const RectCoord &boundingBox, const float &padding, const std::vector<float> &modelMatrix);

    std::unordered_map<Tiled2dMapTileInfo, std::unordered_map<LayerIndentifier, std::vector<std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper>>>> tileSymbolMap;

    std::unordered_map<Tiled2dMapTileInfo, std::unordered_map<std::string, std::vector<Coord>>> tileTextPositionMap;

    std::unordered_map<LayerIndentifier, std::shared_ptr<SymbolVectorLayerDescription>> layerDescriptions;

    TextHelper textHelper;

    const std::shared_ptr<FontLoaderInterface> fontLoader;

    std::shared_ptr<TextureHolderInterface> spriteTexture;

    std::shared_ptr<SpriteData> spriteData;

    std::vector<float> topLeftProj = { 0.0, 0.0, 0.0, 0.0 };
    std::vector<float> topRightProj = { 0.0, 0.0, 0.0, 0.0 };
    std::vector<float> bottomRightProj = { 0.0, 0.0, 0.0, 0.0 };
    std::vector<float> bottomLeftProj = { 0.0, 0.0, 0.0, 0.0 };

    std::unordered_map<std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper>, LayerIndentifier> interactableSet;

    float alpha = 1.0;

    std::optional<std::string> instanceFontName;
    std::shared_ptr<TextInstancedInterface> instancedObject;
    std::shared_ptr<TextInstancedShaderInterface> instancedShader;
    //cached locked unsafe renderpasses
};
