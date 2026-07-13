/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "CoordinatesUtil.h"
#include "DateHelper.h"
#include "HashedTuple.h"
#include "DisplacedTerrainTiled2dMap3dTileSelection.h"
#include "Tiled2dMapSource.h"
#include "TiledLayerError.h"

#include "Matrix.h"
#include "PolygonCoord.h"
#include "TrigonometryLUT.h"
#include "Vec3DHelper.h"
#include "gpc.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

template <class L, class R>
Tiled2dMapSource<L, R>::Tiled2dMapSource(const MapConfig &mapConfig, const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                         const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                         const std::shared_ptr<SchedulerInterface> &scheduler, float screenDensityPpi,
                                         size_t loaderCount, std::string layerName)
    : mapConfig(mapConfig)
    , layerConfig(layerConfig)
    , conversionHelper(conversionHelper)
    , scheduler(scheduler)
    , zoomLevelInfos(layerConfig->getZoomLevelInfos())
    , zoomInfo(layerConfig->getZoomInfo())
    , layerSystemId(layerConfig->getCoordinateSystemIdentifier())
    , isPaused(false)
    , screenDensityPpi(screenDensityPpi)
    , layerName(layerName)
    , curT(std::numeric_limits<decltype(curT)>::lowest())
    , curZoom(std::numeric_limits<decltype(curZoom)>::lowest())
    , loadingQueues(loaderCount)
    , tileSelection(makeDefaultTiled2dMap3dTileSelection<Tiled2dMapSource<L, R>>()) {
    std::sort(zoomLevelInfos.begin(), zoomLevelInfos.end(),
              [](const Tiled2dMapZoomLevelInfo &a, const Tiled2dMapZoomLevelInfo &b) -> bool { return a.zoom > b.zoom; });
    topMostZoomLevel = zoomLevelInfos.empty() ? 0 : zoomLevelInfos.begin()->zoomLevelIdentifier;

    // add virtual zoom levels and sort again
    auto virtualZoomLevelInfos = layerConfig->getVirtualZoomLevelInfos();
    zoomLevelInfosWithVirtual.insert(zoomLevelInfosWithVirtual.end(), zoomLevelInfos.begin(), zoomLevelInfos.end());
    zoomLevelInfosWithVirtual.insert(zoomLevelInfosWithVirtual.end(), virtualZoomLevelInfos.begin(), virtualZoomLevelInfos.end());
    std::sort(zoomLevelInfosWithVirtual.begin(), zoomLevelInfosWithVirtual.end(),
              [](const Tiled2dMapZoomLevelInfo &a, const Tiled2dMapZoomLevelInfo &b) -> bool { return a.zoom > b.zoom; });
    initializeZoomLevelGeometryWithVirtual();
}

template <class L, class R> void Tiled2dMapSource<L, R>::initializeZoomLevelGeometryWithVirtual() {
    zoomLevelGeometryWithVirtual.clear();
    zoomLevelGeometryWithVirtual.reserve(zoomLevelInfosWithVirtual.size());

    for (const auto &level : zoomLevelInfosWithVirtual) {
        const double boundsRatio = std::abs(((level.bounds.bottomRight.y - level.bounds.topLeft.y) / level.numTilesY) /
                                            ((level.bounds.bottomRight.x - level.bounds.topLeft.x) / level.numTilesX));
        const double tileWidth = level.tileWidthLayerSystemUnits;
        const double tileHeight = tileWidth * boundsRatio;

        RectCoord layerBounds = conversionHelper->convertRect(layerSystemId, level.bounds);
        const bool leftToRight = layerBounds.topLeft.x < layerBounds.bottomRight.x;
        const bool topToBottom = layerBounds.topLeft.y < layerBounds.bottomRight.y;
        const double tileWidthAdj = leftToRight ? tileWidth : -tileWidth;
        const double tileHeightAdj = topToBottom ? tileHeight : -tileHeight;

        zoomLevelGeometryWithVirtual.push_back({tileWidthAdj, tileHeightAdj, layerBounds.topLeft.x, layerBounds.topLeft.y});
    }
}

const static double VIEWBOUNDS_PADDING_MIN_DIM_PC = 0.15;
const static int8_t ALWAYS_KEEP_LEVEL_TARGET_ZOOM_OFFSET = -8;

template <class L, class R> bool Tiled2dMapSource<L, R>::isTileVisible(const Tiled2dMapTileInfo &tileInfo) {
    return currentVisibleTiles.count(tileInfo) > 0;
}

template <class L, class R> void Tiled2dMapSource<L, R>::setMaskTileGeometryTileSelectionOptimizationEnabled(bool enabled) {
    if (maskTileGeometryTileSelectionOptimizationEnabled == enabled) {
        return;
    }

    maskTileGeometryTileSelectionOptimizationEnabled = enabled;
    lastVisibleTilesHash = -1;
    lastCameraInputHash = -1;
}

template <class L, class R>
::Vec3D Tiled2dMapSource<L, R>::transformToView(const ::Coord &position, const std::vector<float> &viewMatrix,
                                                const Vec3D &origin) {

    const auto &mapCoord = conversionHelper->convertToRenderSystem(position);

    const double rx = origin.x;
    const double ry = origin.y;
    const double rz = origin.z;

    double sinX, cosX, sinY, cosY;
    lut::sincos(mapCoord.y, sinY, cosY);
    lut::sincos(mapCoord.x, sinX, cosX);

    static thread_local std::vector<float> inVec(4);
    static thread_local std::vector<float> outVec(4);

    inVec[0] = (float)((mapCoord.z * sinY * cosX - rx));
    inVec[1] = (float)((mapCoord.z * cosY - ry));
    inVec[2] = (float)((-mapCoord.z * sinY * sinX - rz));
    inVec[3] = 1.0f;

    Matrix::multiply(viewMatrix, inVec, outVec);

    return Vec3D(outVec[0] / outVec[3], outVec[1] / outVec[3], outVec[2] / outVec[3]);
}

template <class L, class R>
::Vec3D Tiled2dMapSource<L, R>::projectToScreen(const ::Vec3D &position, const std::vector<float> &projectionMatrix) {
    static thread_local std::vector<float> inVec(4);
    static thread_local std::vector<float> outVec(4);

    inVec[0] = (float)position.x;
    inVec[1] = (float)position.y;
    inVec[2] = (float)position.z;
    inVec[3] = 1.0f;

    Matrix::multiply(projectionMatrix, inVec, outVec);

    return Vec3D(outVec[0] / outVec[3], outVec[1] / outVec[3], outVec[2] / outVec[3]);
}

