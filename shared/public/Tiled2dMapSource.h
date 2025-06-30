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

#include "Actor.h"
#include "Coord.h"
#include "CoordinateConversionHelperInterface.h"
#include "CoordinateSystemIdentifiers.h"
#include "Future.hpp"
#include "LambdaTask.h"
#include "LoaderStatus.h"
#include "MapConfig.h"
#include "PolygonCoord.h"
#include "PrioritizedTiled2dMapTileInfo.h"
#include "QuadCoord.h"
#include "SchedulerInterface.h"
#include "TileState.h"
#include "Tiled2dMapLayerConfig.h"
#include "Tiled2dMapSourceInterface.h"
#include "Tiled2dMapVersionedTileInfo.h"
#include "Tiled2dMapZoomInfo.h"
#include "Tiled2dMapZoomLevelInfo.h"
#include "Vec3D.h"
#include "Vec3F.h"
#include "gpc.h"
#include <atomic>
#include <cmath>
#include <map>
#include <mutex>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <random>

// Move-only owning holder for a gpc_polygon.
// Calls gpc_free_polygon on destruction.
class GPCPolygonHolder {
  private:
    gpc_polygon poly;

  public:
    GPCPolygonHolder() noexcept
        : poly{} {}
    ~GPCPolygonHolder() { reset(); }
    GPCPolygonHolder(GPCPolygonHolder &&other) noexcept
        : poly{} {
        other.swap(*this);
    }
    GPCPolygonHolder &operator=(GPCPolygonHolder &&other) noexcept {
        other.swap(*this);
        return *this;
    }

    // No copy
    GPCPolygonHolder(GPCPolygonHolder const &) = delete;
    GPCPolygonHolder &operator=(GPCPolygonHolder const &) = delete;

    void swap(GPCPolygonHolder &other) noexcept { std::swap(poly, other.poly); }

    // Get gpc_polygon pointer for an _output_ parameter of a gpc_... call.
    // Frees currently owned polygon first.
    gpc_polygon *set() {
        reset();
        return &poly;
    }

    // Get gpc_polygon pointer for input parameter of a gpc_... call.
    gpc_polygon *get() { return &poly; }
    const gpc_polygon *get() const { return &poly; }

    operator bool() const {
        assert((poly.num_contours == 0) == (poly.contour == NULL));
        return poly.num_contours != 0;
    }

    void reset() {
        if (poly.num_contours) {
            gpc_free_polygon(&poly);
        }
        poly = {};
    }
};

template <class R> struct TileWrapper {
  public:
    const R result;
    std::vector<::PolygonCoord> masks;
    const PolygonCoord tileBounds;
    GPCPolygonHolder tilePolygon;
    TileState state = TileState::IN_SETUP;
    int tessellationFactor;

    TileWrapper(const R &result, const std::vector<::PolygonCoord> &masks, const PolygonCoord &tileBounds,
                GPCPolygonHolder &&tilePolygon, int tessellationFactor)
        : result(std::move(result))
        , masks(std::move(masks))
        , tileBounds(std::move(tileBounds))
        , tilePolygon(std::move(tilePolygon))
        , tessellationFactor(tessellationFactor){};
};

class Tiled2dMapSourceReadyInterface {
  public:
    virtual ~Tiled2dMapSourceReadyInterface() = default;

    virtual void setTileReady(const Tiled2dMapVersionedTileInfo &tile) = 0;
};

