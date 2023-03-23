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

#include "Tiled2dMapLayer.h"
#include "Tiled2dMapRasterSource.h"
#include "Tiled2dMapRasterSourceListener.h"
#include "Tiled2dMapVectorLayerInterface.h"
#include "Tiled2dMapVectorSource.h"
#include "Tiled2dMapVectorSourceListener.h"
#include "Tiled2dMapVectorSubLayer.h"
#include "Tiled2dMapVectorTile.h"
#include "VectorMapDescription.h"
#include "FontLoaderInterface.h"
#include "PolygonMaskObject.h"
#include "Tiled2dMapVectorLayerReadyInterface.h"
#include "Tiled2dMapLayerMaskWrapper.h"
#include "Tiled2dMapVectorLayerSelectionInterface.h"
#include "TiledLayerError.h"
#include "Actor.h"
#include "Tiled2dMapVectorBackgroundSubLayer.h"
#include "Tiled2dMapVectorSourceTileDataManager.h"
#include "Tiled2dMapVectorSourceRasterTileDataManager.h"
#include "Tiled2dMapVectorSourceVectorTileDataManager.h"
#include "Tiled2dMapVectorSourceSymbolDataManager.h"
#include "Tiled2dMapVectorSourceSymbolCollisionManager.h"
#include <unordered_map>

class Tiled2dMapVectorLayer
        : public Tiled2dMapLayer,
          public TouchInterface,
          public Tiled2dMapVectorLayerInterface,
          public ActorObject,
          public Tiled2dMapRasterSourceListener,
          public Tiled2dMapVectorSourceListener {
public:
    Tiled2dMapVectorLayer(const std::string &layerName,
                          const std::string &remoteStyleJsonUrl,
                          const std::vector <std::shared_ptr<::LoaderInterface>> &loaders,
                          const std::shared_ptr<::FontLoaderInterface> &fontLoader,
                          double dpFactor);

    Tiled2dMapVectorLayer(const std::string &layerName,
                          const std::string &remoteStyleJsonUrl,
                          const std::string &fallbackStyleJsonString,
                          const std::vector <std::shared_ptr<::LoaderInterface>> &loaders,
                          const std::shared_ptr<::FontLoaderInterface> &fontLoader,
                          double dpFactor);

    Tiled2dMapVectorLayer(const std::string &layerName,
                          const std::shared_ptr<VectorMapDescription> & mapDescription,
                          const std::vector<std::shared_ptr<::LoaderInterface>> & loaders,
                          const std::shared_ptr<::FontLoaderInterface> & fontLoader);

    Tiled2dMapVectorLayer(const std::string &layerName,
                          const std::vector<std::shared_ptr<::LoaderInterface>> & loaders,
                          const std::shared_ptr<::FontLoaderInterface> & fontLoader);

    virtual std::shared_ptr<::LayerInterface> asLayerInterface() override;

    virtual void update() override;

    virtual std::vector<::RenderTask> getRenderTasks() override;

    virtual void onRenderPassUpdate(const std::string &source, bool isSymbol, const std::vector<std::tuple<int32_t, std::shared_ptr<RenderPassInterface>>> &renderPasses);

    virtual void onAdded(const std::shared_ptr<::MapInterface> &mapInterface, int32_t layerIndex) override;

    virtual void onRemoved() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void setAlpha(float alpha) override;

    virtual float getAlpha() override;

    void forceReload() override;

    void onTilesUpdated(const std::string &layerName, std::unordered_set<Tiled2dMapRasterTileInfo> currentTileInfos) override;

    void onTilesUpdated(const std::string &sourceName, std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) override;

    virtual void setScissorRect(const std::optional<::RectI> &scissorRect) override;

    void setSelectionDelegate(const WeakActor<Tiled2dMapVectorLayerSelectionInterface> &selectionDelegate);

    void setSelectedFeatureIdentifier(std::optional<int64_t> identifier);

    std::shared_ptr<VectorLayerDescription> getLayerDescriptionWithIdentifier(std::string identifier);

    void updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription);

    std::optional<FeatureContext> getFeatureContext(int64_t identifier);

    // Touch Interface
    bool onTouchDown(const Vec2F &posScreen) override;

    bool onClickUnconfirmed(const Vec2F &posScreen) override;

    bool onClickConfirmed(const Vec2F &posScreen) override;

    bool onDoubleClick(const Vec2F &posScreen) override;

    bool onLongPress(const Vec2F &posScreen) override;

    bool onMove(const Vec2F &deltaScreen, bool confirmed, bool doubleClick) override;

    bool onMoveComplete() override;

    bool onTwoFingerClick(const Vec2F &posScreen1, const Vec2F &posScreen2) override;

    bool onTwoFingerMove(const std::vector<::Vec2F> &posScreenOld, const std::vector<::Vec2F> &posScreenNew) override;

    bool onTwoFingerMoveComplete() override;

    void clearTouch() override;