template <class L, class R>
void Tiled2dMapSource<L, R>::onCameraChange(const std::vector<float> &viewMatrix, const std::vector<float> &projectionMatrix,
                                            const ::Vec3D &origin, float verticalFov, float horizontalFov, float width,
                                            float height, float focusPointAltitude, const ::Coord &focusPointPosition, float zoom,
                                            const std::optional<::Vec3D> &cameraPosition, ::MapCamera3dMode cameraMode) {
    get3dTileSelection().onCameraChange(*this, viewMatrix, projectionMatrix, origin, verticalFov, horizontalFov, width, height,
                                        focusPointAltitude, focusPointPosition, zoom, cameraPosition, cameraMode);
}

template <class L, class R>
void Tiled2dMapSource<L, R>::onVisibleBoundsChanged(const ::RectCoord &visibleBounds, int curT_, double zoom) {
    if (isPaused || isTileLoadingPaused) {
        return;
    }

    std::vector<PrioritizedTiled2dMapTileInfo> visibleTilesVec;

    RectCoord visibleBoundsLayer = conversionHelper->convertRect(layerSystemId, visibleBounds);

    if (((currentViewBoundsRect && visibleBoundsLayer != *currentViewBoundsRect) || curT != curT_)) {
        for (auto it = currentlyLoading.begin(); it != currentlyLoading.end();) {
            if (it->first.t != curT_) {
                cancelLoad(it->first, it->second);
                it = currentlyLoading.erase(it);
            } else
                it++;
        }
    }
    curT = curT_;
    curZoom = zoom;

    const auto dataBounds = layerConfig->getBounds();

    double centerVisibleX = visibleBoundsLayer.topLeft.x + 0.5 * (visibleBoundsLayer.bottomRight.x - visibleBoundsLayer.topLeft.x);
    double centerVisibleY = visibleBoundsLayer.topLeft.y + 0.5 * (visibleBoundsLayer.bottomRight.y - visibleBoundsLayer.topLeft.y);

    size_t numZoomLevels = zoomLevelInfos.size();
    int targetZoomLayer = -1;

    // Each pixel is assumed to be 0.28mm – https://gis.stackexchange.com/a/315989
    const float screenScaleFactor = zoomInfo.adaptScaleToScreen ? screenDensityPpi / (0.0254 / 0.00028) : 1.0;

    if (!zoomInfo.underzoom &&
        (zoomLevelInfos.empty() || zoomLevelInfos[0].zoom * zoomInfo.zoomLevelScaleFactor * screenScaleFactor < zoom) &&
        (zoomLevelInfos.empty() ||
         zoomLevelInfos[0].zoomLevelIdentifier != 0)) { // enable underzoom if the first zoomLevel is zoomLevelIdentifier == 0
        if (lastVisibleTilesHash != 0) {
            lastVisibleTilesHash = 0;
            onVisibleTilesChanged({}, false);
        }
        return;
    }

    for (int i = 0; i < numZoomLevels; i++) {
        const Tiled2dMapZoomLevelInfo &zoomLevelInfo = zoomLevelInfos.at(i);
        if (zoomInfo.zoomLevelScaleFactor * screenScaleFactor * zoomLevelInfo.zoom < zoom) {
            targetZoomLayer = std::max(i - 1, 0);
            break;
        }
    }
    if (targetZoomLayer < 0) {
        if (!zoomInfo.overzoom) {
            onVisibleTilesChanged({}, false);
            return;
        }
        targetZoomLayer = (int)numZoomLevels - 1;
    }

    int targetZoomLevelIdentifier = zoomLevelInfos.at(targetZoomLayer).zoomLevelIdentifier;
    int startZoomLayer = 0;
    int endZoomLevel = std::min((int)numZoomLevels - 1, targetZoomLayer + 2);
    int keepZoomLevelOffset = std::max(zoomLevelInfos.at(startZoomLayer).zoomLevelIdentifier,
                                       zoomLevelInfos.at(endZoomLevel).zoomLevelIdentifier + ALWAYS_KEEP_LEVEL_TARGET_ZOOM_OFFSET) -
                              targetZoomLevelIdentifier;

    int distanceWeight = 100;
    bool prioritizeTime = true;
    int zoomLevelWeight = (prioritizeTime ? 100000 : 1000) * zoomLevelInfos.at(0).numTilesT;
    int zDistanceWeight = (prioritizeTime ? 1000 : 100000) * zoomLevelInfos.at(0).numTilesT;
    int onScreenWeight = 10000000 * zoomLevelInfos.at(0).numTilesT;

    std::vector<VisibleTilesLayer> layers;

    double visibleWidth = visibleBoundsLayer.bottomRight.x - visibleBoundsLayer.topLeft.x;
    double visibleHeight = visibleBoundsLayer.topLeft.y - visibleBoundsLayer.bottomRight.y;
    double viewboundsPadding = VIEWBOUNDS_PADDING_MIN_DIM_PC * std::min(std::abs(visibleWidth), std::abs(visibleHeight));

    double signWidth = visibleWidth / std::abs(visibleWidth);
    const double visibleLeft = visibleBoundsLayer.topLeft.x - signWidth * viewboundsPadding;
    const double visibleRight = visibleBoundsLayer.bottomRight.x + signWidth * viewboundsPadding;
    visibleWidth = std::abs(visibleWidth) + 2 * viewboundsPadding;

    double signHeight = visibleHeight / std::abs(visibleHeight);
    const double visibleTop = visibleBoundsLayer.topLeft.y + signHeight * viewboundsPadding;
    const double visibleBottom = visibleBoundsLayer.bottomRight.y - signHeight * viewboundsPadding;
    visibleHeight = std::abs(visibleHeight) + 2 * viewboundsPadding;

    size_t visibleTileHash = targetZoomLevelIdentifier;

    for (int i = startZoomLayer; i <= endZoomLevel; i++) {
        const Tiled2dMapZoomLevelInfo &zoomLevelInfo = zoomLevelInfos.at(i);

        // If there is only one zoom level (zoomLevel 0), we disregard the min and max zoomLevel settings.
        // This is because a GeoJSON with only points inherently has zoomLevel 0, and restricting the zoom level
        // in such cases wouldn't be meaningful. Therefore, we skip the zoom level checks when startZoomLayer
        // and endZoomLevel are both zero.
        if (!(startZoomLayer == 0 && endZoomLevel == 0)) {
            if (minZoomLevelIdentifier.has_value() && zoomLevelInfo.zoomLevelIdentifier < minZoomLevelIdentifier) {
                continue;
            }
            if (maxZoomLevelIdentifier.has_value() && zoomLevelInfo.zoomLevelIdentifier > maxZoomLevelIdentifier) {
                continue;
            }
        }

        VisibleTilesLayer curVisibleTiles(i - targetZoomLayer, curT);
        std::vector<PrioritizedTiled2dMapTileInfo> curVisibleTilesVec;

        const double boundsRatio =
            std::abs(((zoomLevelInfo.bounds.bottomRight.y - zoomLevelInfo.bounds.topLeft.y) / zoomLevelInfo.numTilesY) /
                     ((zoomLevelInfo.bounds.bottomRight.x - zoomLevelInfo.bounds.topLeft.x) / zoomLevelInfo.numTilesX));
        const double tileWidth = zoomLevelInfo.tileWidthLayerSystemUnits;
        const double tileHeight = zoomLevelInfo.tileWidthLayerSystemUnits * boundsRatio;

        int zoomDistanceFactor = std::abs(zoomLevelInfo.zoomLevelIdentifier - targetZoomLevelIdentifier);

        RectCoord layerBounds = zoomLevelInfo.bounds;
        layerBounds = conversionHelper->convertRect(layerSystemId, layerBounds);

        const bool leftToRight = layerBounds.topLeft.x < layerBounds.bottomRight.x;
        const bool topToBottom = layerBounds.topLeft.y < layerBounds.bottomRight.y;
        const double tileWidthAdj = leftToRight ? tileWidth : -tileWidth;
        const double tileHeightAdj = (topToBottom ? tileHeight : -tileHeight);

        const double boundsLeft = layerBounds.topLeft.x;
        int startTileLeft =
            std::floor(std::max(leftToRight ? (visibleLeft - boundsLeft) : (boundsLeft - visibleLeft), 0.0) / tileWidth);
        int onScreenStartTileLeft = std::floor(std::max(leftToRight ? visibleLeft : -visibleLeft, 0.0) / tileWidth);
        int maxTileLeft =
            std::floor(std::max(leftToRight ? (visibleRight - boundsLeft) : (boundsLeft - visibleRight), 0.0) / tileWidth);
        int onScreenMaxTileLeft = std::floor(std::max(leftToRight ? visibleRight : -visibleRight, 0.0) / tileWidth);

        const double boundsTop = layerBounds.topLeft.y;
        int startTileTop =
            std::floor(std::max(topToBottom ? (visibleTop - boundsTop) : (boundsTop - visibleTop), 0.0) / tileHeight);
        int onScreenStartTileTop = std::floor(std::max(topToBottom ? visibleTop : -visibleTop, 0.0) / tileHeight);
        int maxTileTop =
            std::floor(std::max(topToBottom ? (visibleBottom - boundsTop) : (boundsTop - visibleBottom), 0.0) / tileHeight);
        int onScreenMaxTileTop = std::floor(std::max(topToBottom ? visibleBottom : -visibleBottom, 0.0) / tileHeight);

        if (dataBounds.has_value()) {
            const auto availableTiles = conversionHelper->convertRect(layerSystemId, *dataBounds);

            const double tLength = tileWidth / 256;
            const double tHeight = tileHeight / 256;
            // const double tWidthAdj = leftToRight ? tLength : -tLength;
            // const double tHeightAdj = topToBottom ? tHeight : -tHeight;
            const double originX = leftToRight ? zoomLevelInfo.bounds.topLeft.x : -zoomLevelInfo.bounds.bottomRight.x;
            const double originY = topToBottom ? zoomLevelInfo.bounds.bottomRight.y : -zoomLevelInfo.bounds.topLeft.y;
            const double minAvailableX = leftToRight ? std::min(availableTiles.topLeft.x, availableTiles.bottomRight.x)
                                                     : -std::max(availableTiles.topLeft.x, availableTiles.bottomRight.x);
            const double minAvailableY = topToBottom ? std::min(availableTiles.topLeft.y, availableTiles.bottomRight.y)
                                                     : -std::max(availableTiles.topLeft.y, availableTiles.bottomRight.y);
            const double maxAvailableX = leftToRight ? std::max(availableTiles.topLeft.x, availableTiles.bottomRight.x)
                                                     : -std::min(availableTiles.topLeft.x, availableTiles.bottomRight.x);
            const double maxAvailableY = topToBottom ? std::max(availableTiles.topLeft.y, availableTiles.bottomRight.y)
                                                     : -std::min(availableTiles.topLeft.y, availableTiles.bottomRight.y);

            int min_left_pixel = floor((minAvailableX - originX) / tLength);
            int min_left = std::max(0, min_left_pixel / 256);

            int max_left_pixel = floor((maxAvailableX - originX) / tLength);
            int max_left = std::min(zoomLevelInfo.numTilesX, max_left_pixel / 256);

            int min_top_pixel = floor((minAvailableY - originY) / tHeight);
            int min_top = std::max(0, min_top_pixel / 256);

            int max_top_pixel = floor((maxAvailableY - originY) / tHeight);
            int max_top = std::min(zoomLevelInfo.numTilesY, max_top_pixel / 256);

            startTileLeft = std::max(min_left, startTileLeft);
            maxTileLeft = std::min(max_left, maxTileLeft);
            startTileTop = std::max(min_top, startTileTop);
            maxTileTop = std::min(max_top, maxTileTop);
        }

        const double maxDisCenterX = visibleWidth * 0.5 + tileWidth;
        const double maxDisCenterY = visibleHeight * 0.5 + tileHeight;
        const double maxDisCenter = std::sqrt(maxDisCenterX * maxDisCenterX + maxDisCenterY * maxDisCenterY);

        std::hash_combine(visibleTileHash, std::hash<int>{}(i));
        std::hash_combine(visibleTileHash, std::hash<int>{}(startTileLeft));
        std::hash_combine(visibleTileHash, std::hash<int>{}(maxTileLeft));
        std::hash_combine(visibleTileHash, std::hash<int>{}(startTileTop));
        std::hash_combine(visibleTileHash, std::hash<int>{}(maxTileTop));
        std::hash_combine(visibleTileHash, std::hash<int>{}(curT));

        for (int x = startTileLeft; x <= maxTileLeft && x < zoomLevelInfo.numTilesX; x++) {
            for (int y = startTileTop; y <= maxTileTop && y < zoomLevelInfo.numTilesY; y++) {
                for (int t = 0; t < zoomLevelInfo.numTilesT; t++) {

                    if (zoomDistanceFactor > 0 && t != curT) {
                        // only consider tiles of current zoom if t is different
                        continue;
                    }

                    const Coord topLeft = Coord(layerSystemId, x * tileWidthAdj + boundsLeft, y * tileHeightAdj + boundsTop, 0);
                    const Coord bottomRight = Coord(layerSystemId, topLeft.x + tileWidthAdj, topLeft.y + tileHeightAdj, 0);

                    const double tileCenterX = topLeft.x + 0.5f * tileWidthAdj;
                    const double tileCenterY = topLeft.y + 0.5f * tileHeightAdj;
                    const double tileCenterDis =
                        std::sqrt(std::pow(tileCenterX - centerVisibleX, 2.0) + std::pow(tileCenterY - centerVisibleY, 2.0));

                    float distanceFactor = (tileCenterDis / maxDisCenter) * distanceWeight;
                    float onScreenFactor = 0;
                    if (x < onScreenStartTileLeft || x > onScreenMaxTileLeft || y < onScreenStartTileTop ||
                        y > onScreenMaxTileTop) {
                        onScreenFactor = onScreenWeight;
                    }
                    float zoomlevelFactor = zoomDistanceFactor * zoomLevelWeight;
                    float zDistanceFactor = std::abs(t - curT) * zDistanceWeight;

                    const int priority = std::ceil(distanceFactor + onScreenFactor + zoomlevelFactor + zDistanceFactor);

                    const RectCoord rect(topLeft, bottomRight);
                    curVisibleTilesVec.push_back(PrioritizedTiled2dMapTileInfo(
                        Tiled2dMapTileInfo(rect, x, y, t, zoomLevelInfo.zoomLevelIdentifier, zoomLevelInfo.zoom), priority));

                    visibleTilesVec.push_back(curVisibleTilesVec.back());
                }
            }
        }

        curVisibleTiles.visibleTiles.insert(curVisibleTilesVec.begin(), curVisibleTilesVec.end());

        if (startTileLeft == maxTileLeft && startTileTop == maxTileTop) {
            if (!layers.empty()) {
                layers.back().isLastSingleCover = false;
            }
            curVisibleTiles.isLastSingleCover = true;
        }

        layers.push_back(curVisibleTiles);
    }

    {
        currentZoomLevelIdentifier = targetZoomLevelIdentifier;
    }

    if (lastVisibleTilesHash != visibleTileHash) {
        lastVisibleTilesHash = visibleTileHash;
        onVisibleTilesChanged(layers, false, keepZoomLevelOffset);
    }

    currentViewBounds = {PolygonCoord(
        {visibleBoundsLayer.topLeft,
         Coord(visibleBoundsLayer.topLeft.systemIdentifier, visibleBoundsLayer.bottomRight.x, visibleBoundsLayer.topLeft.y, 0),
         visibleBoundsLayer.bottomRight,
         Coord(visibleBoundsLayer.topLeft.systemIdentifier, visibleBoundsLayer.topLeft.x, visibleBoundsLayer.bottomRight.y, 0)},
        {})};
    currentViewBoundsRect = visibleBoundsLayer;
}

