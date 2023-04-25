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

#include "Coord.h"
#include "CoordinateConversionHelperInterface.h"
#include "LambdaTask.h"
#include "MapConfig.h"
#include "PrioritizedTiled2dMapTileInfo.h"
#include "SchedulerInterface.h"
#include "Tiled2dMapLayerConfig.h"
#include "Tiled2dMapSourceInterface.h"
#include "Tiled2dMapZoomInfo.h"
#include "Tiled2dMapZoomLevelInfo.h"
#include "QuadCoord.h"
#include "PolygonCoord.h"
#include "CoordinateSystemIdentifiers.h"
#include <atomic>
#include <cmath>
#include <mutex>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include "gpc.h"
#include "Actor.h"
#include "Future.hpp"
#include "LoaderStatus.h"
#include "Vec3D.h"

template<class R>
struct TileWrapper {
public:
    const R result;
    std::vector<::PolygonCoord> masks;
    const PolygonCoord tileBounds;
    gpc_polygon tilePolygon;
    bool isVisible = true;

    TileWrapper(const R &result,
                const std::vector<::PolygonCoord> & masks,
                const PolygonCoord & tileBounds,
                const gpc_polygon &tilePolygon) :
    result(std::move(result)),
    masks(std::move(masks)),
    tileBounds(std::move(tileBounds)),
    tilePolygon(std::move(tilePolygon)) {};
};


// T is the Object used for loading
// L is the Loading type
// R is the Result type
template<class T, class L, class R>
class Tiled2dMapSource :
        public Tiled2dMapSourceInterface,
        public std::enable_shared_from_this<Tiled2dMapSourceInterface>,
        public ActorObject {
public:
    Tiled2dMapSource(const MapConfig &mapConfig, const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                     const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                     const std::shared_ptr<SchedulerInterface> &scheduler,
                     float screenDensityPpi,
                     size_t loaderCount);

    virtual void onVisibleBoundsChanged(const ::RectCoord &visibleBounds, int curT, double zoom) override;

    virtual void onCameraChange(const std::vector<float> & viewMatrix, const std::vector<float> & projectionMatrix, float verticalFov, float horizontalFov, float width, float height) override;

    ::Vec3D transformToView(const ::Coord & position, const std::vector<float> & vpMatrix);
    ::Vec3D projectToScreen(const ::Vec3D & point, const std::vector<float> & vpMatrix);

    virtual bool isTileVisible(const Tiled2dMapTileInfo &tileInfo);

    virtual void pause() override;

    virtual void resume() override;

    virtual RectCoord getCurrentViewBounds();

    void setMinZoomLevelIdentifier(std::optional<int32_t> value) override;

    void setMaxZoomLevelIdentifier(std::optional<int32_t> value) override;

    std::optional<int32_t> getMinZoomLevelIdentifier() override;

    std::optional<int32_t> getMaxZoomLevelIdentifier() override;

    virtual ::LayerReadyState isReadyToRenderOffscreen() override;

    virtual void setErrorManager(const std::shared_ptr<::ErrorManager> &errorManager) override;

    virtual void forceReload() override;

    void setTileReady(const Tiled2dMapTileInfo &tile);

    void setTilesReady(const std::vector<const Tiled2dMapTileInfo> &tiles);
            
    virtual void cancelLoad(Tiled2dMapTileInfo tile, size_t loaderIndex) = 0;
            
    virtual ::djinni::Future<L> loadDataAsync(Tiled2dMapTileInfo tile, size_t loaderIndex) = 0;
            
    void didLoad(Tiled2dMapTileInfo tile, size_t loaderIndex, const L &loaderResult);

    virtual LoaderStatus getLoaderStatus(const L &loaderResult) = 0;
    virtual std::optional<std::string> getErrorCode(const L &loaderResult) = 0;

  protected:

    virtual R postLoadingTask(const L &loadedData, const Tiled2dMapTileInfo &tile) = 0;

    MapConfig mapConfig;
    std::shared_ptr<Tiled2dMapLayerConfig> layerConfig;
    std::string layerSystemId;
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
    std::shared_ptr<SchedulerInterface> scheduler;
    std::shared_ptr<::ErrorManager> errorManager;

    std::vector<Tiled2dMapZoomLevelInfo> zoomLevelInfos;
    const Tiled2dMapZoomInfo zoomInfo;

    std::optional<int32_t> minZoomLevelIdentifier;
    std::optional<int32_t> maxZoomLevelIdentifier;

    std::map<Tiled2dMapTileInfo, TileWrapper<R>> currentTiles;

    std::unordered_set<Tiled2dMapTileInfo> currentVisibleTiles;

    std::vector<VisibleTilesLayer> currentPyramid;

    RectCoord currentViewBounds = RectCoord(Coord(CoordinateSystemIdentifiers::RENDERSYSTEM(), 0.0, 0.0, 0.0),
                                            Coord(CoordinateSystemIdentifiers::RENDERSYSTEM(), 0.0, 0.0, 0.0));


    std::atomic<bool> isPaused;

    float screenDensityPpi;
    std::set<Tiled2dMapTileInfo> readyTiles;

private:
    void performLoadingTask(Tiled2dMapTileInfo tile, size_t loaderIndex);

    void onVisibleTilesChanged(const std::vector<VisibleTilesLayer> &pyramid);

    void updateTileMasks();

    std::unordered_map<Tiled2dMapTileInfo, int> currentlyLoading;

    const long long MAX_WAIT_TIME = 32000;
    const long long MIN_WAIT_TIME = 1000;

    struct ErrorInfo {
        long long lastLoad;
        long long delay;
    };

    std::unordered_map<size_t, std::map<Tiled2dMapTileInfo, ErrorInfo>> errorTiles;

    std::unordered_set<Tiled2dMapTileInfo> notFoundTiles;
};

#include "Tiled2dMapSourceImpl.h"
