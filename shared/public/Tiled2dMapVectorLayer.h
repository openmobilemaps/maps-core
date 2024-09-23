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
#include "VectorMapSourceDescription.h"
#include "FontLoaderInterface.h"
#include "PolygonMaskObject.h"
#include "Tiled2dMapVectorLayerTileCallbackInterface.h"
#include "Tiled2dMapLayerMaskWrapper.h"
#include "TiledLayerError.h"
#include "Actor.h"
#include "Tiled2dMapVectorLayerConfig.h"
#include "Tiled2dMapVectorStateManager.h"
#include "Tiled2dMapVectorLayerLocalDataProviderInterface.h"
#include <unordered_map>

class Tiled2dMapVectorBackgroundSubLayer;
class Tiled2dMapVectorSourceTileDataManager;
class Tiled2dMapVectorSourceRasterTileDataManager;
class Tiled2dMapVectorSourceVectorTileDataManager;
class Tiled2dMapVectorSourceSymbolDataManager;
class Tiled2dMapVectorSourceSymbolCollisionManager;
class Tiled2dMapVectorInteractionManager;

struct Tiled2dMapVectorLayerUpdateInformation {
    std::shared_ptr<VectorLayerDescription> layerDescription;
    std::shared_ptr<VectorLayerDescription> oldLayerDescription;
    int32_t legacyIndex;
    bool needsTileReplace;
};