template <class L, class R>
void Tiled2dMapSource<L, R>::onVisibleTilesChanged(const std::vector<VisibleTilesLayer> &pyramid, bool enforceMultipleLevels,
                                                   int keepZoomLevelOffset) {
    currentVisibleTiles.clear();

    size_t pyramidTileCount = 0;
    for (const auto &layer : pyramid) {
        pyramidTileCount += layer.visibleTiles.size();
    }
    currentVisibleTiles.reserve(pyramidTileCount);

    std::vector<PrioritizedTiled2dMapTileInfo> toAdd;
    toAdd.reserve(pyramidTileCount);

    // 3D selection already builds a refinement pyramid ordered by priority. In that mode every
    // generated layer is loading-relevant: sharp pose tiles load first, but coarse parents must also
    // be allowed to load and remain alive so fast camera movement does not expose empty terrain.
    for (const auto &layer : pyramid) {
        if (enforceMultipleLevels ||
            (layer.targetZoomLevelOffset <= 0 && layer.targetZoomLevelOffset >= -zoomInfo.numDrawPreviousLayers) ||
            layer.targetZoomLevelOffset == keepZoomLevelOffset) {
            for (auto const &tileInfo : layer.visibleTiles) {
                if (abs(tileInfo.tileInfo.t - layer.curT) > zoomInfo.numDrawPreviousOrLaterTLayers) {
                    continue;
                }
                currentVisibleTiles.insert(tileInfo.tileInfo);

                size_t currentTilesCount = currentTiles.count(tileInfo.tileInfo);
                size_t currentlyLoadingCount = currentlyLoading.count(tileInfo.tileInfo);
                size_t notFoundCount = notFoundTiles.count(tileInfo.tileInfo);
                // error tiles also don't need to be re-added, they will be retried after the appropriate delay.
                size_t errorTileCount = 0;
                for (auto const &[index, errors] : errorTiles) {
                    errorTileCount += errors.count(tileInfo.tileInfo);
                }

                if (currentTilesCount == 0 && currentlyLoadingCount == 0 && notFoundCount == 0 && errorTileCount == 0) {
                    toAdd.push_back(tileInfo);
                }
            }
        }
    }

    std::unordered_map<Tiled2dMapTileInfo, int> visibleTileTessellationFactors;
    std::unordered_set<Tiled2dMapTileInfo> loadingRelevantTiles;
    std::unordered_set<Tiled2dMapTileInfo> errorVisibleTiles;
    visibleTileTessellationFactors.reserve(pyramidTileCount);
    loadingRelevantTiles.reserve(pyramidTileCount);
    errorVisibleTiles.reserve(pyramidTileCount);

    for (const auto &layer : pyramid) {
        for (auto const &tile : layer.visibleTiles) {
            if (maskTileGeometryTileSelectionOptimizationEnabled && currentVisibleTiles.count(tile.tileInfo) == 0) {
                continue;
            }

            visibleTileTessellationFactors.insert_or_assign(tile.tileInfo, tile.tileInfo.tessellationFactor);
            errorVisibleTiles.insert(tile.tileInfo);

            if (abs(tile.tileInfo.t - layer.curT) <= zoomInfo.numDrawPreviousOrLaterTLayers || layer.isLastSingleCover) {
                loadingRelevantTiles.insert(tile.tileInfo);
            }
        }
    }

    currentPyramid = pyramid;
    currentKeepZoomLevelOffset = keepZoomLevelOffset;

    // we only remove tiles that are not visible anymore directly
    // tile from upper zoom levels will be removed as soon as the correct tiles are loaded if mask tiles is enabled
    std::vector<Tiled2dMapTileInfo> toRemove;

    int currentZoomLevelIdentifier = this->currentZoomLevelIdentifier;
    bool onlyCurrent = !zoomInfo.maskTile && zoomInfo.numDrawPreviousLayers == 0;
    for (auto &[tileInfo, tileWrapper] : currentTiles) {
        bool found = false;

        if ((!onlyCurrent && tileInfo.zoomIdentifier <= currentZoomLevelIdentifier) ||
            (onlyCurrent && tileInfo.zoomIdentifier == currentZoomLevelIdentifier) ||
            tileInfo.zoomIdentifier == (currentZoomLevelIdentifier + keepZoomLevelOffset) || enforceMultipleLevels) {
            auto visibleTileIt = visibleTileTessellationFactors.find(tileInfo);
            if (visibleTileIt != visibleTileTessellationFactors.end()) {
                found = true;
                tileWrapper.tessellationFactor = visibleTileIt->second;
            }
        }

        if (!found && shouldRetainTileUntilReplacementReady(tileInfo, pyramid)) {
            retainedFallbackTiles.insert(tileInfo);
            continue;
        }

        retainedFallbackTiles.erase(tileInfo);
        if (!found) {
            toRemove.push_back(tileInfo);
        }
    }

    for (const auto &removedTile : toRemove) {
        currentTiles.erase(removedTile);
        currentlyLoading.erase(removedTile);
        retainedFallbackTiles.erase(removedTile);

        readyTiles.erase(removedTile);

        if (errorManager)
            errorManager->removeError(
                layerConfig->getTileUrl(removedTile.x, removedTile.y, removedTile.t, removedTile.zoomIdentifier));
    }

    for (auto it = currentlyLoading.begin(); it != currentlyLoading.end();) {
        bool found = false;
        if (it->first.zoomIdentifier <= currentZoomLevelIdentifier) {
            found = loadingRelevantTiles.count(it->first) != 0;
        }

        if (!found) {
            cancelLoad(it->first, it->second);
            it = currentlyLoading.erase(it);
        } else
            it++;
    }

    for (auto &[loaderIndex, errors] : errorTiles) {
        for (auto it = errors.begin(); it != errors.end();) {
            const bool visible = errorVisibleTiles.count(it->first) != 0;
            if (!visible) {
                if (errorManager)
                    errorManager->removeError(
                        layerConfig->getTileUrl(it->first.x, it->first.y, it->first.t, it->first.zoomIdentifier));
                it = errors.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::sort(toAdd.begin(), toAdd.end());
    for (auto &queue : loadingQueues) {
        queue.clear();
    }
    if (loadingQueues.empty()) {
        // no loader (typically this means that the map layer setup is broken)
        for (auto &t : toAdd) {
            notFoundTiles.insert(t.tileInfo);
        }
    } else {
        for (auto &t : toAdd) {
            loadingQueues[0].push_back(t.tileInfo);
        }
    }
    scheduleFixedNumberOfLoadingTasks();
    // if we removed tiles, we potentially need to update the tilemasks - also if no new tile is loaded
    updateTileMasks();

    notifyTilesUpdates();
}

template <class L, class R>
bool Tiled2dMapSource<L, R>::shouldRetainTileUntilReplacementReady(const Tiled2dMapTileInfo &tileInfo,
                                                                   const std::vector<VisibleTilesLayer> &pyramid) const {
    if (!get3dTileDetailSelector().retainsTilesUntilReplacementReady()) {
        return false;
    }

    auto rectsIntersect = [](const RectCoord &lhs, const RectCoord &rhs) {
        const double lhsMinX = std::min(lhs.topLeft.x, lhs.bottomRight.x);
        const double lhsMaxX = std::max(lhs.topLeft.x, lhs.bottomRight.x);
        const double lhsMinY = std::min(lhs.topLeft.y, lhs.bottomRight.y);
        const double lhsMaxY = std::max(lhs.topLeft.y, lhs.bottomRight.y);
        const double rhsMinX = std::min(rhs.topLeft.x, rhs.bottomRight.x);
        const double rhsMaxX = std::max(rhs.topLeft.x, rhs.bottomRight.x);
        const double rhsMinY = std::min(rhs.topLeft.y, rhs.bottomRight.y);
        const double rhsMaxY = std::max(rhs.topLeft.y, rhs.bottomRight.y);

        return lhsMinX < rhsMaxX && lhsMaxX > rhsMinX && lhsMinY < rhsMaxY && lhsMaxY > rhsMinY;
    };

    for (const auto &layer : pyramid) {
        for (const auto &visibleTile : layer.visibleTiles) {
            const auto &visibleTileInfo = visibleTile.tileInfo;
            if (maskTileGeometryTileSelectionOptimizationEnabled && currentVisibleTiles.count(visibleTileInfo) == 0) {
                continue;
            }
            if (!rectsIntersect(tileInfo.bounds, visibleTileInfo.bounds)) {
                continue;
            }

            // Retention is only useful in the refinement direction: keep a coarse tile visible
            // until sharper children are ready. When zooming out, the replacement is coarser,
            // so retaining many sharper descendants keeps far too many tiles alive.
            if (tileInfo.zoomIdentifier > visibleTileInfo.zoomIdentifier) {
                continue;
            }

            const bool replacementReady = currentTiles.count(visibleTileInfo) != 0 && readyTiles.count(visibleTileInfo) != 0;
            if (!replacementReady) {
                return true;
            }
        }
    }

    return false;
}

template <class L, class R> void Tiled2dMapSource<L, R>::pruneRetainedFallbackTiles() {
    std::vector<Tiled2dMapTileInfo> toRemove;
    for (const auto &tileInfo : retainedFallbackTiles) {
        if (!shouldRetainTileUntilReplacementReady(tileInfo, currentPyramid)) {
            toRemove.push_back(tileInfo);
        }
    }

    if (toRemove.empty()) {
        return;
    }

    for (const auto &tileInfo : toRemove) {
        retainedFallbackTiles.erase(tileInfo);
        currentTiles.erase(tileInfo);
        readyTiles.erase(tileInfo);

        if (errorManager) {
            errorManager->removeError(layerConfig->getTileUrl(tileInfo.x, tileInfo.y, tileInfo.t, tileInfo.zoomIdentifier));
        }
    }

    updateTileMasks();
    notifyTilesUpdates();
}

template <class L, class R> void Tiled2dMapSource<L, R>::scheduleFixedNumberOfLoadingTasks() {
    // strictly prioritize by loader index; the fallback loaders must not block the primary loader.
    // alternatively we could e.g. have a max number of ongoing loads per loader.
    for (size_t loaderIndex = 0; loaderIndex < loadingQueues.size(); ++loaderIndex) {
        auto &queue = loadingQueues[loaderIndex];
        while (!queue.empty() && currentlyLoading.size() < maxConcurrentTileLoads) {
            performLoadingTask(queue.front(), loaderIndex);
            queue.pop_front();
        }
    }
}

template <class L, class R> void Tiled2dMapSource<L, R>::performLoadingTask(Tiled2dMapTileInfo tile, size_t loaderIndex) {
    if (currentlyLoading.count(tile) != 0)
        return;

    if (currentVisibleTiles.count(tile) == 0) {
        errorTiles[loaderIndex].erase(tile);
        return;
    }

    std::weak_ptr<Tiled2dMapSource> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapSource>(shared_from_this());
    auto weakActor = WeakActor<Tiled2dMapSource>(mailbox, std::static_pointer_cast<Tiled2dMapSource>(shared_from_this()));

    currentlyLoading.insert({tile, loaderIndex});
    std::string layerName = layerConfig->getLayerName();
    readyTiles.erase(tile);
    loadDataAsync(tile, loaderIndex).then([weakActor, loaderIndex, tile, weakSelfPtr, layerName](::djinni::Future<L> result) {
        auto strongSelf = weakSelfPtr.lock();
        if (strongSelf) {
            auto res = result.get();
            if (res->status == LoaderStatus::OK) {
                if (strongSelf->hasExpensivePostLoadingTask()) {
                    auto strongScheduler = strongSelf->scheduler.lock();
                    if (strongScheduler) {
                        strongScheduler->addTask(std::make_shared<LambdaTask>(
                            TaskConfig("postLoadingTask", 0.0, TaskPriority::NORMAL, ExecutionEnvironment::COMPUTATION),
                            [tile, loaderIndex, weakSelfPtr, weakActor, res] {
                                auto strongSelf = weakSelfPtr.lock();
                                if (strongSelf) {
                                    auto isStillVisible = weakActor.syncAccess([tile](auto actor) {
                                        auto strongSelf = actor.lock();
                                        return strongSelf ? strongSelf->isTileVisible(tile) : false;
                                    });
                                    if (isStillVisible == false) {
                                        weakActor.message(MFN(&Tiled2dMapSource::didFailToLoad), tile, loaderIndex,
                                                          LoaderStatus::ERROR_OTHER, std::nullopt);
                                    } else {
                                        try {
                                            weakActor.message(MFN(&Tiled2dMapSource::didLoad), tile, loaderIndex,
                                                              strongSelf->postLoadingTask(res, tile));
                                        } catch (const std::exception &e) {
                                            LogError << "Failed post-loading for tile " << tile.to_string_short()
                                                     << " with error " <<= e.what();
                                            weakActor.message(MFN(&Tiled2dMapSource::didFailToLoad), tile, loaderIndex,
                                                              LoaderStatus::ERROR_OTHER, std::nullopt);
                                        }
                                    }
                                }
                            }));
                    }
                } else {
                    try {
                        weakActor.message(MFN(&Tiled2dMapSource::didLoad), tile, loaderIndex,
                                          strongSelf->postLoadingTask(res, tile));
                    } catch (const std::exception &e) {
                        LogError << "Failed post-loading for tile " << tile.to_string_short() << " with error " <<= e.what();
                        weakActor.message(MFN(&Tiled2dMapSource::didFailToLoad), tile, loaderIndex, LoaderStatus::ERROR_OTHER,
                                          std::nullopt);
                    }
                }
            } else {
                weakActor.message(MFN(&Tiled2dMapSource::didFailToLoad), tile, loaderIndex, res->status, res->errorCode);
            }
        }
    });
}

template <class L, class R> void Tiled2dMapSource<L, R>::didLoad(Tiled2dMapTileInfo tile, size_t loaderIndex, const R &result) {
    currentlyLoading.erase(tile);
    scheduleFixedNumberOfLoadingTasks();

    const bool isVisible = currentVisibleTiles.count(tile);
    if (!isVisible) {
        errorTiles[loaderIndex].erase(tile);
        return;
    }

    auto errorManager = this->errorManager;

    if (errorManager) {
        errorManager->removeError(layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier));
    }

    auto bounds = tile.bounds;
    PolygonCoord mask({bounds.topLeft, Coord(bounds.topLeft.systemIdentifier, bounds.bottomRight.x, bounds.topLeft.y, 0),
                       bounds.bottomRight, Coord(bounds.topLeft.systemIdentifier, bounds.topLeft.x, bounds.bottomRight.y, 0),
                       bounds.topLeft},
                      {});

    GPCPolygonHolder tilePolygon;
    gpc_set_polygon({mask}, tilePolygon.set());

    currentTiles.emplace(
        tile, TileWrapper<R>(result, std::vector<::PolygonCoord>{}, mask, std::move(tilePolygon), tile.tessellationFactor));

    errorTiles[loaderIndex].erase(tile);

    updateTileMasks();

    notifyTilesUpdates();
}

template <class L, class R>
void Tiled2dMapSource<L, R>::didFailToLoad(Tiled2dMapTileInfo tile, size_t loaderIndex, const LoaderStatus &status,
                                           const std::optional<std::string> &errorCode) {
    currentlyLoading.erase(tile);

    const bool isVisible = currentVisibleTiles.count(tile);
    if (!isVisible) {
        errorTiles[loaderIndex].erase(tile);
        scheduleFixedNumberOfLoadingTasks();
        return;
    }

    auto errorManager = this->errorManager;

    switch (status) {
    case LoaderStatus::OK: {
        assert(false);
        break;
    }
    case LoaderStatus::NOOP: {
        errorTiles[loaderIndex].erase(tile);

        auto newLoaderIndex = loaderIndex + 1;
        if (newLoaderIndex < loadingQueues.size()) {
            loadingQueues[newLoaderIndex].push_back(tile);
            break;
        } else {
            [[fallthrough]]; // no more loaders, treat this same as not found.
        }
    }
    case LoaderStatus::ERROR_400:
    case LoaderStatus::ERROR_404: {
        notFoundTiles.insert(tile);

        errorTiles[loaderIndex].erase(tile);

        if (errorManager) {
            errorManager->addTiledLayerError(TiledLayerError(status, errorCode, layerName,
                                                             layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier),
                                                             false, tile.bounds));
        }
        break;
    }

    case LoaderStatus::ERROR_TIMEOUT:
    case LoaderStatus::ERROR_OTHER:
    case LoaderStatus::ERROR_NETWORK: {
        const auto now = DateHelper::currentTimeMillis();
        int64_t delay = 0;
        if (errorTiles[loaderIndex].count(tile) != 0) {
            errorTiles[loaderIndex].at(tile).lastLoad = now;
            errorTiles[loaderIndex].at(tile).delay = std::min(2 * errorTiles[loaderIndex].at(tile).delay, MAX_WAIT_TIME);
        } else {
            errorTiles[loaderIndex][tile] = {now, MIN_WAIT_TIME};
        }
        delay = errorTiles[loaderIndex].at(tile).delay;

        if (errorManager) {
            errorManager->addTiledLayerError(TiledLayerError(status, errorCode, layerConfig->getLayerName(),
                                                             layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier),
                                                             true, tile.bounds));
        }

        if (!nextDelayTaskExecution || nextDelayTaskExecution > now + delay) {
            nextDelayTaskExecution = now + delay;

            auto taskIdentifier = "Tiled2dMapSource_loadingErrorTask";

            auto strongScheduler = scheduler.lock();
            if (strongScheduler) {
                auto weakActor =
                    WeakActor<Tiled2dMapSource>(mailbox, std::dynamic_pointer_cast<Tiled2dMapSource>(shared_from_this()));
                strongScheduler->addTask(
                    std::make_shared<LambdaTask>(TaskConfig(taskIdentifier, delay, TaskPriority::NORMAL, ExecutionEnvironment::IO),
                                                 [weakActor] { weakActor.message(MFN(&Tiled2dMapSource::performDelayedTasks)); }));
            }
        }
        break;
    }
    }
    scheduleFixedNumberOfLoadingTasks();
}

