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

#include "Tiled2dMapVectorSourceDataManager.h"
#include "SpriteData.h"

class Tiled2dMapVectorLayer;
class Tiled2dMapVectorTile;

class Tiled2dMapVectorSourceTileDataManager : public Tiled2dMapVectorLayerTileCallbackInterface,
                                               public Tiled2dMapVectorSourceDataManager,
                                              public std::enable_shared_from_this<Tiled2dMapVectorSourceTileDataManager> {
public:
    using Tiled2dMapVectorSourceDataManager::Tiled2dMapVectorSourceDataManager;

    virtual void update();

    virtual void pause() override;

    virtual void resume() override;

    virtual void setAlpha(float alpha) override;

    virtual void tileIsReady(const Tiled2dMapTileInfo &tile,
                             const std::string &layerIdentifier,
                             const WeakActor<Tiled2dMapVectorTile> &tileActor) override;

    void tileIsInteractable(const std::string &layerIdentifier) override;

    virtual void setSelectionDelegate(const std::shared_ptr<Tiled2dMapVectorLayerSelectionCallbackInterface> &selectionDelegate) override;

    void setSelectedFeatureIdentifier(std::optional<int64_t> identifier) override;

    void updateMaskObjects(const std::unordered_map<Tiled2dMapTileInfo, Tiled2dMapLayerMaskWrapper> &toSetupMaskObject,
                           const std::unordered_set<Tiled2dMapTileInfo> &tilesToRemove);

    bool onClickUnconfirmed(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) override;

    bool onClickConfirmed(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) override;

    bool onDoubleClick(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) override;

    bool onLongPress(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) override;

    bool onTwoFingerClick(const std::unordered_set<std::string> &layers, const Vec2F &posScreen1, const Vec2F &posScreen2) override;

    void clearTouch() override;

    void setSprites(std::shared_ptr<SpriteData> spriteData, std::shared_ptr<TextureHolderInterface> spriteTexture) override;


protected:
    Actor<Tiled2dMapVectorTile> createTileActor(const Tiled2dMapTileInfo &tileInfo,
                                                const std::shared_ptr<VectorLayerDescription> &layerDescription);

    void setupExistingTilesWithSprite();

    virtual void pregenerateRenderPasses();

    virtual void onTileCompletelyReady(const Tiled2dMapTileInfo tileInfo) = 0;

    std::unordered_map<Tiled2dMapTileInfo, std::vector<std::tuple<int32_t, std::vector<std::shared_ptr<RenderObjectInterface>>>>> tileRenderObjectsMap;
    std::unordered_map<Tiled2dMapTileInfo, std::vector<std::tuple<int32_t, std::string, Actor<Tiled2dMapVectorTile>>>> tiles;
    std::unordered_map<Tiled2dMapTileInfo, Tiled2dMapLayerMaskWrapper> tileMaskMap;
    std::unordered_map<Tiled2dMapTileInfo, TileState> tileStateMap;
    std::unordered_set<Tiled2dMapTileInfo> tilesReady;
    std::unordered_map<Tiled2dMapTileInfo, std::unordered_set<int32_t>> tilesReadyControlSet;

    std::unordered_set<std::string> interactableLayers;

    std::shared_ptr<SpriteData> spriteData;
    std::shared_ptr<TextureHolderInterface> spriteTexture;

    float alpha = 1.0;
};