enum class StateType : int {
    FEATURE = 0,
    GLOBAL = 1,
    BOTH = 2,
};

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
                          const std::optional<Tiled2dMapZoomInfo> &customZoomInfo = std::nullopt,
                          const std::shared_ptr<Tiled2dMapVectorLayerSymbolDelegateInterface> &symbolDelegate = nullptr,
                          const std::unordered_map<std::string, std::string> & sourceUrlParams = {},
                          const std::shared_ptr<Tiled2dMapVectorLayerLocalDataProviderInterface> &localDataProvider = nullptr);

    Tiled2dMapVectorLayer(const std::string &layerName,
                          const std::string &remoteStyleJsonUrl,
                          const std::string &fallbackStyleJsonString,
                          const std::vector <std::shared_ptr<::LoaderInterface>> &loaders,
                          const std::shared_ptr<::FontLoaderInterface> &fontLoader,
                          const std::optional<Tiled2dMapZoomInfo> &customZoomInfo = std::nullopt,
                          const std::shared_ptr<Tiled2dMapVectorLayerSymbolDelegateInterface> &symbolDelegate = nullptr,
                          const std::unordered_map<std::string, std::string> & sourceUrlParams = {},
                          const std::shared_ptr<Tiled2dMapVectorLayerLocalDataProviderInterface> &localDataProvider = nullptr);

    Tiled2dMapVectorLayer(const std::string &layerName,
                          const std::shared_ptr<VectorMapDescription> & mapDescription,
                          const std::vector<std::shared_ptr<::LoaderInterface>> & loaders,
                          const std::shared_ptr<::FontLoaderInterface> & fontLoader,
                          const std::optional<Tiled2dMapZoomInfo> &customZoomInfo = std::nullopt,
                          const std::shared_ptr<Tiled2dMapVectorLayerSymbolDelegateInterface> &symbolDelegate = nullptr,
                          const std::shared_ptr<Tiled2dMapVectorLayerLocalDataProviderInterface> &localDataProvider = nullptr,
                          const std::unordered_map<std::string, std::string> & sourceUrlParams = {});

    Tiled2dMapVectorLayer(const std::string &layerName,
                          const std::vector<std::shared_ptr<::LoaderInterface>> & loaders,
                          const std::shared_ptr<::FontLoaderInterface> &fontLoader,
                          const std::optional<Tiled2dMapZoomInfo> &customZoomInfo = std::nullopt,
                          const std::shared_ptr<Tiled2dMapVectorLayerSymbolDelegateInterface> &symbolDelegate = nullptr,
                          const std::unordered_map<std::string, std::string> & sourceUrlParams = {});

    virtual std::shared_ptr<::LayerInterface> asLayerInterface() override;

    virtual void update() override;

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() override;

    struct TileRenderDescription {
        int32_t layerIndex;
        std::vector<std::shared_ptr<::RenderObjectInterface>> renderObjects;
        std::shared_ptr<MaskingObjectInterface> maskingObject;
        bool isModifyingMask;
        bool selfMasked;
        int32_t renderPassIndex;
    };

    virtual void onRenderPassUpdate(const std::string &source, bool isSymbol, const std::vector<std::shared_ptr<TileRenderDescription>> &renderDescription);

    virtual void onAdded(const std::shared_ptr<::MapInterface> &mapInterface, int32_t layerIndex) override;

    virtual void onRemoved() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void setAlpha(float alpha) override;

    virtual float getAlpha() override;

    void pregenerateRenderPasses();

    void forceReload() override;

    void onTilesUpdated(const std::string &layerName, std::unordered_set<Tiled2dMapRasterTileInfo> currentTileInfos) override;

    void onTilesUpdated(const std::string &sourceName, std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) override;

    virtual void setScissorRect(const std::optional<::RectI> &scissorRect) override;

    void setSelectionDelegate(const std::shared_ptr<Tiled2dMapVectorLayerSelectionCallbackInterface> &selectionDelegate) override;

    void setSelectionDelegate(const std::weak_ptr<Tiled2dMapVectorLayerSelectionCallbackInterface> &selectionDelegate);

    std::shared_ptr<VectorLayerDescription> getLayerDescriptionWithIdentifier(std::string identifier);

    void updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription);

    void updateLayerDescriptions(const std::vector<std::shared_ptr<VectorLayerDescription>> &layerDescriptions);

    std::optional<std::shared_ptr<FeatureContext>> getFeatureContext(int64_t identifier);

    LayerReadyState isReadyToRenderOffscreen() override;

    void setMinZoomLevelIdentifier(std::optional<int32_t> value) override;

    std::optional<int32_t> getMinZoomLevelIdentifier() override;

    void setMaxZoomLevelIdentifier(std::optional<int32_t> value) override;

    std::optional<int32_t> getMaxZoomLevelIdentifier() override;

    void performClick(const Coord &coord) override;

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

    virtual std::optional<std::string> getStyleMetadataJson() override;

    virtual void setFeatureState(const std::string & identifier, const std::unordered_map<std::string, VectorLayerFeatureInfoValue> & properties) override;

    virtual void setGlobalState(const std::unordered_map<std::string, VectorLayerFeatureInfoValue> & properties) override;

    void invalidateCollisionState();

    virtual void reloadDataSource(const std::string & sourceName) override;

	virtual void reloadLocalDataSource(const std::string & sourceName, const std::string & geoJson) override;

    virtual void setReadyStateListener(const /*not-null*/ std::shared_ptr<::Tiled2dMapReadyStateListener> & listener) override;

	protected:
    virtual void setMapDescription(const std::shared_ptr<VectorMapDescription> &mapDescription);

    virtual std::shared_ptr<Tiled2dMapVectorLayerConfig> getLayerConfig(const std::shared_ptr<VectorMapSourceDescription> &source);
    virtual std::shared_ptr<Tiled2dMapVectorLayerConfig> getGeoJSONLayerConfig(const std::string &sourceName, const std::shared_ptr<GeoJSONVTInterface> &source);

    virtual void loadSpriteData(int scale, bool fromLocal = true);

    virtual void didLoadSpriteData(std::shared_ptr<SpriteData> spriteData, std::shared_ptr<::TextureHolderInterface> spriteTexture);

    void enableAnimations(bool enabled) override;

    std::unordered_map<std::string, Actor<Tiled2dMapVectorSource>> vectorTileSources;
    std::vector<Actor<Tiled2dMapRasterSource>> rasterTileSources;

    const std::vector<std::shared_ptr<::LoaderInterface>> loaders;

    virtual std::optional<TiledLayerError> loadStyleJson();
    virtual std::optional<TiledLayerError> loadStyleJsonRemotely();
    virtual std::optional<TiledLayerError> loadStyleJsonLocally(std::string styleJsonString);