template <class L, class R> void Tiled2dMapSource<L, R>::performDelayedTasks() {
    nextDelayTaskExecution = std::nullopt;

    const auto now = DateHelper::currentTimeMillis();
    int64_t minDelay = std::numeric_limits<int64_t>::max();

    std::vector<std::pair<int, Tiled2dMapTileInfo>> toLoad;

    for (auto &[loaderIndex, errors] : errorTiles) {
        for (auto &[tile, errorInfo] : errors) {
            const auto retryAt = errorInfo.lastLoad + errorInfo.delay;
            if (retryAt <= now) {
                toLoad.push_back({loaderIndex, tile});
            } else {
                minDelay = std::min(minDelay, retryAt - now);
            }
        }
    }

    for (auto &[loaderIndex, tile] : toLoad) {
        performLoadingTask(tile, loaderIndex);
    }

    if (minDelay != std::numeric_limits<int64_t>::max()) {
        nextDelayTaskExecution = now + minDelay;

        auto taskIdentifier = "Tiled2dMapSource_loadingErrorTask";

        auto strongScheduler = scheduler.lock();
        if (strongScheduler) {
            auto weakActor = WeakActor<Tiled2dMapSource>(mailbox, std::dynamic_pointer_cast<Tiled2dMapSource>(shared_from_this()));
            strongScheduler->addTask(
                std::make_shared<LambdaTask>(TaskConfig(taskIdentifier, minDelay, TaskPriority::NORMAL, ExecutionEnvironment::IO),
                                             [weakActor] { weakActor.message(MFN(&Tiled2dMapSource::performDelayedTasks)); }));
        }
    }
}