protected:
    virtual std::shared_ptr<Tiled2dMapLayerConfig> getLayerConfig(const std::shared_ptr<VectorMapSourceDescription> &source);

    virtual void setMapDescription(const std::shared_ptr<VectorMapDescription> &mapDescription);

    virtual void loadSpriteData();

    virtual void didLoadSpriteData(std::shared_ptr<SpriteData> spriteData, std::shared_ptr<::TextureHolderInterface> spriteTexture);

    std::unordered_map<std::string, Actor<Tiled2dMapVectorSource>> vectorTileSources;
    std::vector<Actor<Tiled2dMapRasterSource>> rasterTileSources;

    const std::vector<std::shared_ptr<::LoaderInterface>> loaders;

    virtual std::optional<TiledLayerError> loadStyleJson();
    virtual std::optional<TiledLayerError> loadStyleJsonRemotely();
    virtual std::optional<TiledLayerError> loadStyleJsonLocally(std::string styleJsonString);


private:
    void scheduleStyleJsonLoading();

    void initializeVectorLayer();

    int32_t layerIndex = -1;

    const std::optional<double> dpFactor;

    const std::string layerName;
    std::optional<std::string> remoteStyleJsonUrl;
    std::optional<std::string> fallbackStyleJsonString;

    std::shared_ptr<VectorMapDescription> mapDescription;

    std::shared_ptr<Tiled2dMapVectorBackgroundSubLayer> backgroundLayer;

    std::unordered_map<std::string, std::shared_ptr<Tiled2dMapLayerConfig>> layerConfigs;

    const std::shared_ptr<FontLoaderInterface> fontLoader;

    std::recursive_mutex dataManagerMutex;
    std::unordered_map<std::string, Actor<Tiled2dMapVectorSourceTileDataManager>> sourceDataManagers;
    std::unordered_map<std::string, Actor<Tiled2dMapVectorSourceSymbolDataManager>> symbolSourceDataManagers;

    Actor<Tiled2dMapVectorSourceSymbolCollisionManager> collisionManager;

    std::recursive_mutex renderPassMutex;
    std::vector<RenderTask> currentRenderTasks;

    struct SourceRenderPasses {
        std::vector<std::tuple<int32_t, std::shared_ptr<RenderPassInterface>>> renderPasses;
        std::vector<std::tuple<int32_t, std::shared_ptr<RenderPassInterface>>> symbolRenderPasses;
    };

    std::unordered_map<std::string, SourceRenderPasses> sourceRenderPassesMap;

    std::atomic_bool isLoadingStyleJson = false;
    std::atomic_bool isResumed = false;

    WeakActor<Tiled2dMapVectorLayerSelectionInterface> selectionDelegate;

    float alpha = 1.0;
    std::optional<::RectI> scissorRect = std::nullopt;

    std::shared_ptr<SpriteData> spriteData;
    std::shared_ptr<::TextureHolderInterface> spriteTexture;

    bool enableOffscreenRendering = true;
};