protected:
    std::optional<std::string> metadata = std::nullopt;

    std::atomic_bool isLoadingStyleJson = false;
    virtual void didLoadStyleJson(const std::optional<TiledLayerError> &error);
private:
    void scheduleStyleJsonLoading();

    void initializeVectorLayer();

    void applyGlobalOrFeatureStateIfPossible(StateType type);
    
    int32_t layerIndex = -1;

    const std::unordered_map<std::string, std::string> sourceUrlParams;

    const std::string layerName;
    std::optional<Tiled2dMapZoomInfo> customZoomInfo;
    std::optional<std::string> remoteStyleJsonUrl;
    std::optional<std::string> fallbackStyleJsonString;

    std::recursive_mutex mapDescriptionMutex;
    std::shared_ptr<VectorMapDescription> mapDescription;

    bool persistingSymbolPlacement;

    std::shared_ptr<Tiled2dMapVectorBackgroundSubLayer> backgroundLayer;

    std::unordered_map<std::string, std::shared_ptr<Tiled2dMapVectorLayerConfig>> layerConfigs;

    const std::shared_ptr<FontLoaderInterface> fontLoader;

    std::unordered_map<std::string, Actor<Tiled2dMapVectorSourceTileDataManager>> sourceDataManagers;
    std::unordered_map<std::string, Actor<Tiled2dMapVectorSourceSymbolDataManager>> symbolSourceDataManagers;
    Actor<Tiled2dMapVectorSourceSymbolCollisionManager> collisionManager;
    std::atomic_flag prevCollisionStillValid;
    std::atomic_flag tilesStillValid;
    std::shared_ptr<Tiled2dMapVectorInteractionManager> interactionManager;

    long long lastDataManagerUpdate = 0;
    long long lastCollitionCheck = 0;
    double lastDataManagerZoom = 0;
    bool isAnimating = false;

    std::shared_ptr<Tiled2dMapVectorLayerSelectionCallbackInterface> strongSelectionDelegate;
    std::weak_ptr<Tiled2dMapVectorLayerSelectionCallbackInterface> selectionDelegate;
    std::shared_ptr<Tiled2dMapVectorLayerLocalDataProviderInterface> localDataProvider;

    std::recursive_mutex renderPassMutex;
    std::vector<std::shared_ptr<RenderPassInterface>> currentRenderPasses;

    struct SourceRenderDescriptions {
        std::vector<std::shared_ptr<TileRenderDescription>> renderDescriptions;
        std::vector<std::shared_ptr<TileRenderDescription>> symbolRenderDescriptions;
    };

    std::unordered_map<std::string, SourceRenderDescriptions> sourceRenderDescriptionMap;

    std::atomic_bool isResumed = false;

    float alpha = 1.0;
    std::optional<::RectI> scissorRect = std::nullopt;

    std::shared_ptr<SpriteData> spriteData;
    std::shared_ptr<::TextureHolderInterface> spriteTexture;

    std::shared_ptr<Tiled2dMapVectorStateManager> featureStateManager;
    std::shared_ptr<Tiled2dMapVectorLayerSymbolDelegateInterface> symbolDelegate;

    void updateReadyStateListenerIfNeeded();

    std::optional<LayerReadyState> lastReadyState;
    std::shared_ptr<::Tiled2dMapReadyStateListener> readyStateListener;

   std::mutex setupMutex;
   std::condition_variable setupCV;
   bool setupReady = false;
   bool animationsEnabled = true;
};