template <class L, class R> void Tiled2dMapSource<L, R>::updateTileMasks() {

    if (!zoomInfo.maskTile) {
        for (auto it = currentTiles.rbegin(); it != currentTiles.rend(); it++) {
            auto &[tileInfo, tileWrapper] = *it;
            if (readyTiles.count(tileInfo) == 0) {
                tileWrapper.state = TileState::IN_SETUP;
            } else {
                tileWrapper.state = TileState::VISIBLE;
            }
        }
        return;
    }

    if (maskTileGeometryTileSelectionOptimizationEnabled) {
        if (currentTiles.empty() && outdatedTiles.empty()) {
            return;
        }

        GPCPolygonHolder currentTileMask;

        GPCPolygonHolder currentViewBoundsPolygon;
        gpc_set_polygons(currentViewBounds, currentViewBoundsPolygon.set());

        auto addTileToMask = [&](const TileWrapper<R> &tileWrapper) {
            GPCPolygonHolder tilePolygon;
            gpc_set_polygon({tileWrapper.tileBounds}, tilePolygon.set());
            if (!currentTileMask) {
                currentTileMask = std::move(tilePolygon);
                return;
            }

            GPCPolygonHolder nextTileMask;
            gpc_polygon_clip(GPC_UNION, currentTileMask.get(), tilePolygon.get(), nextTileMask.set());
            currentTileMask = std::move(nextTileMask);
        };

        auto applyTileMask = [&](const Tiled2dMapTileInfo &tileInfo, TileWrapper<R> &tileWrapper, bool currentVisible) {
            GPCPolygonHolder polygonDiffOwned;
            gpc_polygon *polygonDiff = tileWrapper.tilePolygon.get();
            if (currentTileMask) {
                gpc_polygon_clip(GPC_DIFF, tileWrapper.tilePolygon.get(), currentTileMask.get(), polygonDiffOwned.set());
                polygonDiff = polygonDiffOwned.get();
            }

            if (polygonDiff->contour == nullptr) {
                tileWrapper.state = TileState::CACHED;
                return;
            }

            if (!currentVisible) {
                GPCPolygonHolder resultingMask;
                gpc_polygon_clip(GPC_INT, polygonDiff, currentViewBoundsPolygon.get(), resultingMask.set());
                if (!resultingMask) {
                    tileWrapper.state = TileState::CACHED;
                    return;
                }
            }

            tileWrapper.masks = currentTileMask ? gpc_get_polygon_coord(polygonDiff, tileInfo.bounds.topLeft.systemIdentifier)
                                                : std::vector<PolygonCoord>{tileWrapper.tileBounds};
            tileWrapper.state = TileState::VISIBLE;
            addTileToMask(tileWrapper);
        };

        for (auto it = currentTiles.rbegin(); it != currentTiles.rend(); it++) {
            auto &[tileInfo, tileWrapper] = *it;
            tileWrapper.state = TileState::CACHED;
            if (readyTiles.count(tileInfo) == 0) {
                tileWrapper.state = TileState::IN_SETUP;
                continue;
            }

            if (currentVisibleTiles.count(tileInfo) != 0) {
                applyTileMask(tileInfo, tileWrapper, true);
            }
        }

        for (auto it = currentTiles.rbegin(); it != currentTiles.rend(); it++) {
            auto &[tileInfo, tileWrapper] = *it;
            if (currentVisibleTiles.count(tileInfo) != 0 || readyTiles.count(tileInfo) == 0) {
                continue;
            }

            applyTileMask(tileInfo, tileWrapper, false);
        }

        return;
    }

    if (currentTiles.empty() && outdatedTiles.empty()) {
        return;
    }

    int currentZoomLevelIdentifier = this->currentZoomLevelIdentifier;

    GPCPolygonHolder currentTileMask;
    bool isFirst = true;

    GPCPolygonHolder currentViewBoundsPolygon;
    gpc_set_polygons(currentViewBounds, currentViewBoundsPolygon.set());

    bool completeViewBoundsDrawn = false;

    for (auto it = currentTiles.rbegin(); it != currentTiles.rend(); it++) {
        auto &[tileInfo, tileWrapper] = *it;

        tileWrapper.state = TileState::VISIBLE;

        if (readyTiles.count(tileInfo) == 0) {
            tileWrapper.state = TileState::IN_SETUP;
            continue;
        }

        if (tileInfo.zoomIdentifier != currentZoomLevelIdentifier) {

            if (currentTileMask) {
                if (!completeViewBoundsDrawn) {
                    GPCPolygonHolder diff;
                    gpc_polygon_clip(GPC_DIFF, currentViewBoundsPolygon.get(), currentTileMask.get(), diff.set());

                    if (!diff) {
                        completeViewBoundsDrawn = true;
                    }
                }
            }

            if (completeViewBoundsDrawn) {
                tileWrapper.state = TileState::CACHED;
                continue;
            }

            GPCPolygonHolder polygonDiffOwned;
            gpc_polygon *polygonDiff;
            if (currentTileMask) {
                gpc_polygon_clip(GPC_DIFF, tileWrapper.tilePolygon.get(), currentTileMask.get(), polygonDiffOwned.set());
                polygonDiff = polygonDiffOwned.get();
            } else {
                polygonDiff = tileWrapper.tilePolygon.get();
            }

            if (polygonDiff->contour == NULL) {
                tileWrapper.state = TileState::CACHED;
                continue;
            } else {
                GPCPolygonHolder resultingMask;
                gpc_polygon_clip(GPC_INT, polygonDiff, currentViewBoundsPolygon.get(), resultingMask.set());

                if (!resultingMask) {
                    tileWrapper.state = TileState::CACHED;
                    continue;
                } else {
                    tileWrapper.masks = gpc_get_polygon_coord(polygonDiff, tileInfo.bounds.topLeft.systemIdentifier);
                }
            }
        } else {
            tileWrapper.masks = {tileWrapper.tileBounds};
        }

        // add tileBounds to currentTileMask
        if (tileWrapper.state == TileState::VISIBLE) {
            if (isFirst) {
                gpc_set_polygon({tileWrapper.tileBounds}, currentTileMask.set());
                isFirst = false;
            } else {
                GPCPolygonHolder result;
                gpc_polygon_clip(GPC_UNION, currentTileMask.get(), tileWrapper.tilePolygon.get(), result.set());
                currentTileMask = std::move(result);
            }
        }
    }
}

