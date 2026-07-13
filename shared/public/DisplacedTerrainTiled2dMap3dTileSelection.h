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
#include "CoordinatesUtil.h"
#include "DefaultTiled2dMap3dTileSelection.h"
#include "HashedTuple.h"
#include "PolygonCoord.h"
#include "Tiled2dMap3dTileSelection.h"
#include "Tiled2dMap3dTileSelectionHelpers.h"
#include "Tiled2dMapSource.h"
#include "Vec2DHelper.h"
#include "Vec3DHelper.h"
#include "gpc.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <queue>
#include <unordered_set>

template <class Source> class DisplacedTerrainTiled2dMap3dTileSelection final : public Tiled2dMap3dTileSelection<Source> {
  public:
    void onCameraChange(Source &source, const std::vector<float> &viewMatrix, const std::vector<float> &projectionMatrix,
                        const ::Vec3D &origin, float verticalFov, float horizontalFov, float width, float height,
                        float focusPointAltitude, const ::Coord &focusPointPosition, float zoom,
                        const std::optional<::Vec3D> &cameraPosition, ::MapCamera3dMode cameraMode) const override {
        auto &mapConfig = source.mapConfig;
        auto &layerConfig = source.layerConfig;
        auto &conversionHelper = source.conversionHelper;
        auto &zoomLevelInfosWithVirtual = source.zoomLevelInfosWithVirtual;
        auto &zoomLevelGeometryWithVirtual = source.zoomLevelGeometryWithVirtual;
        auto &zoomInfo = source.zoomInfo;
        auto &layerSystemId = source.layerSystemId;
        auto &maskTileGeometryTileSelectionOptimizationEnabled = source.maskTileGeometryTileSelectionOptimizationEnabled;
        auto &topMostZoomLevel = source.topMostZoomLevel;
        auto &currentViewBounds = source.currentViewBounds;
        auto &currentZoomLevelIdentifier = source.currentZoomLevelIdentifier;
        auto &isPaused = source.isPaused;
        auto &isTileLoadingPaused = source.isTileLoadingPaused;
        auto &lastVisibleTilesHash = source.lastVisibleTilesHash;
        auto &lastCameraInputHash = source.lastCameraInputHash;
        auto &screenDensityPpi = source.screenDensityPpi;

        auto transformToView = [&source](const ::Coord &position, const std::vector<float> &matrix, const Vec3D &matrixOrigin) {
            return source.transformToView(position, matrix, matrixOrigin);
        };
        auto get3dCullingElevationOffsetMin = [&source]() { return source.get3dCullingElevationOffsetMin(); };
        auto get3dCullingElevationOffsetMax = [&source]() { return source.get3dCullingElevationOffsetMax(); };
        auto get3dTileDetailSelector = [&source]() -> const Tiled2dMap3dTileDetailSelector & {
            return source.get3dTileDetailSelector();
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

        constexpr double pixelHashScale = 1.0;
        constexpr double cameraValueHashScale = 100.0;
        constexpr double coordinateHashScale = 1000000.0;
        constexpr double matrixHashScale = 10000.0;

        constexpr double numericEpsilon = 1e-9;
        constexpr double distanceEpsilon = 1e-6;
        constexpr double minimumDistanceMeters = 1.0;
        constexpr double minimumAspectRatio = 0.1;
        constexpr double minimumViewportPixels = 1.0;
        constexpr double minVerticalFovDegrees = 1.0;
        constexpr double maxVerticalFovDegrees = 140.0;
        constexpr double degreesToRadians = M_PI / 180.0;

        constexpr int maxInitialSeedTiles = 100;
        constexpr double wgs84MinLongitude = -180.0;
        constexpr double wgs84MaxLongitude = 180.0;
        constexpr double wgs84MinLatitude = -90.0;
        constexpr double wgs84MaxLatitude = 90.0;

        constexpr double poseNearCameraRadiusScale = 2.5;
        constexpr double minPoseNearCameraRadiusMeters = 250.0;
        constexpr double maxPoseNearCameraRadiusMeters = 20000.0;
        constexpr double priorityScale = 100000.0;
        constexpr int tilePixelSize = 256;
        constexpr int maxTessellationFactor = 4;

        // Skip the selection walk when the camera inputs are unchanged.
        {
            size_t cameraInputHash = 0;
            std::hash_combine(cameraInputHash, std::hash<::MapCamera3dMode>{}(cameraMode));
            std::hash_combine(cameraInputHash, std::hash<bool>{}(maskTileGeometryTileSelectionOptimizationEnabled));
            std::hash_combine(cameraInputHash, quantizeHashValue(width, pixelHashScale));
            std::hash_combine(cameraInputHash, quantizeHashValue(height, pixelHashScale));
            std::hash_combine(cameraInputHash, quantizeHashValue(verticalFov, cameraValueHashScale));
            std::hash_combine(cameraInputHash, quantizeHashValue(horizontalFov, cameraValueHashScale));
            std::hash_combine(cameraInputHash, quantizeHashValue(zoom, cameraValueHashScale));
            std::hash_combine(cameraInputHash, quantizeHashValue(focusPointAltitude, cameraValueHashScale));
            std::hash_combine(cameraInputHash, quantizeHashValue(focusPointPosition.x, coordinateHashScale));
            std::hash_combine(cameraInputHash, quantizeHashValue(focusPointPosition.y, coordinateHashScale));
            std::hash_combine(cameraInputHash, quantizeHashValue(origin.x, coordinateHashScale));
            std::hash_combine(cameraInputHash, quantizeHashValue(origin.y, coordinateHashScale));
            std::hash_combine(cameraInputHash, quantizeHashValue(origin.z, coordinateHashScale));
            if (cameraPosition.has_value()) {
                std::hash_combine(cameraInputHash, quantizeHashValue(cameraPosition->x, coordinateHashScale));
                std::hash_combine(cameraInputHash, quantizeHashValue(cameraPosition->y, coordinateHashScale));
                std::hash_combine(cameraInputHash, quantizeHashValue(cameraPosition->z, coordinateHashScale));
            }
            for (const auto &matrixValue : viewMatrix) {
                std::hash_combine(cameraInputHash, quantizeHashValue(matrixValue, matrixHashScale));
            }
            for (const auto &matrixValue : projectionMatrix) {
                std::hash_combine(cameraInputHash, quantizeHashValue(matrixValue, matrixHashScale));
            }
            if (cameraInputHash == lastCameraInputHash) {
                return;
            }
            lastCameraInputHash = cameraInputHash;
        }

        // Build a refinement pyramid for 3D raster terrain.
        struct CoveringTileCandidate {
            VisibleTileCandidate tile;
            bool fullyVisible;
        };

        std::vector<CoveringTileCandidate> candidates;
        bool validViewBounds = false;
        int maxLevel = 0;
        int minZoomLevelIndex = 0;

        for (int index = 0; index < zoomLevelInfosWithVirtual.size(); ++index) {
            const auto &level = zoomLevelInfosWithVirtual[index];
            if (level.numTilesX <= 0 || level.numTilesY <= 0) {
                continue;
            }
            if (level.numTilesX * level.numTilesY > maxInitialSeedTiles) {
                break;
            }
            for (int x = 0; x < level.numTilesX; x++) {
                for (int y = 0; y < level.numTilesY; y++) {
                    candidates.push_back({VisibleTileCandidate{x, y, index}, false});
                }
            }
            maxLevel = level.zoomLevelIdentifier;
            minZoomLevelIndex = index;
            break;
        }

        const bool shouldComputeCurrentViewBounds = zoomInfo.maskTile;
        gpc_polygon currentViewBoundsPolygon = {};
        if (shouldComputeCurrentViewBounds) {
            const int32_t wgs84System = CoordinateSystemIdentifiers::EPSG4326();
            gpc_set_polygon(
                {PolygonCoord(
                    {conversionHelper->convert(layerSystemId, Coord(wgs84System, wgs84MinLongitude, wgs84MaxLatitude, 0)),
                     conversionHelper->convert(layerSystemId, Coord(wgs84System, wgs84MaxLongitude, wgs84MaxLatitude, 0)),
                     conversionHelper->convert(layerSystemId, Coord(wgs84System, wgs84MaxLongitude, wgs84MinLatitude, 0)),
                     conversionHelper->convert(layerSystemId, Coord(wgs84System, wgs84MinLongitude, wgs84MinLatitude, 0)),
                     conversionHelper->convert(layerSystemId, Coord(wgs84System, wgs84MinLongitude, wgs84MaxLatitude, 0))},
                    {})},
                &currentViewBoundsPolygon);
        }
        const bool optimizeMaskTileGeometrySelection =
            maskTileGeometryTileSelectionOptimizationEnabled && shouldComputeCurrentViewBounds;
        std::vector<RectCoord> rejectedViewRects;
        auto clipCandidateFromViewBounds = [&](const Coord &topLeft, const Coord &topRight, const Coord &bottomRight,
                                               const Coord &bottomLeft) {
            if (!shouldComputeCurrentViewBounds) {
                return;
            }

            if (optimizeMaskTileGeometrySelection) {
                rejectedViewRects.emplace_back(
                    Coord(topLeft.systemIdentifier, std::min({topLeft.x, topRight.x, bottomRight.x, bottomLeft.x}),
                          std::min({topLeft.y, topRight.y, bottomRight.y, bottomLeft.y}), 0.0),
                    Coord(topLeft.systemIdentifier, std::max({topLeft.x, topRight.x, bottomRight.x, bottomLeft.x}),
                          std::max({topLeft.y, topRight.y, bottomRight.y, bottomLeft.y}), 0.0));
            }

            gpc_polygon currentTilePolygon = {};
            gpc_set_polygon({PolygonCoord({topLeft, topRight, bottomRight, bottomLeft, topLeft}, {})}, &currentTilePolygon);
            gpc_polygon_clip(GPC_DIFF, &currentViewBoundsPolygon, &currentTilePolygon, &currentViewBoundsPolygon);
            gpc_free_polygon(&currentTilePolygon);
        };
        auto candidateIntersectsViewBoundsMask = [&](const RectCoord &candidateRect) {
            if (!optimizeMaskTileGeometrySelection) {
                return true;
            }

            return !coordsutil::checkRectCoordFullyCoveredByNonOverlappingRects2d(candidateRect, rejectedViewRects);
        };

        size_t visibleTileHash = minZoomLevelIndex;
        std::vector<std::pair<VisibleTileCandidate, PrioritizedTiled2dMapTileInfo>> visibleTilesVec;

        auto maxLevelAvailable = zoomLevelInfosWithVirtual.size() - 1;

        const bool isPoseCameraActive = cameraMode == ::MapCamera3dMode::POSE && cameraPosition.has_value();
        const bool isOrbitCameraActive = cameraMode == ::MapCamera3dMode::ORBIT && cameraPosition.has_value();
        auto focusPointInLayerCoords = conversionHelper->convert(layerSystemId, focusPointPosition);
        auto lodOriginInLayerCoords = focusPointInLayerCoords;
        const auto focusPointView = transformToView(
            Coord(layerSystemId, focusPointInLayerCoords.x, focusPointInLayerCoords.y, focusPointAltitude), viewMatrix, origin);
        const bool poseNearCameraRadiusEnabled = layerSystemId != CoordinateSystemIdentifiers::EPSG4326();

        auto earthCenterView = transformToView(Coord(CoordinateSystemIdentifiers::UnitSphere(), 0, 0, 0), viewMatrix, origin);
        const double cullingElevationOffsetMin = get3dCullingElevationOffsetMin();
        const double cullingElevationOffsetMax = get3dCullingElevationOffsetMax();
        const double zMin = focusPointAltitude + cullingElevationOffsetMin;
        const double zMax = focusPointAltitude + cullingElevationOffsetMax;

        auto scaleZoom = [&](double scale) { return std::log2(std::max(scale, numericEpsilon)); };

        // Start from the center tile level, then bias each candidate by camera distance and pitch.
        // Foreground terrain gets more detail before flooring to an index.
        auto desiredContinuousLevelForTile = [&](double requestedCenterLevel, double distanceToTile2D, double distanceToTileZ,
                                                 double distanceToCenter3D, double cameraVerticalFov) {
            constexpr double maxMercatorHorizonAngle = 89.25;
            constexpr double maxZoomLevelsOnScreen = 9.314;
            constexpr double minPerspectiveCosine = 0.5;
            constexpr double minPitchCosine = 0.01;

            const double clampedVerticalFov = std::clamp(cameraVerticalFov, minVerticalFovDegrees, maxVerticalFovDegrees);
            const double fovRadians = clampedVerticalFov * degreesToRadians;
            const double pitchTileLoadingBehavior =
                2.0 * ((maxZoomLevelsOnScreen - 1.0) /
                           scaleZoom(std::cos((maxMercatorHorizonAngle - clampedVerticalFov) * degreesToRadians) /
                                     std::cos(maxMercatorHorizonAngle * degreesToRadians)) -
                       1.0);

            distanceToTile2D = std::max(0.0, distanceToTile2D);
            distanceToTileZ = std::max(distanceEpsilon, distanceToTileZ);
            distanceToCenter3D = std::max(distanceToTileZ, distanceToCenter3D);
            const double distanceToTile3D = std::max(distanceEpsilon, std::hypot(distanceToTile2D, distanceToTileZ));

            const double thisTilePitch = std::atan(distanceToTile2D / distanceToTileZ);

            double desiredLevel = requestedCenterLevel;
            desiredLevel +=
                scaleZoom(distanceToCenter3D / distanceToTile3D / std::max(minPerspectiveCosine, std::cos(fovRadians / 2.0)));
            desiredLevel += pitchTileLoadingBehavior * scaleZoom(std::max(minPitchCosine, std::cos(thisTilePitch))) / 2.0;
            return desiredLevel;
        };

        auto poseLodReferenceDistanceMeters = [&](double cameraAltitudeMeters, double verticalFov,
                                                  double unconstrainedDistanceMeters) {
            constexpr double baseRadiusFovScale = 1.25;
            constexpr double baseRadiusFloorMeters = 3000.0;
            constexpr double flatViewStartAltitudeRatio = 1.5;
            constexpr double flatViewBlendAltitudeRange = 6.0;
            constexpr double flatViewRadiusAltitudeScale = 2.0;
            constexpr double flatViewRadiusFloorMeters = 4500.0;

            const double clampedAltitude = std::max(minimumDistanceMeters, cameraAltitudeMeters);
            const double fovRadians = std::clamp(verticalFov, minVerticalFovDegrees, maxVerticalFovDegrees) * degreesToRadians;
            const double baseRadiusCapMeters = std::max(clampedAltitude, baseRadiusFloorMeters);
            const double baseRadiusMeters =
                std::clamp(clampedAltitude * std::tan(fovRadians / 2.0) * baseRadiusFovScale, clampedAltitude, baseRadiusCapMeters);
            const double flatness = std::clamp((unconstrainedDistanceMeters / clampedAltitude - flatViewStartAltitudeRatio) /
                                                   flatViewBlendAltitudeRange,
                                               0.0, 1.0);
            const double flatViewRadiusMeters =
                std::min(clampedAltitude * flatViewRadiusAltitudeScale, std::max(clampedAltitude, flatViewRadiusFloorMeters));
            const double fullQualityRadiusMeters = baseRadiusMeters + (flatViewRadiusMeters - baseRadiusMeters) * flatness;
            return std::clamp(unconstrainedDistanceMeters, clampedAltitude, fullQualityRadiusMeters);
        };

        auto orbitLodReferenceDistanceMeters = [&](double cameraAltitudeMeters) {
            constexpr double radiusAltitudeScale = 2.0;
            constexpr double radiusFloorMeters = 4500.0;
            const double clampedAltitude = std::max(minimumDistanceMeters, cameraAltitudeMeters);
            return std::min(clampedAltitude * radiusAltitudeScale, std::max(clampedAltitude, radiusFloorMeters));
        };

        auto poseDistanceLodPenalty = [&](double distanceToTileMeters, double fullQualityRadiusMeters) {
            const double radius = std::max(1.0, fullQualityRadiusMeters);
            return scaleZoom(std::max(1.0, distanceToTileMeters / radius));
        };

        auto makeCandidateBounds = [&](const VisibleTileCandidate &candidate) {
            const auto &levelGeometry = zoomLevelGeometryWithVirtual.at(candidate.levelIndex);
            const double tileWidthAdj = levelGeometry.tileWidthAdj;
            const double tileHeightAdj = levelGeometry.tileHeightAdj;
            const double boundsLeft = levelGeometry.boundsLeft;
            const double boundsTop = levelGeometry.boundsTop;

            const Coord topLeft(layerSystemId, candidate.x * tileWidthAdj + boundsLeft, candidate.y * tileHeightAdj + boundsTop,
                                zMax);
            const Coord bottomRight(layerSystemId, topLeft.x + tileWidthAdj, topLeft.y + tileHeightAdj, zMin);

            return RectCoord(topLeft, bottomRight);
        };

        // Convert the camera ground scale to a continuous tile level.
        // Each pixel is assumed to be 0.28mm – https://gis.stackexchange.com/a/315989
        const float screenScaleFactor = zoomInfo.adaptScaleToScreen ? screenDensityPpi / (0.0254 / 0.00028) : 1.0;
        double requestedCenterLevel = static_cast<double>(maxLevelAvailable);
        for (int index = 0; index < static_cast<int>(zoomLevelInfosWithVirtual.size()); ++index) {
            const double levelZoom = zoomInfo.zoomLevelScaleFactor * screenScaleFactor * zoomLevelInfosWithVirtual[index].zoom;
            if (zoom >= levelZoom) {
                requestedCenterLevel = static_cast<double>(std::max(index - 1, 0));
                if (index > 0) {
                    const double previousLevelZoom =
                        zoomInfo.zoomLevelScaleFactor * screenScaleFactor * zoomLevelInfosWithVirtual[index - 1].zoom;
                    const double zoomRatio = previousLevelZoom / std::max(levelZoom, numericEpsilon);
                    const double zoomProgress = std::log2(previousLevelZoom / std::max(static_cast<double>(zoom), numericEpsilon)) /
                                                std::max(std::log2(std::max(zoomRatio, 1.0 + distanceEpsilon)), numericEpsilon);
                    requestedCenterLevel =
                        std::clamp(static_cast<double>(index - 1) + zoomProgress, 0.0, static_cast<double>(maxLevelAvailable));
                }
                break;
            }
        }

        requestedCenterLevel = std::clamp(requestedCenterLevel, 0.0, static_cast<double>(maxLevelAvailable));

        double cameraAltitudeMeters = std::max(minimumDistanceMeters, static_cast<double>(focusPointAltitude));
        if (cameraPosition.has_value() && mapConfig.mapCoordinateSystem.identifier == CoordinateSystemIdentifiers::UnitSphere()) {
            const double unitSphereRadiusMeters =
                1.0 / CoordinateSystemIdentifiers::unitToMeterFactor(CoordinateSystemIdentifiers::UnitSphere());
            const auto cameraWorld = *cameraPosition + origin;
            cameraAltitudeMeters =
                std::max(minimumDistanceMeters, (Vec3DHelper::length(cameraWorld) - 1.0) * unitSphereRadiusMeters);
            if (isPoseCameraActive) {
                // Anchor flat pose views around the camera footprint, not the far screen-center hit.
                const auto cameraSurface = Vec3DHelper::normalize(cameraWorld);
                double phi = std::atan2(cameraSurface.z, -cameraSurface.x);
                if (phi >= 0.0) {
                    phi -= 2.0 * M_PI;
                }
                const double theta = -std::acos(std::clamp(cameraSurface.y, -1.0, 1.0));
                const Coord cameraSurfaceWgs84 = conversionHelper->convert(
                    CoordinateSystemIdentifiers::EPSG4326(), Coord(CoordinateSystemIdentifiers::UnitSphere(), phi, theta, 0.0));
                lodOriginInLayerCoords = conversionHelper->convert(layerSystemId, cameraSurfaceWgs84);
            }
        }
        double layerMetersPerUnit = 1.0 / CoordinateSystemIdentifiers::unitToMeterFactor(layerSystemId);
        const double layerUnitsPerMeter = 1.0 / layerMetersPerUnit;

        // Web mercator shrinks ground distances by cos(latitude): one mercator unit covers less ground
        // towards the poles, and a tile of the same level holds correspondingly more detail per ground meter.
        const bool layerIsWebMercator = layerSystemId == CoordinateSystemIdentifiers::EPSG3857();
        double webMercatorEarthRadiusMeters = 1.0;
        if (layerIsWebMercator) {
            const auto &mercatorBounds = CoordinateSystemFactory::getEpsg3857System().bounds;
            webMercatorEarthRadiusMeters = std::abs(mercatorBounds.bottomRight.x - mercatorBounds.topLeft.x) / (2.0 * M_PI);
        }
        // Ground meters per layer unit relative to the equator at a given layer y; 1.0 for non-mercator layers.
        auto mercatorGroundScale = [&](double layerY) {
            if (!layerIsWebMercator) {
                return 1.0;
            }
            constexpr double minLatitudeCosine = 0.01;
            const double latitudeRadians = 2.0 * std::atan(std::exp(layerY / webMercatorEarthRadiusMeters)) - M_PI / 2.0;
            return std::max(std::cos(latitudeRadians), minLatitudeCosine);
        };
        const double lodOriginGroundScale = mercatorGroundScale(lodOriginInLayerCoords.y);
        double poseCenterDistanceMeters = cameraAltitudeMeters;
        if (isPoseCameraActive) {
            // Invert getPoseDerivedZoom() and cap the center distance used for LOD.
            const double pixelsPerMeter = screenDensityPpi / 0.0254;
            const double aspect =
                height != 0.0f ? std::max(minimumAspectRatio, static_cast<double>(width) / static_cast<double>(height)) : 1.0;
            const double verticalFovRadians =
                std::clamp(static_cast<double>(verticalFov), minVerticalFovDegrees, maxVerticalFovDegrees) * degreesToRadians;
            const double horizontalFovRadians = 2.0 * std::atan(std::tan(verticalFovRadians / 2.0) * aspect);
            const double unconstrainedPoseCenterDistanceMeters = std::max(
                cameraAltitudeMeters, static_cast<double>(zoom) * std::max(minimumViewportPixels, static_cast<double>(width)) /
                                          std::max(pixelsPerMeter, numericEpsilon) /
                                          std::max(2.0 * std::tan(horizontalFovRadians / 2.0), numericEpsilon));
            poseCenterDistanceMeters =
                poseLodReferenceDistanceMeters(cameraAltitudeMeters, verticalFov, unconstrainedPoseCenterDistanceMeters);

            // Lift the base level by the same ratio, otherwise near foreground tiles are too coarse.
            const double centerLevelBoost = std::log2(
                std::max(1.0, unconstrainedPoseCenterDistanceMeters / std::max(minimumDistanceMeters, poseCenterDistanceMeters)));
            requestedCenterLevel = std::clamp(requestedCenterLevel + centerLevelBoost, 0.0, static_cast<double>(maxLevelAvailable));
        }

        auto desiredLevelIndexForTile = [&](const RectCoord &tileBounds, const Vec3D &tileCenterView) {
            const double cameraVerticalFov =
                std::clamp(static_cast<double>(verticalFov), minVerticalFovDegrees, maxVerticalFovDegrees);
            double distanceToCenter3D = std::max(distanceEpsilon, Vec3DHelper::length(focusPointView));
            double distanceToTile2D = std::hypot(tileCenterView.x - focusPointView.x, tileCenterView.y - focusPointView.y);
            double distanceToTileZ = std::max(distanceEpsilon, std::abs(focusPointView.z));
            double groundDistanceLodPenalty = 0.0;
            double mercatorLevelOffset = 0.0;

            if ((isPoseCameraActive || isOrbitCameraActive) && poseNearCameraRadiusEnabled) {
                // View-space depth is weak for terrain LOD; use closest ground distance instead.
                distanceToTile2D =
                    coordsutil::closestDistanceToRectCoord2d(lodOriginInLayerCoords.x, lodOriginInLayerCoords.y, tileBounds) *
                    layerMetersPerUnit;

                // Evaluate the mercator scale at the tile point closest to the LOD origin: the distance
                // becomes true ground meters, and the level drops where tiles hold more detail per meter.
                const double closestYToOrigin =
                    std::clamp(lodOriginInLayerCoords.y, std::min(tileBounds.topLeft.y, tileBounds.bottomRight.y),
                               std::max(tileBounds.topLeft.y, tileBounds.bottomRight.y));
                const double groundScale = mercatorGroundScale(closestYToOrigin);
                distanceToTile2D *= groundScale;
                mercatorLevelOffset = scaleZoom(groundScale);

                distanceToTileZ = std::max(distanceEpsilon, cameraAltitudeMeters);
                const double lodReferenceDistanceMeters =
                    isPoseCameraActive ? poseCenterDistanceMeters : orbitLodReferenceDistanceMeters(cameraAltitudeMeters);
                distanceToCenter3D = std::max(distanceToTileZ, lodReferenceDistanceMeters);
                groundDistanceLodPenalty = poseDistanceLodPenalty(distanceToTile2D, lodReferenceDistanceMeters);

                if (isPoseCameraActive) {
                    // Reduce detail for side/back tiles kept alive only for turning.
                    constexpr double poseAzimuthLodPenalty = 4.0;
                    const double horizontal = std::hypot(tileCenterView.x, tileCenterView.z);
                    const double forwardCos = horizontal > distanceEpsilon ? -tileCenterView.z / horizontal : 1.0;
                    const double behindness = std::clamp((1.0 - forwardCos) * 0.5, 0.0, 1.0);
                    groundDistanceLodPenalty += poseAzimuthLodPenalty * behindness * behindness;
                }
            }

            const double desiredLevel = desiredContinuousLevelForTile(requestedCenterLevel, distanceToTile2D, distanceToTileZ,
                                                                      distanceToCenter3D, cameraVerticalFov) -
                                        groundDistanceLodPenalty + mercatorLevelOffset;
            return std::clamp(static_cast<int>(std::floor(desiredLevel)), 0, static_cast<int>(maxLevelAvailable));
        };

        enum class TileFrustumIntersection { None, Partial, Full };

        // Conservative view-space frustum test for orbit and pose cameras.
        const double tanHalfHorizontalFov =
            projectionMatrix[0] != 0.0f ? 1.0 / projectionMatrix[0] : std::numeric_limits<double>::infinity();
        const double tanHalfVerticalFov =
            projectionMatrix[5] != 0.0f ? 1.0 / projectionMatrix[5] : std::numeric_limits<double>::infinity();
        const double frustumNearPlane = projectionMatrix[14] / (projectionMatrix[10] - 1.0f);
        const double frustumFarPlane = projectionMatrix[14] / (projectionMatrix[10] + 1.0f);
        // Keep/reject uses a wider frustum; fully-inside stays tight for the child fast-path.
        constexpr double frustumCullMargin = 1.15;
        constexpr size_t cullingSampleCount = 13;
        const double tanHalfHorizontalFovMargin = tanHalfHorizontalFov * frustumCullMargin;
        const double tanHalfVerticalFovMargin = tanHalfVerticalFov * frustumCullMargin;

        auto classifyFrustumIntersection = [&](const std::array<Vec3D, cullingSampleCount> &cullingViews,
                                               bool poseCameraInsideTileVolume, bool poseNearCameraTile, bool isSeedLevel) {
            // Keep the tile alive when the pose camera sits inside or very near its displaced volume.
            if (poseCameraInsideTileVolume || poseNearCameraTile) {
                return TileFrustumIntersection::Partial;
            }

            // Test each sphere sample against its own sight line so flat horizon views stay visible.
            auto isFacingAway = [&](const Vec3D &viewPos) {
                const Vec3D normal = viewPos - earthCenterView;
                return normal.x * viewPos.x + normal.y * viewPos.y + normal.z * viewPos.z > 0.0;
            };
            if (mapConfig.mapCoordinateSystem.identifier == CoordinateSystemIdentifiers::UnitSphere() &&
                std::all_of(cullingViews.begin(), cullingViews.end(), isFacingAway)) {
                return TileFrustumIntersection::None;
            }

            bool anyInFront = false;
            bool outsideLeft = true;
            bool outsideRight = true;
            bool outsideBottom = true;
            bool outsideTop = true;
            bool outsideNear = true;
            bool outsideFar = true;
            bool allInside = true;
            int validPointCount = 0;

            for (const auto &viewPos : cullingViews) {
                if (!std::isfinite(viewPos.x) || !std::isfinite(viewPos.y) || !std::isfinite(viewPos.z)) {
                    allInside = false;
                    continue;
                }

                validPointCount++;
                anyInFront = anyInFront || viewPos.z < 0.0;

                // Orbit near/far planes are for depth precision, not terrain rejection.
                const bool insideNear = !isPoseCameraActive || viewPos.z <= -frustumNearPlane;
                const bool insideFar = !isPoseCameraActive || viewPos.z >= -frustumFarPlane;

                // Widened side planes decide keep/reject (conservative).
                outsideLeft = outsideLeft && !(viewPos.x >= viewPos.z * tanHalfHorizontalFovMargin);
                outsideRight = outsideRight && !(viewPos.x <= -viewPos.z * tanHalfHorizontalFovMargin);
                outsideBottom = outsideBottom && !(viewPos.y >= viewPos.z * tanHalfVerticalFovMargin);
                outsideTop = outsideTop && !(viewPos.y <= -viewPos.z * tanHalfVerticalFovMargin);
                outsideNear = outsideNear && !insideNear;
                outsideFar = outsideFar && !insideFar;

                // Tight side planes decide fully-inside (drives the child fast-path).
                allInside = allInside && viewPos.x >= viewPos.z * tanHalfHorizontalFov &&
                            viewPos.x <= -viewPos.z * tanHalfHorizontalFov && viewPos.y >= viewPos.z * tanHalfVerticalFov &&
                            viewPos.y <= -viewPos.z * tanHalfVerticalFov && insideNear && insideFar;
            }

            if (validPointCount == 0 || !anyInFront || outsideLeft || outsideRight || outsideBottom || outsideTop || outsideNear ||
                outsideFar) {
                return isPoseCameraActive && isSeedLevel ? TileFrustumIntersection::Partial : TileFrustumIntersection::None;
            }

            return allInside ? TileFrustumIntersection::Full : TileFrustumIntersection::Partial;
        };

        // Adjacent custom pyramid levels are not always exact integer multiples.
        std::unordered_set<VisibleTileCandidate> enqueuedCandidates;
        auto pushCandidateChildren = [&](const VisibleTileCandidate &candidate, const RectCoord &parentBounds, bool fullyVisible) {
            const auto &childInfo = zoomLevelInfosWithVirtual.at(candidate.levelIndex + 1);
            const auto &childGeometry = zoomLevelGeometryWithVirtual.at(candidate.levelIndex + 1);

            const int childXMin =
                static_cast<int>(std::floor((parentBounds.topLeft.x - childGeometry.boundsLeft) / childGeometry.tileWidthAdj));
            const int childXMax =
                static_cast<int>(std::ceil((parentBounds.bottomRight.x - childGeometry.boundsLeft) / childGeometry.tileWidthAdj)) -
                1;
            const int childYMin =
                static_cast<int>(std::floor((parentBounds.topLeft.y - childGeometry.boundsTop) / childGeometry.tileHeightAdj));
            const int childYMax =
                static_cast<int>(std::ceil((parentBounds.bottomRight.y - childGeometry.boundsTop) / childGeometry.tileHeightAdj)) -
                1;

            for (int childY = childYMax; childY >= childYMin; --childY) {
                for (int childX = childXMax; childX >= childXMin; --childX) {
                    if (childX < 0 || childX >= childInfo.numTilesX || childY < 0 || childY >= childInfo.numTilesY) {
                        continue;
                    }
                    const VisibleTileCandidate child{childX, childY, candidate.levelIndex + 1};
                    if (enqueuedCandidates.insert(child).second) {
                        candidates.push_back({child, fullyVisible});
                    }
                }
            }
        };

        // Depth-first pyramid walk: reject, accept at target level, or refine.
        const auto &detailSelector = get3dTileDetailSelector();
        while (!candidates.empty()) {
            CoveringTileCandidate stackEntry = candidates.back();
            candidates.pop_back();
            VisibleTileCandidate candidate = stackEntry.tile;
            bool fullyVisible = stackEntry.fullyVisible;

            const Tiled2dMapZoomLevelInfo &zoomLevelInfo = zoomLevelInfosWithVirtual.at(candidate.levelIndex);
            const auto tileBounds = makeCandidateBounds(candidate);
            const Coord topRight(layerSystemId, tileBounds.bottomRight.x, tileBounds.topLeft.y, tileBounds.topLeft.z);
            const Coord bottomLeft(layerSystemId, tileBounds.topLeft.x, tileBounds.bottomRight.y, tileBounds.bottomRight.z);
            const double distanceToPoseGroundLayerUnits =
                coordsutil::closestDistanceToRectCoord2d(lodOriginInLayerCoords.x, lodOriginInLayerCoords.y, tileBounds);
            const bool poseCameraInsideTileVolume = isPoseCameraActive && distanceToPoseGroundLayerUnits <= distanceEpsilon &&
                                                    cameraAltitudeMeters >= zMin && cameraAltitudeMeters <= zMax;
            const double poseNearCameraRadiusMeters =
                std::clamp(cameraAltitudeMeters *
                               std::tan(std::clamp(static_cast<double>(verticalFov), minVerticalFovDegrees, maxVerticalFovDegrees) *
                                        degreesToRadians / 2.0) *
                               poseNearCameraRadiusScale,
                           minPoseNearCameraRadiusMeters, maxPoseNearCameraRadiusMeters);
            const bool poseNearCameraTile =
                isPoseCameraActive && poseNearCameraRadiusEnabled &&
                distanceToPoseGroundLayerUnits <= poseNearCameraRadiusMeters * layerUnitsPerMeter / lodOriginGroundScale;

            // Sample the tile footprint at both displacement extremes.
            const Coord topLeftMin = Coord(layerSystemId, tileBounds.topLeft.x, tileBounds.topLeft.y, zMin);
            const Coord topRightMin = Coord(layerSystemId, topRight.x, topRight.y, zMin);
            const Coord bottomLeftMax = Coord(layerSystemId, bottomLeft.x, bottomLeft.y, zMax);
            const Coord bottomRightMax = Coord(layerSystemId, tileBounds.bottomRight.x, tileBounds.bottomRight.y, zMax);

            const Coord tileCenter = Coord(layerSystemId, tileBounds.topLeft.x * 0.5 + tileBounds.bottomRight.x * 0.5,
                                           tileBounds.topLeft.y * 0.5 + tileBounds.bottomRight.y * 0.5,
                                           tileBounds.topLeft.z * 0.5 + tileBounds.bottomRight.z * 0.5);

            const auto tileCenterView = transformToView(tileCenter, viewMatrix, origin);

            const bool isSeedLevel = candidate.levelIndex == minZoomLevelIndex;

            if (!fullyVisible) {
                // Coarse globe tiles can span a hemisphere, so corners alone are not enough.
                const double zMid = (zMin + zMax) * 0.5;
                const Coord topEdge(layerSystemId, (tileBounds.topLeft.x + topRight.x) * 0.5,
                                    (tileBounds.topLeft.y + topRight.y) * 0.5, zMid);
                const Coord bottomEdge(layerSystemId, (bottomLeft.x + tileBounds.bottomRight.x) * 0.5,
                                       (bottomLeft.y + tileBounds.bottomRight.y) * 0.5, zMid);
                const Coord leftEdge(layerSystemId, (tileBounds.topLeft.x + bottomLeft.x) * 0.5,
                                     (tileBounds.topLeft.y + bottomLeft.y) * 0.5, zMid);
                const Coord rightEdge(layerSystemId, (topRight.x + tileBounds.bottomRight.x) * 0.5,
                                      (topRight.y + tileBounds.bottomRight.y) * 0.5, zMid);

                const std::array<Vec3D, cullingSampleCount> cullingViews = {
                    transformToView(tileBounds.topLeft, viewMatrix, origin),
                    transformToView(topRight, viewMatrix, origin),
                    transformToView(bottomLeft, viewMatrix, origin),
                    transformToView(tileBounds.bottomRight, viewMatrix, origin),
                    transformToView(topLeftMin, viewMatrix, origin),
                    transformToView(topRightMin, viewMatrix, origin),
                    transformToView(bottomLeftMax, viewMatrix, origin),
                    transformToView(bottomRightMax, viewMatrix, origin),
                    tileCenterView,
                    transformToView(topEdge, viewMatrix, origin),
                    transformToView(bottomEdge, viewMatrix, origin),
                    transformToView(leftEdge, viewMatrix, origin),
                    transformToView(rightEdge, viewMatrix, origin)};

                const auto intersection =
                    classifyFrustumIntersection(cullingViews, poseCameraInsideTileVolume, poseNearCameraTile, isSeedLevel);
                if (intersection == TileFrustumIntersection::None) {
                    // Mask layers need the inverse of the selected area.
                    clipCandidateFromViewBounds(tileBounds.topLeft, topRight, tileBounds.bottomRight, bottomLeft);
                    continue;
                }
                // Fully visible parents do not need another frustum test for their children.
                fullyVisible = intersection == TileFrustumIntersection::Full;
            }

            if (!validViewBounds) {
                validViewBounds = true;
            }

            bool lastLevel = candidate.levelIndex == maxLevelAvailable;
            const int terrainDesiredLevelIndex = detailSelector.needsTerrainDesiredLevelIndex()
                                                     ? desiredLevelIndexForTile(tileBounds, tileCenterView)
                                                     : candidate.levelIndex;
            const Tiled2dMap3dTileDetailSelectionContext detailSelectionContext{candidate.levelIndex,
                                                                                terrainDesiredLevelIndex,
                                                                                layerSystemId,
                                                                                focusPointAltitude,
                                                                                width,
                                                                                height,
                                                                                zoomInfo.zoomLevelScaleFactor,
                                                                                focusPointInLayerCoords,
                                                                                tileCenter,
                                                                                tileBounds,
                                                                                viewMatrix,
                                                                                projectionMatrix,
                                                                                origin,
                                                                                *conversionHelper};
            bool preciseEnough = detailSelector.isPreciseEnough(detailSelectionContext);

            bool isVirtual = topMostZoomLevel > zoomLevelInfo.zoomLevelIdentifier;

            if (!isVirtual && (preciseEnough || lastLevel)) {
                int t = 0;
                double priority = std::sqrt(std::pow(tileCenterView.x - focusPointView.x, 2.0) +
                                            std::pow(tileCenterView.y - focusPointView.y, 2.0) +
                                            std::pow(tileCenterView.z - focusPointView.z, 2.0)) *
                                  priorityScale;
                if (isPoseCameraActive) {
                    // Use the same closest-point distance as the LOD calculation so close, high-detail
                    // pose tiles are requested before lower-detail tiles farther away.
                    priority = distanceToPoseGroundLayerUnits * layerMetersPerUnit;
                }
                visibleTilesVec.push_back(std::make_pair(
                    candidate,
                    PrioritizedTiled2dMapTileInfo(Tiled2dMapTileInfo(tileBounds, candidate.x, candidate.y, t,
                                                                     zoomLevelInfo.zoomLevelIdentifier, zoomLevelInfo.zoom),
                                                  priority)));

                maxLevel = std::max(maxLevel, zoomLevelInfo.zoomLevelIdentifier);
                continue;
            }

            if ((!preciseEnough || isVirtual) && !lastLevel) {
                // Virtual tiles cannot draw, so they keep descending until a real level.
                pushCandidateChildren(candidate, tileBounds, fullyVisible);
            }
        }

        if (shouldComputeCurrentViewBounds) {
            currentViewBounds = gpc_get_polygon_coord(&currentViewBoundsPolygon, layerSystemId);
        }

        if (!validViewBounds) {
            if (shouldComputeCurrentViewBounds) {
                gpc_free_polygon(&currentViewBoundsPolygon);
            }
            return;
        }

        std::sort(visibleTilesVec.begin(), visibleTilesVec.end(),
                  [](const auto &lhs, const auto &rhs) { return lhs.second.priority < rhs.second.priority; });

        std::vector<VisibleTilesLayer> layers;
        const auto dataBounds = layerConfig->getBounds();
        const std::optional<RectCoord> availableDataBounds =
            dataBounds.has_value() ? std::make_optional(conversionHelper->convertRect(layerSystemId, *dataBounds)) : std::nullopt;

        // Later layers are parent fallbacks while child tiles load.
        for (int previousLayerOffset = 0; (previousLayerOffset <= zoomInfo.numDrawPreviousLayers || zoomInfo.maskTile);
             previousLayerOffset++) {

            VisibleTilesLayer curVisibleTiles(-previousLayerOffset, 0);

            std::vector<std::pair<VisibleTileCandidate, PrioritizedTiled2dMapTileInfo>> nextVisibleTilesVec;

            bool allTopMost = true;

            for (auto &tile : visibleTilesVec) {
                const auto &tileBounds = tile.second.tileInfo.bounds;
                if (!candidateIntersectsViewBoundsMask(tileBounds)) {
                    continue;
                }

                if (availableDataBounds.has_value()) {
                    const auto &availableTiles = *availableDataBounds;
                    const Tiled2dMapZoomLevelInfo &zoomLevelInfo = zoomLevelInfosWithVirtual.at(tile.first.levelIndex);

                    RectCoord layerBounds = zoomLevelInfo.bounds;
                    const bool leftToRight = layerBounds.topLeft.x < layerBounds.bottomRight.x;
                    const bool topToBottom = layerBounds.topLeft.y < layerBounds.bottomRight.y;

                    const double boundsRatio =
                        std::abs(((zoomLevelInfo.bounds.bottomRight.y - zoomLevelInfo.bounds.topLeft.y) / zoomLevelInfo.numTilesY) /
                                 ((zoomLevelInfo.bounds.bottomRight.x - zoomLevelInfo.bounds.topLeft.x) / zoomLevelInfo.numTilesX));
                    const double tileWidth = zoomLevelInfo.tileWidthLayerSystemUnits;
                    const double tileHeight = zoomLevelInfo.tileWidthLayerSystemUnits * boundsRatio;
                    const double tLength = tileWidth / tilePixelSize;
                    const double tHeight = tileHeight / tilePixelSize;

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
                    int min_left = std::max(0, min_left_pixel / tilePixelSize);

                    int max_left_pixel = floor((maxAvailableX - originX) / tLength);
                    int max_left = std::min(zoomLevelInfo.numTilesX, max_left_pixel / tilePixelSize);

                    int min_top_pixel = floor((minAvailableY - originY) / tHeight);
                    int min_top = std::max(0, min_top_pixel / tilePixelSize);

                    int max_top_pixel = floor((maxAvailableY - originY) / tHeight);
                    int max_top = std::min(zoomLevelInfo.numTilesY, max_top_pixel / tilePixelSize);

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

                tile.second.tileInfo.tessellationFactor =
                    std::min(std::max(0, maxLevel - tile.second.tileInfo.zoomIdentifier), maxTessellationFactor);
                curVisibleTiles.visibleTiles.insert(tile.second);

                if (allTopMost && tile.second.tileInfo.zoomIdentifier != topMostZoomLevel) {
                    allTopMost = false;
                }

                std::hash_combine(visibleTileHash, std::hash<Tiled2dMapTileInfo>{}(tile.second.tileInfo));

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
                    double priority = previousLayerOffset * priorityScale + tile.second.priority;
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

        std::hash_combine(visibleTileHash, std::hash<int>{}(maxLevel));
        std::hash_combine(visibleTileHash, std::hash<::MapCamera3dMode>{}(cameraMode));
        std::hash_combine(visibleTileHash, std::hash<bool>{}(maskTileGeometryTileSelectionOptimizationEnabled));
        std::hash_combine(visibleTileHash, quantizeHashValue(width, pixelHashScale));
        std::hash_combine(visibleTileHash, quantizeHashValue(height, pixelHashScale));
        std::hash_combine(visibleTileHash, quantizeHashValue(verticalFov, cameraValueHashScale));
        std::hash_combine(visibleTileHash, quantizeHashValue(horizontalFov, cameraValueHashScale));

        if (isPoseCameraActive) {
            const auto &cameraCartesian = *cameraPosition;
            std::hash_combine(visibleTileHash, quantizeHashValue(cameraCartesian.x, coordinateHashScale));
            std::hash_combine(visibleTileHash, quantizeHashValue(cameraCartesian.y, coordinateHashScale));
            std::hash_combine(visibleTileHash, quantizeHashValue(cameraCartesian.z, coordinateHashScale));
            for (const auto &matrixValue : viewMatrix) {
                std::hash_combine(visibleTileHash, quantizeHashValue(matrixValue, matrixHashScale));
            }
        }

        if (lastVisibleTilesHash != visibleTileHash) {
            lastVisibleTilesHash = visibleTileHash;
            onVisibleTilesChanged(layers, true);
        }

        if (shouldComputeCurrentViewBounds) {
            gpc_free_polygon(&currentViewBoundsPolygon);
        }
    }
};

template <class Source> std::unique_ptr<Tiled2dMap3dTileSelection<Source>> makeDefaultTiled2dMap3dTileSelection() {
    return std::make_unique<DefaultTiled2dMap3dTileSelection<Source>>();
}

template <class Source> std::unique_ptr<Tiled2dMap3dTileSelection<Source>> makeDisplacedTerrainTiled2dMap3dTileSelection() {
    return std::make_unique<DisplacedTerrainTiled2dMap3dTileSelection<Source>>();
}
