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
#include "TextHelper.h"
#include "SpriteData.h"
#include "FontLoaderResult.h"
#include "Tiled2dMapVectorSymbolGroup.h"
#include "TextInstancedInterface.h"
#include "Tiled2dMapVectorFontProvider.h"

class Tiled2dMapVectorSourceSymbolDataManager:
        public Tiled2dMapVectorSourceDataManager,
        public std::enable_shared_from_this<Tiled2dMapVectorSourceSymbolDataManager>,
        public Tiled2dMapVectorFontProvider {
public:
    Tiled2dMapVectorSourceSymbolDataManager(const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                          const std::shared_ptr<VectorMapDescription> &mapDescription,
                                          const std::string &source,
                                          const std::shared_ptr<FontLoaderInterface> &fontLoader);

    void onAdded(const std::weak_ptr< ::MapInterface> &mapInterface) override;

    void onRemoved() override;

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

    std::shared_ptr<FontLoaderResult> loadFont(const std::string &fontName) override;

private:

    std::optional<Actor<Tiled2dMapVectorSymbolGroup>> createSymbolGroup(const Tiled2dMapTileInfo &tileInfo, const std::string &layerIdentifier, const std::shared_ptr<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>> features);

    void setupSymbolGroups(const std::vector<Actor<Tiled2dMapVectorSymbolGroup>> toSetup, const std::unordered_set<Tiled2dMapTileInfo> tilesToRemove);

    void setupExistingSymbolWithSprite();

    void pregenerateRenderPasses();
    
    std::unordered_map<std::string, std::shared_ptr<FontLoaderResult>> fontLoaderResults;

    std::unordered_map<Tiled2dMapTileInfo, std::unordered_map<std::string, std::vector<Actor<Tiled2dMapVectorSymbolGroup>>>> tileSymbolGroupMap;

    std::unordered_map<std::string, std::shared_ptr<SymbolVectorLayerDescription>> layerDescriptions;

    TextHelper textHelper;

    const std::shared_ptr<FontLoaderInterface> fontLoader;

    std::shared_ptr<TextureHolderInterface> spriteTexture;

    std::shared_ptr<SpriteData> spriteData;

    float alpha = 1.0;
};