template <class L, class R> void Tiled2dMapSource<L, R>::setTileReady(const Tiled2dMapVersionedTileInfo &tile) {
    bool needsUpdate = false;

    if (readyTiles.count(tile.tileInfo) == 0) {
        if (currentTiles.count(tile.tileInfo) != 0) {
            readyTiles.insert(tile.tileInfo);
            outdatedTiles.erase(tile.tileInfo);
            needsUpdate = true;
        }
    }

    if (!needsUpdate) {
        return;
    }

    pruneRetainedFallbackTiles();
    updateTileMasks();

    notifyTilesUpdates();
}

template <class L, class R> void Tiled2dMapSource<L, R>::setTilesReady(const std::vector<Tiled2dMapVersionedTileInfo> &tiles) {
    bool needsUpdate = false;

    for (auto const &tile : tiles) {
        if (readyTiles.count(tile.tileInfo) == 0 || outdatedTiles.count(tile.tileInfo) > 0) {
            const auto &tileEntry = currentTiles.find(tile.tileInfo);
            if (tileEntry != currentTiles.end()) {
                if (!zoomInfo.maskTile) {
                    tileEntry->second.state = TileState::VISIBLE;
                }
                readyTiles.insert(tile.tileInfo);
                outdatedTiles.erase(tile.tileInfo);
                needsUpdate = true;
            }
        }
    }

    if (!needsUpdate) {
        return;
    }

    pruneRetainedFallbackTiles();
    updateTileMasks();
    notifyTilesUpdates();
}

