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
#include "CollisionGrid.h"
#include "SymbolAnimationCoordinator.h"
#include "SymbolAnimationCoordinatorMap.h"
#include "Tiled2dMapVectorSymbolFontProviderManager.h"
#include "Tiled2dMapVectorLayerSymbolDelegateInterface.h"

struct InstanceCounter {
    InstanceCounter() : baseValue(0), decreasingCounter(0) {}

    void increaseBase() {
        baseValue++;
        decreasingCounter++;
    }

    bool decreaseAndCheckFinal() {
        return --decreasingCounter <= 0;
    }

    bool isDone() {
        return decreasingCounter <= 0;
    }

    uint16_t baseValue;
private:
    uint16_t decreasingCounter;
};

class Tiled2dMapVectorSourceSymbolDataManager:
        public Tiled2dMapVectorSourceDataManager,
        public std::enable_shared_from_this<Tiled2dMapVectorSourceSymbolDataManager> {
public:
    Tiled2dMapVectorSourceSymbolDataManager(const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                            const std::shared_ptr<VectorMapDescription> &mapDescription,
                                            const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                            const std::string &source,
                                            const std::shared_ptr<FontLoaderInterface> &fontLoader,
                                            const WeakActor<Tiled2dMapVectorSource> &vectorSource,
                                            const Actor<Tiled2dMapVectorReadyManager> &readyManager,
                                            const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager,
                                            const std::shared_ptr<Tiled2dMapVectorLayerSymbolDelegateInterface> &symbolDelegate,
                                            bool persistingSymbolPlacement);

    void onAdded(const std::weak_ptr<::MapInterface> &mapInterface) override;

    void onRemoved() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void setAlpha(float alpha) override;

    void onVectorTilesUpdated(const std::string &sourceName, std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) override;

    void updateLayerDescriptions(std::vector<Tiled2dMapVectorLayerUpdateInformation> layerUpdates) override;

    void updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription,
                                int32_t legacyIndex,
                                bool needsTileReplace) override;

    void reloadLayerContent(const std::vector<std::tuple<std::string, std::string>> &sourceLayerIdentifierPairs);

    void collisionDetection(std::vector<std::string> layerIdentifiers, std::shared_ptr<CollisionGrid> collisionGrid);

    bool update(long long now);

    void setSprites(std::shared_ptr<SpriteData> spriteData, std::shared_ptr<TextureHolderInterface> spriteTexture) override;

    bool onClickUnconfirmed(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) override;

    bool onClickConfirmed(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) override;

    bool onDoubleClick(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) override;

    bool onLongPress(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) override;

    bool onTwoFingerClick(const std::unordered_set<std::string> &layers, const Vec2F &posScreen1, const Vec2F &posScreen2) override;

    void clearTouch() override;

    bool performClick(const std::unordered_set<std::string> &layers, const Coord &coord) override;

    void onSymbolGroupInitialized(bool success, const Tiled2dMapVersionedTileInfo &tileInfo, const std::string &layerIdentifier, const WeakActor<Tiled2dMapVectorSymbolGroup> &symbolGroup);

    void setSymbolDelegate(const /*not-null*/ std::shared_ptr<Tiled2dMapVectorLayerSymbolDelegateInterface> & symbolDelegate);

    void enableAnimations(bool enabled);

private:
    std::vector<Actor<Tiled2dMapVectorSymbolGroup>>
    createSymbolGroups(const Tiled2dMapVersionedTileInfo &tileInfo, const std::string &layerIdentifier,
                       std::shared_ptr<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>> features);

    void setupSymbolGroups(const Tiled2dMapVersionedTileInfo &tileInfo, const std::string &layerIdentifier);

    void updateSymbolGroups();

    void setupExistingSymbolWithSprite();

    void pregenerateRenderPasses();

    const WeakActor<Tiled2dMapVectorSource> vectorSource;

    std::unordered_map<Tiled2dMapVersionedTileInfo, std::unordered_map<std::string, std::tuple<InstanceCounter, std::vector<Actor<Tiled2dMapVectorSymbolGroup>>>>> tileSymbolGroupMap;
    std::unordered_map<Tiled2dMapVersionedTileInfo, TileState> tileStateMap;

    std::atomic_flag updateFlag = ATOMIC_FLAG_INIT;
    std::recursive_mutex updateMutex;
    std::vector<Actor<Tiled2dMapVectorSymbolGroup>> tilesToClear;
    std::unordered_set<Tiled2dMapVersionedTileInfo> tileStatesToRemove;
    std::unordered_map<Tiled2dMapVersionedTileInfo, TileState> tileStateUpdates;

    std::unordered_map<std::string, std::shared_ptr<SymbolVectorLayerDescription>> layerDescriptions;

    TextHelper textHelper;
    std::shared_ptr<FontLoaderInterface> fontLoader;
    Actor<Tiled2dMapVectorSymbolFontProviderManager> fontProviderManager;

    std::shared_ptr<TextureHolderInterface> spriteTexture;
    std::shared_ptr<SpriteData> spriteData;

    float alpha = 1.0;

    std::unordered_set<std::string> interactableLayers;

    std::shared_ptr<SymbolAnimationCoordinatorMap> animationCoordinatorMap;
    std::shared_ptr<Tiled2dMapVectorLayerSymbolDelegateInterface> symbolDelegate;

    bool persistingSymbolPlacement = false;

#ifdef __ANDROID__
    // Higher counts may cause issues for instanced text rendering
    int32_t maxNumFeaturesPerGroup = 3500;
#else
    int32_t maxNumFeaturesPerGroup = std::numeric_limits<int32_t>().max();
#endif
};