// L is the Loading type; std::shared_ptr to either DataLoaderResult or TextureLoaderResult (or compatible class).
// R is the Result type
//
// NOTE: only include the implementation header Tile2dMapSourceImpl.h
// selectively to avoid including this in too many compilation units.
// Instantiate the template with the chosen type parameters for the derived class,
// e.g. at the bottom of the .cpp of the derived class:
//
//  #include <Tiled2dMapSourceImpl.h>
//  template class Tiled2dMapSource<std::shared_ptr<FooLoaderResult>, FooResultType>;
template <class L, class R>
class Tiled2dMapSource : public Tiled2dMapSourceInterface,
                         public Tiled2dMapSourceReadyInterface,
                         public std::enable_shared_from_this<Tiled2dMapSourceInterface>,
                         public ActorObject {
  public:
    Tiled2dMapSource(const MapConfig &mapConfig, const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                     const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                     const std::shared_ptr<SchedulerInterface> &scheduler, float screenDensityPpi, size_t loaderCount,
                     std::string layerName);

    virtual void onVisibleBoundsChanged(const ::RectCoord &visibleBounds, int curT, double zoom) override;

    virtual void onCameraChange(const std::vector<float> &viewMatrix, const std::vector<float> &projectionMatrix,
                                const ::Vec3D &origin, float verticalFov, float horizontalFov, float width, float height,
                                float focusPointAltitude, const ::Coord &focusPointPosition, float zoom) override;

    virtual bool isTileVisible(const Tiled2dMapTileInfo &tileInfo);

    virtual void pause() override;

    virtual void resume() override;

    void setMinZoomLevelIdentifier(std::optional<int32_t> value) override;

    void setMaxZoomLevelIdentifier(std::optional<int32_t> value) override;

    std::optional<int32_t> getMinZoomLevelIdentifier() override;

    std::optional<int32_t> getMaxZoomLevelIdentifier() override;

    virtual ::LayerReadyState isReadyToRenderOffscreen() override;

    virtual void setErrorManager(const std::shared_ptr<::ErrorManager> &errorManager) override;

    virtual void forceReload() override;

    virtual void reloadTiles();

    void setTileReady(const Tiled2dMapVersionedTileInfo &tile) override;

    void setTilesReady(const std::vector<Tiled2dMapVersionedTileInfo> &tiles);

    virtual void cancelLoad(Tiled2dMapTileInfo tile, size_t loaderIndex) = 0;

    virtual ::djinni::Future<L> loadDataAsync(Tiled2dMapTileInfo tile, size_t loaderIndex) = 0;

    void didLoad(Tiled2dMapTileInfo tile, size_t loaderIndex, const R &result);

    void didFailToLoad(Tiled2dMapTileInfo tile, size_t loaderIndex, const LoaderStatus &status,
                       const std::optional<std::string> &errorCode);

  protected:
    virtual bool hasExpensivePostLoadingTask() = 0;

    virtual R postLoadingTask(L loadedData, Tiled2dMapTileInfo tile) = 0;

    ::Vec3D transformToView(const ::Coord &position, const std::vector<float> &vpMatrix, const Vec3D &origin);
    ::Vec3D projectToScreen(const ::Vec3D &point, const std::vector<float> &vpMatrix);

    MapConfig mapConfig;
    std::shared_ptr<Tiled2dMapLayerConfig> layerConfig;
    int32_t layerSystemId;
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
    std::weak_ptr<SchedulerInterface> scheduler;
    std::shared_ptr<::ErrorManager> errorManager;

    std::vector<Tiled2dMapZoomLevelInfo> zoomLevelInfos;
    std::vector<Tiled2dMapZoomLevelInfo> zoomLevelInfosWithVirtual;
    const Tiled2dMapZoomInfo zoomInfo;
    int topMostZoomLevel;

    std::optional<int32_t> minZoomLevelIdentifier;
    std::optional<int32_t> maxZoomLevelIdentifier;

    std::map<Tiled2dMapTileInfo, TileWrapper<R>> currentTiles;
    std::map<Tiled2dMapTileInfo, TileWrapper<R>> outdatedTiles;

    std::unordered_set<Tiled2dMapTileInfo> loadingTiles;
    std::mutex loadingTilesMutex;

    int currentZoomLevelIdentifier = 0;

    int curT;
    double curZoom;

    std::unordered_set<Tiled2dMapTileInfo> currentVisibleTiles;

    std::vector<VisibleTilesLayer> currentPyramid;
    int currentKeepZoomLevelOffset;
    std::vector<std::vector<Tiled2dMapTileInfo>> loadingQueues;

    std::vector<PolygonCoord> currentViewBounds = {};
    std::optional<RectCoord> currentViewBoundsRect = std::nullopt;

    std::atomic<bool> isPaused;

    float screenDensityPpi;
    std::set<Tiled2dMapTileInfo> readyTiles;

    size_t lastVisibleTilesHash = -1;

    void onVisibleTilesChanged(const std::vector<VisibleTilesLayer> &pyramid, bool keepMultipleLevels, int keepZoomLevelOffset = 0);

  protected:
    void scheduleFixedNumberOfLoadingTasks();
    void performLoadingTask(Tiled2dMapTileInfo tile, size_t loaderIndex);

    void scheduleRetries();

    void updateTileMasks();

    std::unordered_map<Tiled2dMapTileInfo, int> currentlyLoading;

    const long long RETRY_FREQUENCY = 1000;
    const long long RETRY_MIN_WAIT_TIME = 500;

    struct ErrorInfo {
        long long lastLoad; //!< timestamp of last end of tile load
        bool loading; //!< tile loading is currently being retried
    };

    std::unordered_map<size_t, std::map<Tiled2dMapTileInfo, ErrorInfo>> errorTiles;
    std::optional<long long> nextRetryTaskExecution;

    std::unordered_set<Tiled2dMapTileInfo> notFoundTiles;

    std::string layerName;

  private:
    std::default_random_engine retryRandomGen;
};