template <class L, class R> void Tiled2dMapSource<L, R>::setMinZoomLevelIdentifier(std::optional<int32_t> value) {
    minZoomLevelIdentifier = value;
}

template <class L, class R> void Tiled2dMapSource<L, R>::setMaxZoomLevelIdentifier(std::optional<int32_t> value) {
    maxZoomLevelIdentifier = value;
}

template <class L, class R> std::optional<int32_t> Tiled2dMapSource<L, R>::getMinZoomLevelIdentifier() {
    return minZoomLevelIdentifier;
}

template <class L, class R> std::optional<int32_t> Tiled2dMapSource<L, R>::getMaxZoomLevelIdentifier() {
    return maxZoomLevelIdentifier;
}

template <class L, class R> void Tiled2dMapSource<L, R>::setZoomLevelScaleFactor(float value) {
    const float clampedValue = std::clamp(value, 0.05f, 8.0f);
    if (std::abs(zoomInfo.zoomLevelScaleFactor - clampedValue) < 0.0001f) {
        return;
    }

    zoomInfo.zoomLevelScaleFactor = clampedValue;
    lastVisibleTilesHash = -1;
    lastCameraInputHash = -1;
}

template <class L, class R> void Tiled2dMapSource<L, R>::pause() { isPaused = true; }

