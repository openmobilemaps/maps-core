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

#include "CoordinateSystemFactory.h"
#include "PolygonCoord.h"
#include "Tiled2dMap3dTileSelection.h"
#include "Tiled2dMap3dTileSelectionHelpers.h"
#include "Tiled2dMapSource.h"
#include "Vec2DHelper.h"
#include "gpc.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <memory>
#include <queue>
#include <unordered_set>

template <class Source> class DefaultTiled2dMap3dTileSelection final : public Tiled2dMap3dTileSelection<Source> {
  public:
    void onCameraChange(Source &source, const std::vector<float> &viewMatrix, const std::vector<float> &projectionMatrix,
                        const ::Vec3D &origin, float verticalFov, float horizontalFov, float width, float height,
                        float focusPointAltitude, const ::Coord &focusPointPosition, float /*zoom*/,
                        const std::optional<::Vec3D> & /*cameraPosition*/, ::MapCamera3dMode /*cameraMode*/) const override {
        auto &mapConfig = source.mapConfig;
        auto &layerConfig = source.layerConfig;
        auto &conversionHelper = source.conversionHelper;
        auto &zoomLevelInfosWithVirtual = source.zoomLevelInfosWithVirtual;
        auto &zoomLevelGeometryWithVirtual = source.zoomLevelGeometryWithVirtual;
        auto &zoomInfo = source.zoomInfo;
        auto &layerSystemId = source.layerSystemId;
        auto &topMostZoomLevel = source.topMostZoomLevel;
        auto &currentViewBounds = source.currentViewBounds;
        auto &currentZoomLevelIdentifier = source.currentZoomLevelIdentifier;
        auto &isPaused = source.isPaused;
        auto &isTileLoadingPaused = source.isTileLoadingPaused;
        auto &lastVisibleTilesHash = source.lastVisibleTilesHash;
        auto &layerName = source.layerName;

        auto transformToView = [&source](const ::Coord &position, const std::vector<float> &matrix, const Vec3D &matrixOrigin) {
            return source.transformToView(position, matrix, matrixOrigin);
        };
        auto projectToScreen = [&source](const ::Vec3D &position, const std::vector<float> &matrix) {
            return source.projectToScreen(position, matrix);
        };
        auto onVisibleTilesChanged = [&source](const std::vector<VisibleTilesLayer> &pyramid, bool keepMultipleLevels) {
            source.onVisibleTilesChanged(pyramid, keepMultipleLevels);
        };

        if (isPaused || isTileLoadingPaused) {
            return;
        }

        if (width <= 0 || height <= 0) {
            return;
        }

        std::queue<VisibleTileCandidate> candidates;
        std::unordered_set<VisibleTileCandidate> candidatesSet;

        bool validViewBounds = false;

        int minNumTiles = layerSystemId == CoordinateSystemIdentifiers::EPSG4326() ? 0 : 1;

        int maxLevel = 0;
        int minZoomLevelIndex = 0;
        for (int index = 0; index < zoomLevelInfosWithVirtual.size(); ++index) {
            const auto &level = zoomLevelInfosWithVirtual[index];
            if (level.numTilesX > minNumTiles && level.numTilesY > minNumTiles) {
                if (level.numTilesX * level.numTilesY > 100) {
                    printf("Ignore seed candidates for %d x %d tiles for %s\n", level.numTilesX, level.numTilesY,
                           layerName.c_str());
                    break;
                }
                for (int x = 0; x < level.numTilesX; x++) {
                    for (int y = 0; y < level.numTilesY; y++) {
                        VisibleTileCandidate c;
                        c.levelIndex = index;
                        c.x = x;
                        c.y = y;

                        candidates.push(c);
                    }
                }
                maxLevel = level.zoomLevelIdentifier;
                minZoomLevelIndex = level.zoomLevelIdentifier;
                break;
            }
        }

        const bool shouldComputeCurrentViewBounds = zoomInfo.maskTile;
        gpc_polygon currentViewBoundsPolygon = {};
        if (shouldComputeCurrentViewBounds) {
            gpc_set_polygon({PolygonCoord(
                                {
                                    conversionHelper->convert(layerSystemId, Coord(4326, -180, 90, 0)),  // top left
                                    conversionHelper->convert(layerSystemId, Coord(4326, 180, 90, 0)),   // top right
                                    conversionHelper->convert(layerSystemId, Coord(4326, 180, -90, 0)),  // bottom right
                                    conversionHelper->convert(layerSystemId, Coord(4326, -180, -90, 0)), // bottom left
                                    conversionHelper->convert(layerSystemId, Coord(4326, -180, 90, 0))   // top left
                                },
                                {})},
                            &currentViewBoundsPolygon);
        }
        auto clipCandidateFromViewBounds = [&](const Coord &topLeft, const Coord &topRight, const Coord &bottomRight,
                                               const Coord &bottomLeft) {
            if (!shouldComputeCurrentViewBounds) {
                return;
            }

            gpc_polygon currentTilePolygon = {};
            gpc_set_polygon({PolygonCoord({topLeft, topRight, bottomRight, bottomLeft, topLeft}, {})}, &currentTilePolygon);
            gpc_polygon_clip(GPC_DIFF, &currentViewBoundsPolygon, &currentTilePolygon, &currentViewBoundsPolygon);
            gpc_free_polygon(&currentTilePolygon);
        };

        size_t visibleTileHash = minZoomLevelIndex;
        std::vector<std::pair<VisibleTileCandidate, PrioritizedTiled2dMapTileInfo>> visibleTilesVec;

        auto maxLevelAvailable = zoomLevelInfosWithVirtual.size() - 1;

        int candidateChecks = 0;

        const int maxCandidateChecks = 1000;
        auto focusPointInLayerCoords = conversionHelper->convert(layerSystemId, focusPointPosition);

        auto earthCenterView = transformToView(Coord(CoordinateSystemIdentifiers::UnitSphere(), 0, 0, 0), viewMatrix, origin);

        while (candidates.size() > 0) {
            VisibleTileCandidate candidate = candidates.front();
            candidates.pop();
            candidatesSet.erase(candidate);

            candidateChecks++;

            if (candidateChecks > maxCandidateChecks) {
                // Stop expanding further, but still publish the best hierarchy gathered so far.
                break;
            }

            const Tiled2dMapZoomLevelInfo &zoomLevelInfo = zoomLevelInfosWithVirtual.at(candidate.levelIndex);
            const auto &levelGeometry = zoomLevelGeometryWithVirtual.at(candidate.levelIndex);
            const double tileWidthAdj = levelGeometry.tileWidthAdj;
            const double tileHeightAdj = levelGeometry.tileHeightAdj;
            const double boundsLeft = levelGeometry.boundsLeft;
            const double boundsTop = levelGeometry.boundsTop;

            const Coord topLeft = Coord(layerSystemId, candidate.x * tileWidthAdj + boundsLeft,
                                        candidate.y * tileHeightAdj + boundsTop, focusPointAltitude);
            const Coord topRight = Coord(layerSystemId, topLeft.x + tileWidthAdj, topLeft.y, focusPointAltitude);
            const Coord bottomLeft = Coord(layerSystemId, topLeft.x, topLeft.y + tileHeightAdj, focusPointAltitude);
            const Coord bottomRight = Coord(layerSystemId, topLeft.x + tileWidthAdj, topLeft.y + tileHeightAdj, focusPointAltitude);

            const Coord tileCenter = Coord(layerSystemId, topLeft.x * 0.5 + bottomRight.x * 0.5,
                                           topLeft.y * 0.5 + bottomRight.y * 0.5, topLeft.z * 0.5 + bottomRight.z * 0.5);

            const double sampleSize = 0.25;
            const auto focusPointClampedToTile =
                Coord(layerSystemId,
                      topLeft.x < topRight.x ? std::clamp(focusPointInLayerCoords.x, topLeft.x, topRight.x)
                                             : std::clamp(focusPointInLayerCoords.x, topRight.x, topLeft.x),
                      topLeft.y < bottomLeft.y ? std::clamp(focusPointInLayerCoords.y, topLeft.y, bottomLeft.y)
                                               : std::clamp(focusPointInLayerCoords.y, bottomLeft.y, topLeft.y),
                      focusPointAltitude);

            auto toRight = focusPointClampedToTile.x < tileCenter.x;
            auto toTop = focusPointClampedToTile.y < tileCenter.y;

            const auto focusPointSampleX =
                Coord(layerSystemId, focusPointClampedToTile.x + (toRight ? tileWidthAdj : -tileWidthAdj) * sampleSize,
                      focusPointClampedToTile.y, focusPointClampedToTile.z);

            const auto focusPointSampleY =
                Coord(layerSystemId, focusPointClampedToTile.x,
                      focusPointClampedToTile.y + (toTop ? -tileHeightAdj : tileHeightAdj) * sampleSize, focusPointClampedToTile.z);

            const Coord topCenter = Coord(layerSystemId, topLeft.x * 0.5 + topRight.x * 0.5, topLeft.y, focusPointAltitude);
            const Coord bottomCenter =
                Coord(layerSystemId, bottomLeft.x * 0.5 + bottomRight.x * 0.5, bottomLeft.y, focusPointAltitude);
            const Coord leftCenter = Coord(layerSystemId, topLeft.x, bottomLeft.y * 0.5 + topLeft.y * 0.5, focusPointAltitude);
            const Coord rightCenter = Coord(layerSystemId, topRight.x, bottomRight.y * 0.5 + topRight.y * 0.5, focusPointAltitude);

            auto topLeftView = transformToView(topLeft, viewMatrix, origin);
            auto topRightView = transformToView(topRight, viewMatrix, origin);
            auto bottomLeftView = transformToView(bottomLeft, viewMatrix, origin);
            auto bottomRightView = transformToView(bottomRight, viewMatrix, origin);

            /*
             use focuspoint in layersystem and clamp to tileBounds
             */

            auto focusPointClampedView = transformToView(focusPointClampedToTile, viewMatrix, origin);
            auto focusPointSampleXView = transformToView(focusPointSampleX, viewMatrix, origin);
            auto focusPointSampleYView = transformToView(focusPointSampleY, viewMatrix, origin);

            auto topCenterView = transformToView(topCenter, viewMatrix, origin);
            auto bottomCenterView = transformToView(bottomCenter, viewMatrix, origin);
            auto leftCenterView = transformToView(leftCenter, viewMatrix, origin);
            auto rightCenterView = transformToView(rightCenter, viewMatrix, origin);

            float centerZ = (topLeftView.z + topRightView.z + bottomLeftView.z + bottomRightView.z) / 4.0;

            bool isKeptLevel = candidate.levelIndex == minZoomLevelIndex;

            const std::array<Vec3D, 8> cullingViews = {topLeftView,   topRightView,     bottomLeftView, bottomRightView,
                                                       topCenterView, bottomCenterView, leftCenterView, rightCenterView};

            auto isFacingAway = [&](const Vec3D &viewPos) { return (viewPos - earthCenterView).z < 0.0; };

            if (!isKeptLevel && std::all_of(cullingViews.begin(), cullingViews.end(), isFacingAway)) {
                clipCandidateFromViewBounds(topLeft, topRight, bottomRight, bottomLeft);
                // LogDebug << "UBCM: dropping tile (all facing away) " << candidate.levelIndex << "/" << candidate.x << "/" <<=
                // candidate.y; Tile is facing away from the camera
                continue;
            }

            auto samplePointOriginViewScreen = projectToScreen(focusPointClampedView, projectionMatrix);
            if (!isKeptLevel && (samplePointOriginViewScreen.x < -1.0 || samplePointOriginViewScreen.x > 1.0 ||
                                 samplePointOriginViewScreen.y < -1.0 || samplePointOriginViewScreen.y > 1.0)) {
                if (mapConfig.mapCoordinateSystem.identifier == CoordinateSystemIdentifiers::UnitSphere()) {
                    // v(0,0,+1) = unit-vector out of screen
                    // 0.5: half of view on each side of center
                    // 1.1: increase angle with padding
                    float fovFactor = 0.5 * 1.1;

                    float left = -horizontalFov * fovFactor;
                    float right = horizontalFov * fovFactor;
                    float top = verticalFov * fovFactor;
                    float bottom = -verticalFov * fovFactor;

                    auto allCameraFacingPointsPass = [&](const auto &predicate) {
                        bool anyCameraFacing = false;
                        for (const auto &viewPos : cullingViews) {
                            if (isFacingAway(viewPos)) {
                                continue;
                            }
                            anyCameraFacing = true;
                            if (!predicate(viewPos)) {
                                return false;
                            }
                        }
                        return anyCameraFacing;
                    };

                    if (allCameraFacingPointsPass(
                            [&](const Vec3D &viewPos) { return 180.0 / M_PI * atan2(viewPos.y, -viewPos.z) < bottom; })) {
                        clipCandidateFromViewBounds(topLeft, topRight, bottomRight, bottomLeft);
                        // LogDebug << "UBCM: dropping tile (below) " << candidate.levelIndex << "/" << candidate.x << "/" <<=
                        // candidate.y;
                        continue; // All camera-facing corners are BELOW the viewport
                    }
                    if (allCameraFacingPointsPass(
                            [&](const Vec3D &viewPos) { return 180.0 / M_PI * atan2(viewPos.x, -viewPos.z) < left; })) {
                        clipCandidateFromViewBounds(topLeft, topRight, bottomRight, bottomLeft);
                        // LogDebug << "UBCM: dropping tile (left) " << candidate.levelIndex << "/" << candidate.x << "/" <<=
                        // candidate.y;
                        continue; // All camera-facing corners are TO THE LEFT of the viewport
                    }
                    if (allCameraFacingPointsPass(
                            [&](const Vec3D &viewPos) { return 180.0 / M_PI * atan2(viewPos.y, -viewPos.z) > top; })) {
                        clipCandidateFromViewBounds(topLeft, topRight, bottomRight, bottomLeft);
                        // LogDebug << "UBCM: dropping tile (above) " << candidate.levelIndex << "/" << candidate.x << "/" <<=
                        // candidate.y;
                        continue; // All camera-facing corners are ABOVE the viewport
                    }
                    if (allCameraFacingPointsPass(
                            [&](const Vec3D &viewPos) { return 180.0 / M_PI * atan2(viewPos.x, -viewPos.z) > right; })) {
                        clipCandidateFromViewBounds(topLeft, topRight, bottomRight, bottomLeft);
                        // LogDebug << "UBCM: dropping tile (right) " << candidate.levelIndex << "/" << candidate.x << "/" <<=
                        // candidate.y;
                        continue; // All camera-facing corners are TO THE RIGHT of the viewport
                    }
                } else {
                    if (std::all_of(cullingViews.begin(), cullingViews.end(),
                                    [&](const Vec3D &viewPos) { return viewPos.x < -width / 2.0; })) {
                        clipCandidateFromViewBounds(topLeft, topRight, bottomRight, bottomLeft);
                        continue;
                    }
                    if (std::all_of(cullingViews.begin(), cullingViews.end(),
                                    [&](const Vec3D &viewPos) { return viewPos.y < -height / 2.0; })) {
                        clipCandidateFromViewBounds(topLeft, topRight, bottomRight, bottomLeft);
                        continue;
                    }
                    if (std::all_of(cullingViews.begin(), cullingViews.end(),
                                    [&](const Vec3D &viewPos) { return viewPos.x > width / 2.0; })) {
                        clipCandidateFromViewBounds(topLeft, topRight, bottomRight, bottomLeft);
                        continue;
                    }
                    if (std::all_of(cullingViews.begin(), cullingViews.end(),
                                    [&](const Vec3D &viewPos) { return viewPos.y > height / 2.0; })) {
                        clipCandidateFromViewBounds(topLeft, topRight, bottomRight, bottomLeft);
                        continue;
                    }
                }
            }

            if (!validViewBounds) {
                validViewBounds = true;
            }

            bool lastLevel = candidate.levelIndex == maxLevelAvailable;
            auto samplePointYViewScreen = projectToScreen(focusPointSampleYView, projectionMatrix);
            auto samplePointXViewScreen = projectToScreen(focusPointSampleXView, projectionMatrix);

            Vec2D samplePointOriginViewScreenPx(samplePointOriginViewScreen.x * (width / 2.0),
                                                samplePointOriginViewScreen.y * (height / 2.0));
            Vec2D samplePointYViewScreenPx(samplePointYViewScreen.x * (width / 2.0), samplePointYViewScreen.y * (height / 2.0));
            Vec2D samplePointXViewScreenPx(samplePointXViewScreen.x * (width / 2.0), samplePointXViewScreen.y * (height / 2.0));

            double xLengthPx = Vec2DHelper::distance(samplePointOriginViewScreenPx, samplePointXViewScreenPx);
            double yLengthPx = Vec2DHelper::distance(samplePointOriginViewScreenPx, samplePointYViewScreenPx);

            double maxLength = sampleSize * (std::min(width, height) * 0.5 / zoomInfo.zoomLevelScaleFactor);
            bool preciseEnough = xLengthPx <= maxLength || yLengthPx <= maxLength;

            bool isVirtual = topMostZoomLevel > zoomLevelInfo.zoomLevelIdentifier;

            if (!isVirtual && (preciseEnough || lastLevel || isKeptLevel)) {
                const RectCoord rect(topLeft, bottomRight);
                int t = 0;
                double priority = -centerZ * 100000;
                visibleTilesVec.push_back(std::make_pair(
                    candidate,
                    PrioritizedTiled2dMapTileInfo(Tiled2dMapTileInfo(rect, candidate.x, candidate.y, t,
                                                                     zoomLevelInfo.zoomLevelIdentifier, zoomLevelInfo.zoom),
                                                  priority)));

                maxLevel = std::max(maxLevel, zoomLevelInfo.zoomLevelIdentifier);
            }

            if (!preciseEnough && !lastLevel) {
                const auto &nextLevelGeometry = zoomLevelGeometryWithVirtual.at(candidate.levelIndex + 1);
                const double tileWidthAdj = nextLevelGeometry.tileWidthAdj;
                const double tileHeightAdj = nextLevelGeometry.tileHeightAdj;
                const double boundsLeft = nextLevelGeometry.boundsLeft;
                const double boundsTop = nextLevelGeometry.boundsTop;

                int nextCandidateXMin = floor((topLeft.x - boundsLeft) / tileWidthAdj);
                int nextCandidateXMax = ceil((topRight.x - boundsLeft) / tileWidthAdj) - 1;

                int nextCandidateYMin = floor((topLeft.y - boundsTop) / tileHeightAdj);
                int nextCandidateYMax = ceil((bottomLeft.y - boundsTop) / tileHeightAdj) - 1;

                for (int nextX = nextCandidateXMin; nextX <= nextCandidateXMax; nextX++) {
                    for (int nextY = nextCandidateYMin; nextY <= nextCandidateYMax; nextY++) {
                        VisibleTileCandidate cNext;
                        cNext.levelIndex = candidate.levelIndex + 1;
                        cNext.x = nextX;
                        cNext.y = nextY;
                        if (candidatesSet.find(cNext) == candidatesSet.end()) {
                            candidates.push(cNext);
                            candidatesSet.insert(cNext);
                        }
                    }
                }
            }
        }

        if (shouldComputeCurrentViewBounds) {
            currentViewBounds = gpc_get_polygon_coord(&currentViewBoundsPolygon, layerSystemId);
            gpc_free_polygon(&currentViewBoundsPolygon);
        }

        if (!validViewBounds) {
            return;
        }

        std::vector<VisibleTilesLayer> layers;

        for (int previousLayerOffset = 0; (previousLayerOffset <= zoomInfo.numDrawPreviousLayers || zoomInfo.maskTile);
             previousLayerOffset++) {

            VisibleTilesLayer curVisibleTiles(-previousLayerOffset, 0);

            std::vector<std::pair<VisibleTileCandidate, PrioritizedTiled2dMapTileInfo>> nextVisibleTilesVec;

            bool allTopMost = true;

            for (auto &tile : visibleTilesVec) {

                const auto dataBounds = layerConfig->getBounds();

                if (dataBounds.has_value()) {
                    const auto availableTiles = conversionHelper->convertRect(layerSystemId, *dataBounds);

                    const Tiled2dMapZoomLevelInfo &zoomLevelInfo = zoomLevelInfosWithVirtual.at(tile.first.levelIndex);

                    RectCoord layerBounds = zoomLevelInfo.bounds;
                    const bool leftToRight = layerBounds.topLeft.x < layerBounds.bottomRight.x;
                    const bool topToBottom = layerBounds.topLeft.y < layerBounds.bottomRight.y;

                    const double boundsRatio =
                        std::abs(((zoomLevelInfo.bounds.bottomRight.y - zoomLevelInfo.bounds.topLeft.y) / zoomLevelInfo.numTilesY) /
                                 ((zoomLevelInfo.bounds.bottomRight.x - zoomLevelInfo.bounds.topLeft.x) / zoomLevelInfo.numTilesX));
                    const double tileWidth = zoomLevelInfo.tileWidthLayerSystemUnits;
                    const double tileHeight = zoomLevelInfo.tileWidthLayerSystemUnits * boundsRatio;
                    const double tLength = tileWidth / 256;
                    const double tHeight = tileHeight / 256;

                    // const double tileWidthAdj = leftToRight ? tileWidth : -tileWidth;
                    // const double tileHeightAdj = topToBottom ? tileHeight : -tileHeight;
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

                    if (tile.second.tileInfo.x < min_left) {
                        continue;
                    }
                    if (tile.second.tileInfo.x > max_left) {
                        continue;
                    }
                    if (tile.second.tileInfo.y < min_top) {
                        continue;
                    }
                    if (tile.second.tileInfo.y > max_top) {
                        continue;
                    }
                }

                tile.second.tileInfo.tessellationFactor = std::min(std::max(0, maxLevel - tile.second.tileInfo.zoomIdentifier), 4);
                curVisibleTiles.visibleTiles.insert(tile.second);

                if (allTopMost && tile.second.tileInfo.zoomIdentifier != topMostZoomLevel) {
                    allTopMost = false;
                }

                hash_combine(visibleTileHash, std::hash<Tiled2dMapTileInfo>{}(tile.second.tileInfo));

                if (tile.first.levelIndex > 0 && (previousLayerOffset < zoomInfo.numDrawPreviousLayers || zoomInfo.maskTile)) {

                    const Tiled2dMapZoomLevelInfo &zoomLevelInfo = zoomLevelInfosWithVirtual.at(tile.first.levelIndex - 1);
                    const double boundsRatio =
                        std::abs(((zoomLevelInfo.bounds.bottomRight.y - zoomLevelInfo.bounds.topLeft.y) / zoomLevelInfo.numTilesY) /
                                 ((zoomLevelInfo.bounds.bottomRight.x - zoomLevelInfo.bounds.topLeft.x) / zoomLevelInfo.numTilesX));
                    const double tileWidth = zoomLevelInfo.tileWidthLayerSystemUnits;
                    const double tileHeight = zoomLevelInfo.tileWidthLayerSystemUnits * boundsRatio;

                    RectCoord layerBounds = zoomLevelInfo.bounds;
                    layerBounds = conversionHelper->convertRect(layerSystemId, layerBounds);

                    const bool leftToRight = layerBounds.topLeft.x < layerBounds.bottomRight.x;
                    const bool topToBottom = layerBounds.topLeft.y < layerBounds.bottomRight.y;
                    const double tileWidthAdj = leftToRight ? tileWidth : -tileWidth;
                    const double tileHeightAdj = topToBottom ? tileHeight : -tileHeight;

                    const double boundsLeft = layerBounds.topLeft.x;
                    const double boundsTop = layerBounds.topLeft.y;

                    VisibleTileCandidate parent;
                    parent.levelIndex = tile.first.levelIndex - 1;
                    parent.x = floor((tile.second.tileInfo.bounds.topLeft.x - boundsLeft) / tileWidthAdj);
                    parent.y = floor((tile.second.tileInfo.bounds.topLeft.y - boundsTop) / tileHeightAdj);

                    const Coord topLeft = Coord(layerSystemId, parent.x * tileWidthAdj + boundsLeft,
                                                parent.y * tileHeightAdj + boundsTop, focusPointAltitude);
                    const Coord bottomRight =
                        Coord(layerSystemId, topLeft.x + tileWidthAdj, topLeft.y + tileHeightAdj, focusPointAltitude);

                    const RectCoord rect(topLeft, bottomRight);
                    int t = 0;
                    double priority = previousLayerOffset * 100000 + tile.second.priority;
                    nextVisibleTilesVec.push_back(std::make_pair(
                        parent,
                        PrioritizedTiled2dMapTileInfo(
                            Tiled2dMapTileInfo(rect, parent.x, parent.y, t, zoomLevelInfo.zoomLevelIdentifier, zoomLevelInfo.zoom),
                            priority)));
                }
            }

            visibleTilesVec = nextVisibleTilesVec;

            layers.push_back(curVisibleTiles);

            if (allTopMost) {
                break;
            }
        }

        currentZoomLevelIdentifier = maxLevel;

        hash_combine(visibleTileHash, std::hash<int>{}(maxLevel));
        hash_combine(visibleTileHash, quantizeHashValue(width, 1.0));
        hash_combine(visibleTileHash, quantizeHashValue(height, 1.0));
        hash_combine(visibleTileHash, quantizeHashValue(verticalFov, 100.0));
        hash_combine(visibleTileHash, quantizeHashValue(horizontalFov, 100.0));

        if (lastVisibleTilesHash != visibleTileHash) {
            lastVisibleTilesHash = visibleTileHash;
            onVisibleTilesChanged(layers, true);
        }
    }
};
