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
#include "Tiled2dMapVectorLayerInterface.h"
#include "Tiled2dMapVectorSource.h"
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

class Tiled2dMapVectorLayer : public Tiled2dMapLayer, public TouchInterface, public Tiled2dMapVectorLayerInterface, public Tiled2dMapVectorLayerReadyInterface, public ActorObject {
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

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() override;

    virtual void onAdded(const std::shared_ptr<::MapInterface> &mapInterface, int32_t layerIndex) override;

    virtual void onRemoved() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void setAlpha(float alpha) override;

    virtual float getAlpha() override;

    void forceReload() override;

    void onTilesUpdated(std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos);

    virtual void tileIsReady(const Tiled2dMapTileInfo &tile) override;

    virtual void setScissorRect(const std::optional<::RectI> &scissorRect) override;

    void setSelectionDelegate(const std::weak_ptr<Tiled2dMapVectorLayerSelectionInterface> selectionDelegate);

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


    Actor<Tiled2dMapVectorSource> vectorTileSource;

    const std::vector<std::shared_ptr<::LoaderInterface>> loaders;

    virtual std::optional<TiledLayerError> loadStyleJson();
    virtual std::optional<TiledLayerError> loadStyleJsonRemotely();
    virtual std::optional<TiledLayerError> loadStyleJsonLocally(std::string styleJsonString);

private:
    void scheduleStyleJsonLoading();


    void initializeVectorLayer();

    void updateMaskObjects(const std::unordered_map<Tiled2dMapTileInfo, Tiled2dMapLayerMaskWrapper> toSetupMaskObject, const std::unordered_set<Tiled2dMapTileInfo> tilesToRemove);

    int32_t layerIndex = -1;

    const std::optional<double> dpFactor;

    const std::string layerName;
    std::optional<std::string> remoteStyleJsonUrl;
    std::optional<std::string> fallbackStyleJsonString;

    std::shared_ptr<VectorMapDescription> mapDescription;

    std::unordered_map<std::string, std::shared_ptr<Tiled2dMapLayerConfig>> layerConfigs;

    const std::shared_ptr<FontLoaderInterface> fontLoader;

    std::recursive_mutex tileUpdateMutex;

    std::recursive_mutex tilesReadyMutex;
    std::unordered_set<Tiled2dMapTileInfo> tilesReady;

    std::recursive_mutex tilesReadyCountMutex;
    std::unordered_map<Tiled2dMapTileInfo, int> tilesReadyCount;

    std::recursive_mutex tileMaskMapMutex;
    std::unordered_map<Tiled2dMapTileInfo, Tiled2dMapLayerMaskWrapper> tileMaskMap;

    std::recursive_mutex tilesMutex;
    std::unordered_map<Tiled2dMapTileInfo, std::vector<Actor<Tiled2dMapVectorTile>>> tiles;

    std::atomic_bool isLoadingStyleJson = false;
    std::atomic_bool isResumed = false;

    std::weak_ptr<Tiled2dMapVectorLayerSelectionInterface> selectionDelegate;

    float alpha;
};