template <class L, class R> void Tiled2dMapSource<L, R>::resume() { isPaused = false; }

template <class L, class R> void Tiled2dMapSource<L, R>::setTileLoadingPaused(bool paused) {
    isTileLoadingPaused = paused;
    if (!paused) {
        // Force the next onCameraChange to run a full selection walk after the pause.
        lastCameraInputHash = -1;
    }
}

template <class L, class R>::LayerReadyState Tiled2dMapSource<L, R>::isReadyToRenderOffscreen() {
    if (notFoundTiles.size() > 0) {
        return LayerReadyState::ERROR;
    }

    for (auto const &[index, errors] : errorTiles) {
        if (errors.size() > 0) {
            return LayerReadyState::ERROR;
        }
    }

    if (!currentlyLoading.empty()) {
        return LayerReadyState::NOT_READY;
    }

    for (const auto &visible : currentVisibleTiles) {
        if (currentTiles.count(visible) == 0) {
            return LayerReadyState::NOT_READY;
        }
        if (readyTiles.count(visible) == 0) {
            return LayerReadyState::NOT_READY;
        }
    }

    if (!mailbox->isEmpty()) {
        return LayerReadyState::NOT_READY;
    }

    return LayerReadyState::READY;
}

template <class L, class R> void Tiled2dMapSource<L, R>::setErrorManager(const std::shared_ptr<::ErrorManager> &errorManager) {
    this->errorManager = errorManager;
}

template <class L, class R> void Tiled2dMapSource<L, R>::forceReload() {

    // set delay to 0 for all error tiles
    std::vector<std::pair<Tiled2dMapTileInfo, size_t>> newLoadingTasks;
    for (auto &[loaderIndex, errors] : errorTiles) {
        for (auto &[tile, errorInfo] : errors) {
            errorInfo.delay = 1;
            newLoadingTasks.emplace_back(tile, loaderIndex);
        }
    }
    for (const auto &[tile, loaderIndex] : newLoadingTasks) {
        performLoadingTask(tile, loaderIndex);
    }

    onVisibleTilesChanged(currentPyramid, false, currentKeepZoomLevelOffset);
}

template <class L, class R> void Tiled2dMapSource<L, R>::reloadTiles() {
    outdatedTiles.clear();
    outdatedTiles.swap(currentTiles);
    retainedFallbackTiles.clear();
    readyTiles.clear();

    for (auto it = currentlyLoading.begin(); it != currentlyLoading.end(); ++it) {
        cancelLoad(it->first, it->second);
    }
    currentlyLoading.clear();
    errorTiles.clear();

    lastVisibleTilesHash = -1;
    lastCameraInputHash = -1;
    onVisibleTilesChanged(currentPyramid, false, currentKeepZoomLevelOffset);
}
