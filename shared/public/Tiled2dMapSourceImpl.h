/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "DateHelper.h"
#include "Tiled2dMapSource.h"
#include "TiledLayerError.h"
#include <algorithm>

#include "PolygonCoord.h"
#include <iostream>
#include "gpc.h"
#include "Logger.h"
#include "Matrix.h"
#include "Vec2DHelper.h"
#include "Vec3DHelper.h"

struct VisibleTileCandidate {
    int x; int y;
    int levelIndex;
    bool operator==(const VisibleTileCandidate& other) const {
        return x == other.x && y == other.y && levelIndex == other.levelIndex;
    }
};

namespace std {
    template <>
    struct hash<VisibleTileCandidate> {
        size_t operator()(const VisibleTileCandidate& candidate) const {
            size_t h1 = hash<int>()(candidate.x);
            size_t h2 = hash<int>()(candidate.y);
            size_t h3 = hash<int>()(candidate.levelIndex);

            // Combine hashes using a simple combining function, based on the one from Boost library
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}

template<class T, class L, class R>
Tiled2dMapSource<T, L, R>::Tiled2dMapSource(const MapConfig &mapConfig, const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                         const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                         const std::shared_ptr<SchedulerInterface> &scheduler,
                                         float screenDensityPpi,
                                         size_t loaderCount,
                                         std::string layerName)
    : mapConfig(mapConfig)
    , layerConfig(layerConfig)
    , conversionHelper(conversionHelper)
    , scheduler(scheduler)
    , zoomLevelInfos(layerConfig->getZoomLevelInfos())
    , zoomInfo(layerConfig->getZoomInfo())
    , layerSystemId(layerConfig->getCoordinateSystemIdentifier())
    , isPaused(false)
    , screenDensityPpi(screenDensityPpi)
    , layerName(layerName) {
    std::sort(zoomLevelInfos.begin(), zoomLevelInfos.end(),
              [](const Tiled2dMapZoomLevelInfo &a, const Tiled2dMapZoomLevelInfo &b) -> bool { return a.zoom > b.zoom; });
}

const static double VIEWBOUNDS_PADDING_MIN_DIM_PC = 0.15;
const static int8_t ALWAYS_KEEP_LEVEL_TARGET_ZOOM_OFFSET = -8;

template<class T, class L, class R>
bool Tiled2dMapSource<T, L, R>::isTileVisible(const Tiled2dMapTileInfo &tileInfo) {
    return currentVisibleTiles.count(tileInfo) > 0;
}

template <typename T>
static void hash_combine(size_t& seed, const T& value) {
    std::hash<T> hasher;
    auto v = hasher(value);
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<class T, class L, class R>
::Vec3D Tiled2dMapSource<T, L, R>::transformToView(const ::Coord &position, const std::vector<float> &viewMatrix) {
    Coord mapCoord = conversionHelper->convertToRenderSystem(position);
    std::vector<float> inVec = {(float) (mapCoord.z * sin(mapCoord.y) * cos(mapCoord.x)),
                                (float) (mapCoord.z * cos(mapCoord.y)),
                                (float) (-mapCoord.z * sin(mapCoord.y) * sin(mapCoord.x)),
                                1.0};
    std::vector<float> outVec = {0, 0, 0, 0};

    Matrix::multiply(viewMatrix, inVec, outVec);

    auto point2d = Vec3D(outVec[0] / outVec[3], outVec[1] / outVec[3], outVec[2] / outVec[3]);
    return point2d;
}

template<class T, class L, class R>
::Vec3D Tiled2dMapSource<T, L, R>::projectToScreen(const ::Vec3D & position, const std::vector<float> & projectionMatrix) {
    std::vector<float> inVec = {(float)position.x, (float)position.y, (float)position.z, 1.0};
    std::vector<float> outVec = {0, 0, 0, 0};

    Matrix::multiply(projectionMatrix, inVec, outVec);

    auto point2d = Vec3D(outVec[0] / outVec[3], outVec[1] / outVec[3], outVec[2] / outVec[3]);
    return point2d;
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::onCameraChange(const std::vector<float> &viewMatrix, const std::vector<float> &projectionMatrix,
                                               float verticalFov, float horizontalFov, float width, float height,
                                               float focusPointAltitude, const ::Coord & focusPointPosition, float zoom) {

    if (isPaused) {
        return;
    }

    if (width <= 0 || height <= 0) {
        return;
    }

    std::queue<VisibleTileCandidate> candidates;
    std::unordered_set<VisibleTileCandidate> candidatesSet;


    Coord viewBoundsTopLeft(layerSystemId, 0, 0, 0);
    Coord viewBoundsTopRight(layerSystemId, 0, 0, 0);
    Coord viewBoundsBottomLeft(layerSystemId, 0, 0, 0);
    Coord viewBoundsBottomRight(layerSystemId, 0, 0, 0);
    bool validViewBounds = false;

    int minNumTiles = layerSystemId == CoordinateSystemIdentifiers::EPSG4326() ? 0 : 1;

    int maxLevel = 0;
    int minZoomLevelIndex = 0;
    for (int index = 0; index < zoomLevelInfos.size(); ++index) {
        const auto &level = zoomLevelInfos[index];
        if (level.numTilesX > minNumTiles && level.numTilesY > minNumTiles) {
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

    std::vector<std::pair<VisibleTileCandidate, PrioritizedTiled2dMapTileInfo>> visibleTilesVec;

    auto maxLevelAvailable = zoomLevelInfos.size() - 1;

    int candidateChecks = 0;

    auto focusPointInLayerCoords = conversionHelper->convert(layerSystemId, focusPointPosition);

    auto earthCenterView = transformToView(Coord(CoordinateSystemIdentifiers::UnitSphere(), 0.0, 0.0, 0.0), viewMatrix);

    while (candidates.size() > 0) {
        VisibleTileCandidate candidate = candidates.front();
        candidates.pop();
        candidatesSet.erase(candidate);

        candidateChecks++;


        const Tiled2dMapZoomLevelInfo &zoomLevelInfo = zoomLevelInfos.at(candidate.levelIndex);

        const double boundsRatio = std::abs((zoomLevelInfo.bounds.bottomRight.y  - zoomLevelInfo.bounds.topLeft.y) / (zoomLevelInfo.bounds.bottomRight.x  - zoomLevelInfo.bounds.topLeft.x));
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

        const double heightRange = 1000;

        const Coord topLeft = Coord(layerSystemId, candidate.x * tileWidthAdj + boundsLeft, candidate.y * tileHeightAdj + boundsTop, focusPointAltitude);
        const Coord topRight = Coord(layerSystemId, topLeft.x + tileWidthAdj, topLeft.y, focusPointAltitude - heightRange / 2.0);
        const Coord bottomLeft = Coord(layerSystemId, topLeft.x, topLeft.y + tileHeightAdj, focusPointAltitude - heightRange / 2.0);
        const Coord bottomRight = Coord(layerSystemId, topLeft.x + tileWidthAdj, topLeft.y + tileHeightAdj, focusPointAltitude- heightRange / 2.0);

        const Coord tileCenter = Coord(layerSystemId, topLeft.x * 0.5 + bottomRight.x * 0.5, topLeft.y * 0.5 + bottomRight.y * 0.5, topLeft.z * 0.5 + bottomRight.z * 0.5);

        const auto focusPointClampedToTile = Coord(layerSystemId,
                                                   topLeft.x < topRight.x ? std::clamp(focusPointInLayerCoords.x, topLeft.x, topRight.x) : std::clamp(focusPointInLayerCoords.x, topRight.x, topLeft.x),
                                                   topLeft.y < bottomLeft.y ? std::clamp(focusPointInLayerCoords.y, topLeft.y, bottomLeft.y) : std::clamp(focusPointInLayerCoords.y, bottomLeft.y, topLeft.y),
                                                   focusPointAltitude);

        auto toRight = focusPointClampedToTile.x < tileCenter.x;
        auto toTop = focusPointClampedToTile.y < tileCenter.y;

        const double sampleSize = 0.25;

        const auto focusPointSampleX = Coord(layerSystemId, 
                                             focusPointClampedToTile.x + (toRight ? tileWidthAdj : -tileWidthAdj) * sampleSize,
                                             focusPointClampedToTile.y,
                                             focusPointClampedToTile.z);

        const auto focusPointSampleY = Coord(layerSystemId,
                                             focusPointClampedToTile.x,
                                             focusPointClampedToTile.y + (toTop ? -tileHeightAdj : tileHeightAdj) * sampleSize,
                                             focusPointClampedToTile.z);

        const Coord topCenter = Coord(layerSystemId, topLeft.x * 0.5 + topRight.x * 0.5, topLeft.y, topLeft.z);
        const Coord bottomCenter = Coord(layerSystemId, bottomLeft.x * 0.5 + bottomRight.x * 0.5, bottomLeft.y, bottomLeft.z);
        const Coord leftCenter = Coord(layerSystemId, topLeft.x, bottomLeft.y * 0.5 + topLeft.y * 0.5, topLeft.z);
        const Coord rightCenter = Coord(layerSystemId, topRight.x, bottomRight.y * 0.5 + topRight.y * 0.5, topRight.z);

        const Coord topLeftHigh = Coord(layerSystemId, topLeft.x, topLeft.y, focusPointAltitude + heightRange / 2.0);
        const Coord topRightHigh = Coord(layerSystemId, topRight.x , topRight.y, focusPointAltitude + heightRange / 2.0);
        const Coord bottomLeftHigh = Coord(layerSystemId, bottomLeft.x, bottomLeft.y, focusPointAltitude + heightRange / 2.0);
        const Coord bottomRightHigh = Coord(layerSystemId, bottomRight.x, bottomRight.y, focusPointAltitude + heightRange / 2.0);

        auto topLeftView = transformToView(topLeft, viewMatrix);
        auto topRightView = transformToView(topRight, viewMatrix);
        auto bottomLeftView = transformToView(bottomLeft, viewMatrix);
        auto bottomRightView = transformToView(bottomRight, viewMatrix);

        /*
         use focuspoint in layersystem and clamp to tileBounds
         */


        auto focusPointClampedView = transformToView(focusPointClampedToTile, viewMatrix);
        auto focusPointSampleXView = transformToView(focusPointSampleX, viewMatrix);
        auto focusPointSampleYView = transformToView(focusPointSampleY, viewMatrix);

        auto topLeftHighView = transformToView(topLeftHigh, viewMatrix);
        auto topRightHighView = transformToView(topRightHigh, viewMatrix);
        auto bottomLeftHighView = transformToView(bottomLeftHigh, viewMatrix);
        auto bottomRightHighView = transformToView(bottomRightHigh, viewMatrix);

        float centerZ = (topLeftView.z + topRightView.z + bottomLeftView.z + bottomRightView.z) / 4.0;

        auto diffCenterViewTopLeft = topLeftView - earthCenterView;
        auto diffCenterViewTopRight = topRightView - earthCenterView;
        auto diffCenterViewBottomLeft = bottomLeftView - earthCenterView;
        auto diffCenterViewBottomRight = bottomRightView - earthCenterView;

        bool isKeptLevel = candidate.levelIndex == minZoomLevelIndex;

        if (!isKeptLevel && diffCenterViewTopLeft.z < 0.0 && diffCenterViewTopRight.z < 0.0 && diffCenterViewBottomLeft.z < 0.0 &&
            diffCenterViewBottomRight.z < 0.0) {
            continue;
        }
        auto samplePointOriginViewScreen = projectToScreen(focusPointClampedView, projectionMatrix);
        if (!isKeptLevel && 
            (samplePointOriginViewScreen.x < -1.0 || samplePointOriginViewScreen.x > 1.0 ||
            samplePointOriginViewScreen.y < -1.0 || samplePointOriginViewScreen.y > 1.0)) {
            if (mapConfig.mapCoordinateSystem.identifier == CoordinateSystemIdentifiers::UnitSphere()) {
                // v(0,0,+1) = unit-vector out of screen
                float topLeftHA = 180.0 / M_PI * atan2(topLeftView.x, -topLeftView.z);
                float topLeftVA = 180.0 / M_PI * atan2(topLeftView.y, -topLeftView.z);
                float topRightHA = 180.0 / M_PI * atan2(topRightView.x, -topRightView.z);
                float topRightVA = 180.0 / M_PI * atan2(topRightView.y, -topRightView.z);
                float bottomLeftHA = 180.0 / M_PI * atan2(bottomLeftView.x, -bottomLeftView.z);
                float bottomLeftVA = 180.0 / M_PI * atan2(bottomLeftView.y, -bottomLeftView.z);
                float bottomRightHA = 180.0 / M_PI * atan2(bottomRightView.x, -bottomRightView.z);
                float bottomRightVA = 180.0 / M_PI * atan2(bottomRightView.y, -bottomRightView.z);

                float topLeftHighHA = 180.0 / M_PI * atan2(topLeftHighView.x, -topLeftHighView.z);
                float topLeftHighVA = 180.0 / M_PI * atan2(topLeftHighView.y, -topLeftHighView.z);
                float topRightHighHA = 180.0 / M_PI * atan2(topRightHighView.x, -topRightHighView.z);
                float topRightHighVA = 180.0 / M_PI * atan2(topRightHighView.y, -topRightHighView.z);
                float bottomLeftHighHA = 180.0 / M_PI * atan2(bottomLeftHighView.x, -bottomLeftHighView.z);
                float bottomLeftHighVA = 180.0 / M_PI * atan2(bottomLeftHighView.y, -bottomLeftHighView.z);
                float bottomRightHighHA = 180.0 / M_PI * atan2(bottomRightHighView.x, -bottomRightHighView.z);
                float bottomRightHighVA = 180.0 / M_PI * atan2(bottomRightHighView.y, -bottomRightHighView.z);

                // 0.5: half of view on each side of center
                // 1.02: increase angle with padding
                float fovFactor = 0.5 * 1.0;

                if (topLeftVA < -verticalFov * fovFactor && topRightVA < -verticalFov * fovFactor &&
                    bottomLeftVA < -verticalFov * fovFactor && bottomRightVA < -verticalFov * fovFactor &&
                    topLeftHighVA < -verticalFov * fovFactor && topRightHighVA < -verticalFov * fovFactor &&
                    bottomLeftHighVA < -verticalFov * fovFactor && bottomRightHighVA < -verticalFov * fovFactor
                        ) {
                    continue;
                }
                if (topLeftHA < -horizontalFov * fovFactor && topRightHA < -horizontalFov * fovFactor &&
                    bottomLeftHA < -horizontalFov * fovFactor && bottomRightHA < -horizontalFov * fovFactor &&
                    topLeftHighHA < -horizontalFov * fovFactor && topRightHighHA < -horizontalFov * fovFactor &&
                    bottomLeftHighHA < -horizontalFov * fovFactor && bottomRightHighHA < -horizontalFov * fovFactor
                        ) {
                    continue;
                }
                if (topLeftVA > verticalFov * fovFactor && topRightVA > verticalFov * fovFactor &&
                    bottomLeftVA > verticalFov * fovFactor && bottomRightVA > verticalFov * fovFactor &&
                    topLeftHighVA > verticalFov * fovFactor && topRightHighVA > verticalFov * fovFactor &&
                    bottomLeftHighVA > verticalFov * fovFactor && bottomRightHighVA > verticalFov * fovFactor
                        ) {
                    continue;
                }
                if (topLeftHA > horizontalFov * fovFactor && topRightHA > horizontalFov * fovFactor &&
                    bottomLeftHA > horizontalFov * fovFactor && bottomRightHA > horizontalFov * fovFactor &&
                    topLeftHighHA > horizontalFov * fovFactor && topRightHighHA > horizontalFov * fovFactor &&
                    bottomLeftHighHA > horizontalFov * fovFactor && bottomRightHighHA > horizontalFov * fovFactor
                        ) {
                    continue;
                }
            } else {
                if (topLeftView.x < -width / 2.0 && topRightView.x < -width / 2.0 && bottomLeftView.x < -width / 2.0 &&
                    bottomRightView.x < -width / 2.0) {
                    continue;
                }
                if (topLeftView.y < -height / 2.0 && topRightView.y < -height / 2.0 && bottomLeftView.y < -height / 2.0 &&
                    bottomRightView.y < -height / 2.0) {
                    continue;
                }
                if (topLeftView.x > width / 2.0 && topRightView.x > width / 2.0 && bottomLeftView.x > width / 2.0 &&
                    bottomRightView.x > width / 2.0) {
                    continue;
                }
                if (topLeftView.y > height / 2.0 && topRightView.y > height / 2.0 && bottomLeftView.y > height / 2.0 &&
                    bottomRightView.y > height / 2.0) {
                    continue;
                }
            }
        }


        if (!validViewBounds) {
            viewBoundsTopLeft = topLeft;
            viewBoundsBottomRight = bottomRight;
            validViewBounds = true;
        }

        auto updateBounds = [](double& bound, double value, bool compareLess) {
            bound = compareLess ? std::min(bound, value) : std::max(bound, value);
        };

        // Update x coordinates
        updateBounds(viewBoundsTopRight.x, topRight.x, leftToRight);
        updateBounds(viewBoundsTopLeft.x, topLeft.x, !leftToRight);
        updateBounds(viewBoundsBottomRight.x, bottomRight.x, leftToRight);
        updateBounds(viewBoundsBottomLeft.x, bottomLeft.x, !leftToRight);

        // Update y coordinates
        updateBounds(viewBoundsTopRight.y, topRight.y, topToBottom);
        updateBounds(viewBoundsTopLeft.y, topLeft.y, topToBottom);
        updateBounds(viewBoundsBottomRight.y, bottomRight.y, !topToBottom);
        updateBounds(viewBoundsBottomLeft.y, bottomLeft.y, !topToBottom);

        auto samplePointYViewScreen = projectToScreen(focusPointSampleYView, projectionMatrix);
        auto samplePointXViewScreen = projectToScreen(focusPointSampleXView, projectionMatrix);

        Vec2D samplePointOriginViewScreenPx(samplePointOriginViewScreen.x * (width / 2.0), samplePointOriginViewScreen.y * (height / 2.0));
        Vec2D samplePointYViewScreenPx(samplePointYViewScreen.x * (width / 2.0), samplePointYViewScreen.y * (height / 2.0));
        Vec2D samplePointXViewScreenPx(samplePointXViewScreen.x * (width / 2.0), samplePointXViewScreen.y * (height / 2.0));

        double xLengthPx = Vec2DHelper::distance(samplePointOriginViewScreenPx, samplePointXViewScreenPx);
        double yLengthPx = Vec2DHelper::distance(samplePointOriginViewScreenPx, samplePointYViewScreenPx);

        double maxLength = sampleSize * (std::min(width, height) * 0.5 / zoomInfo.zoomLevelScaleFactor);
        bool preciseEnough = xLengthPx <= maxLength && yLengthPx <= maxLength;

        bool lastLevel = candidate.levelIndex == maxLevelAvailable;

        if (preciseEnough || lastLevel || isKeptLevel) {
            const RectCoord rect(topLeft, bottomRight);
            int t = 0;
            double priority = -centerZ * 100000;
            visibleTilesVec.push_back(std::make_pair(candidate, PrioritizedTiled2dMapTileInfo(
                    Tiled2dMapTileInfo(rect, candidate.x, candidate.y, t, zoomLevelInfo.zoomLevelIdentifier, zoomLevelInfo.zoom),
                    priority)));

            maxLevel = std::max(maxLevel, zoomLevelInfo.zoomLevelIdentifier);
        }

        if (!preciseEnough && !lastLevel) {
            const Tiled2dMapZoomLevelInfo &zoomLevelInfo = zoomLevelInfos.at(candidate.levelIndex + 1);

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

    if (!validViewBounds) {
        return;
    }

    currentViewBounds.topLeft = Coord(4326, -180, 90, 0);
    currentViewBounds.topRight =  Coord(4326, 180, 90, 0);
    currentViewBounds.bottomRight =  Coord(4326, 180, -90, 0);
    currentViewBounds.bottomLeft =  Coord(4326, -180, -90, 0);
    currentViewBounds = conversionHelper->convertQuad(layerSystemId, currentViewBounds);

    std::vector<VisibleTilesLayer> layers;

    for (int previousLayerOffset = 0; previousLayerOffset <= zoomInfo.numDrawPreviousLayers; previousLayerOffset++) {

        VisibleTilesLayer curVisibleTiles(-previousLayerOffset);

        std::vector<std::pair<VisibleTileCandidate, PrioritizedTiled2dMapTileInfo>> nextVisibleTilesVec;

        for (auto &tile : visibleTilesVec) {
            tile.second.tileInfo.tessellationFactor = std::min(std::max(0, maxLevel - tile.second.tileInfo.zoomIdentifier), 4);
            curVisibleTiles.visibleTiles.insert(tile.second);

            if (tile.first.levelIndex > 0 && previousLayerOffset < zoomInfo.numDrawPreviousLayers) {

                const Tiled2dMapZoomLevelInfo &zoomLevelInfo = zoomLevelInfos.at(tile.first.levelIndex - 1);
                const double boundsRatio = std::abs((zoomLevelInfo.bounds.bottomRight.y  - zoomLevelInfo.bounds.topLeft.y) / (zoomLevelInfo.bounds.bottomRight.x  - zoomLevelInfo.bounds.topLeft.x));
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

                const Coord topLeft = Coord(layerSystemId, parent.x * tileWidthAdj + boundsLeft, parent.y * tileHeightAdj + boundsTop, focusPointAltitude);
                const Coord bottomRight = Coord(layerSystemId, topLeft.x + tileWidthAdj, topLeft.y + tileHeightAdj, focusPointAltitude);

                const RectCoord rect(topLeft, bottomRight);
                int t = 0;
                double priority = previousLayerOffset * 100000 + tile.second.priority;
                nextVisibleTilesVec.push_back(std::make_pair(parent, PrioritizedTiled2dMapTileInfo(
                        Tiled2dMapTileInfo(rect, parent.x, parent.y, t, zoomLevelInfo.zoomLevelIdentifier, zoomLevelInfo.zoom),
                        priority)));
            }
            visibleTilesVec = nextVisibleTilesVec;
        }

        layers.push_back(curVisibleTiles);
    }

    currentZoomLevelIdentifier = maxLevel;

    onVisibleTilesChanged(layers, true);

}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::onVisibleBoundsChanged(const ::RectCoord &visibleBounds, int curT, double zoom) {
    if (isPaused) {
        return;
    }

    std::vector<PrioritizedTiled2dMapTileInfo> visibleTilesVec;

    RectCoord visibleBoundsLayer = conversionHelper->convertRect(layerSystemId, visibleBounds);

    const auto bounds = layerConfig->getBounds();

    double centerVisibleX = visibleBoundsLayer.topLeft.x + 0.5 * (visibleBoundsLayer.bottomRight.x - visibleBoundsLayer.topLeft.x);
    double centerVisibleY = visibleBoundsLayer.topLeft.y + 0.5 * (visibleBoundsLayer.bottomRight.y - visibleBoundsLayer.topLeft.y);

    size_t numZoomLevels = zoomLevelInfos.size();
    int targetZoomLayer = -1;

    // Each pixel is assumed to be 0.28mm – https://gis.stackexchange.com/a/315989
    const float screenScaleFactor = zoomInfo.adaptScaleToScreen ? screenDensityPpi / (0.0254 / 0.00028) : 1.0;

    if (!zoomInfo.underzoom
        && (zoomLevelInfos.empty() || zoomLevelInfos[0].zoom * zoomInfo.zoomLevelScaleFactor * screenScaleFactor < zoom)
        && (zoomLevelInfos.empty() || zoomLevelInfos[0].zoomLevelIdentifier != 0)) { // enable underzoom if the first zoomLevel is zoomLevelIdentifier == 0
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
        targetZoomLayer = (int) numZoomLevels - 1;
    }

    int targetZoomLevelIdentifier = zoomLevelInfos.at(targetZoomLayer).zoomLevelIdentifier;
    int startZoomLayer = 0;
    int endZoomLevel = std::min((int) numZoomLevels - 1, targetZoomLayer + 2);
    int keepZoomLevelOffset = std::max(zoomLevelInfos.at(startZoomLayer).zoomLevelIdentifier,
                                       zoomLevelInfos.at(endZoomLevel).zoomLevelIdentifier + ALWAYS_KEEP_LEVEL_TARGET_ZOOM_OFFSET) -
                              targetZoomLevelIdentifier;

    int distanceWeight = 100;
    int zoomLevelWeight = 1000 * zoomLevelInfos.at(0).numTilesT;
    int zDistanceWeight = 1000 * zoomLevelInfos.at(0).numTilesT;

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

        VisibleTilesLayer curVisibleTiles(i - targetZoomLayer);
        std::vector<PrioritizedTiled2dMapTileInfo> curVisibleTilesVec;

        const double boundsRatio = std::abs((zoomLevelInfo.bounds.bottomRight.y  - zoomLevelInfo.bounds.topLeft.y) / (zoomLevelInfo.bounds.bottomRight.x  - zoomLevelInfo.bounds.topLeft.x));

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
        int maxTileLeft = std::floor(
                std::max(leftToRight ? (visibleRight - boundsLeft) : (boundsLeft - visibleRight), 0.0) / tileWidth);

        const double boundsTop = layerBounds.topLeft.y;
        int startTileTop = std::floor(std::max(topToBottom ? (visibleTop - boundsTop) : (boundsTop - visibleTop), 0.0) / tileHeight);
        int maxTileTop = std::floor(
                std::max(topToBottom ? (visibleBottom - boundsTop) : (boundsTop - visibleBottom), 0.0) / tileHeight);

        if (bounds.has_value()) {
            const auto availableTiles = conversionHelper->convertRect(layerSystemId, *bounds);

            const double tLength = tileWidth / 256;
            const double tHeight = tileHeight / 256;
            const double tWidthAdj = leftToRight ? tLength : -tLength;
            const double tHeightAdj = topToBottom ? tHeight : -tHeight;
            const double originX = leftToRight ? zoomLevelInfo.bounds.topLeft.x : -zoomLevelInfo.bounds.bottomRight.x;
            const double originY = topToBottom ? zoomLevelInfo.bounds.bottomRight.y : -zoomLevelInfo.bounds.topLeft.y;
            const double minAvailableX = leftToRight ? std::min(availableTiles.topLeft.x, availableTiles.bottomRight.x) : -std::max(availableTiles.topLeft.x, availableTiles.bottomRight.x);
            const double minAvailableY = topToBottom ? std::min(availableTiles.topLeft.y, availableTiles.bottomRight.y) : -std::max(availableTiles.topLeft.y, availableTiles.bottomRight.y);
            const double maxAvailableX = leftToRight ? std::max(availableTiles.topLeft.x, availableTiles.bottomRight.x) : -std::min(availableTiles.topLeft.x, availableTiles.bottomRight.x);
            const double maxAvailableY = topToBottom ? std::max(availableTiles.topLeft.y, availableTiles.bottomRight.y) : -std::min(availableTiles.topLeft.y, availableTiles.bottomRight.y);


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

        hash_combine(visibleTileHash, std::hash<int>{}(i));
        hash_combine(visibleTileHash, std::hash<int>{}(startTileLeft));
        hash_combine(visibleTileHash, std::hash<int>{}(maxTileLeft));
        hash_combine(visibleTileHash, std::hash<int>{}(startTileTop));
        hash_combine(visibleTileHash, std::hash<int>{}(maxTileTop));
        hash_combine(visibleTileHash, std::hash<int>{}(zoomLevelInfo.numTilesT));

        for (int x = startTileLeft; x <= maxTileLeft && x < zoomLevelInfo.numTilesX; x++) {
            for (int y = startTileTop; y <= maxTileTop && y < zoomLevelInfo.numTilesY; y++) {
                for (int t = 0; t < zoomLevelInfo.numTilesT; t++) {

                    if( t != curT ) {
                        continue;
                    }

                    const Coord topLeft = Coord(layerSystemId, x * tileWidthAdj + boundsLeft, y * tileHeightAdj + boundsTop, 0);
                    const Coord bottomRight = Coord(layerSystemId, topLeft.x + tileWidthAdj, topLeft.y + tileHeightAdj, 0);

                    const double tileCenterX = topLeft.x + 0.5f * tileWidthAdj;
                    const double tileCenterY = topLeft.y + 0.5f * tileHeightAdj;
                    const double tileCenterDis = std::sqrt(std::pow(tileCenterX - centerVisibleX, 2.0) + std::pow(tileCenterY - centerVisibleY, 2.0));

                    float distanceFactor = (tileCenterDis / maxDisCenter) * distanceWeight;
                    float zoomlevelFactor = zoomDistanceFactor * zoomLevelWeight;
                    float zDistanceFactor = std::abs(t - curT) * zDistanceWeight;

                    const int priority = std::ceil(distanceFactor + zoomlevelFactor + zDistanceFactor);

                    const RectCoord rect(topLeft, bottomRight);
                    curVisibleTilesVec.push_back(PrioritizedTiled2dMapTileInfo(
                            Tiled2dMapTileInfo(rect, x, y, t, zoomLevelInfo.zoomLevelIdentifier, zoomLevelInfo.zoom),
                            priority));

                    visibleTilesVec.push_back(curVisibleTilesVec.back());
                }
            }
        }

        curVisibleTiles.visibleTiles.insert(curVisibleTilesVec.begin(), curVisibleTilesVec.end());

        std::unordered_set<PrioritizedTiled2dMapTileInfo> visibleTiles(visibleTilesVec.begin(), visibleTilesVec.end());
        layers.push_back(curVisibleTiles);
    }

    {
        currentZoomLevelIdentifier = targetZoomLevelIdentifier;
    }

    if (lastVisibleTilesHash != visibleTileHash) {
        lastVisibleTilesHash = visibleTileHash;
        onVisibleTilesChanged(layers, false, keepZoomLevelOffset);
    }

    currentViewBounds.topLeft = visibleBoundsLayer.topLeft;
    currentViewBounds.topRight = Coord(visibleBoundsLayer.topLeft.systemIdentifier, visibleBoundsLayer.bottomRight.x,
                                       visibleBoundsLayer.topLeft.y, 0);
    currentViewBounds.bottomRight = visibleBoundsLayer.bottomRight;
    currentViewBounds.bottomLeft = Coord(visibleBoundsLayer.topLeft.systemIdentifier, visibleBoundsLayer.topLeft.x,
                                         visibleBoundsLayer.bottomRight.y, 0);
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::onVisibleTilesChanged(const std::vector<VisibleTilesLayer> &pyramid, bool enforceMultipleLevels, int keepZoomLevelOffset) {

    currentVisibleTiles.clear();

    std::vector<PrioritizedTiled2dMapTileInfo> toAdd;

    // make sure all tiles on the current zoom level are scheduled to load (as well as those from the level we always want to keep)
    for (const auto &layer: pyramid) {
        if ((layer.targetZoomLevelOffset <= 0 && layer.targetZoomLevelOffset >= -zoomInfo.numDrawPreviousLayers) || layer.targetZoomLevelOffset == keepZoomLevelOffset){
            for (auto const &tileInfo: layer.visibleTiles) {
                currentVisibleTiles.insert(tileInfo.tileInfo);

                size_t currentTilesCount = currentTiles.count(tileInfo.tileInfo);
                size_t currentlyLoadingCount = currentlyLoading.count(tileInfo.tileInfo);
                size_t notFoundCount = notFoundTiles.count(tileInfo.tileInfo);

                if (currentTilesCount == 0 && currentlyLoadingCount == 0 && notFoundCount == 0) {
                    toAdd.push_back(tileInfo);
                }
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

        if ((!onlyCurrent && tileInfo.zoomIdentifier <= currentZoomLevelIdentifier)
            || (onlyCurrent && tileInfo.zoomIdentifier == currentZoomLevelIdentifier)
            || tileInfo.zoomIdentifier == (currentZoomLevelIdentifier + keepZoomLevelOffset)
            || enforceMultipleLevels) {
            for (const auto &layer: pyramid) {
                for (auto const &tile: layer.visibleTiles) {
                    if (tileInfo == tile.tileInfo) {
                        found = true;
                        tileWrapper.tessellationFactor = tile.tileInfo.tessellationFactor;
                        break;
                    }
                }

                if(found) { break; }
            }
        }

        if (!found) {
            toRemove.push_back(tileInfo);
        }
    }

    for (const auto &removedTile : toRemove) {
        gpc_free_polygon(&currentTiles.at(removedTile).tilePolygon);
        currentTiles.erase(removedTile);
        currentlyLoading.erase(removedTile);

        readyTiles.erase(removedTile);

        if (errorManager)
            errorManager->removeError(
                    layerConfig->getTileUrl(removedTile.x, removedTile.y, removedTile.t, removedTile.zoomIdentifier));
    }

    for (auto it = currentlyLoading.begin(); it != currentlyLoading.end();) {
        bool found = false;
        if (it->first.zoomIdentifier <= currentZoomLevelIdentifier) {
            for (const auto &layer: pyramid) {
                for (auto const &tile: layer.visibleTiles) {
                    if (it->first == tile.tileInfo) {
                        found = true;
                        break;
                    }
                }

                if(found) { break; }
            }
        }

       if(!found) {
          cancelLoad(it->first, it->second);
          it = currentlyLoading.erase(it);
       }
       else
          it++;
    }

    for (auto &[loaderIndex, errors]: errorTiles) {
        for (auto it = errors.begin(); it != errors.end();) {
            bool found = false;
            for (const auto &layer: pyramid) {
                auto visibleTile = layer.visibleTiles.find({it->first, 0});
                if (visibleTile == layer.visibleTiles.end()) {
                    found = true;
                    break;
                }
            }
            if (found) {
                if (errorManager) errorManager->removeError(layerConfig->getTileUrl(it->first.x, it->first.y, it->first.t, it->first.zoomIdentifier));
                it = errors.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::sort(toAdd.begin(), toAdd.end());

    for (const auto &addedTile : toAdd) {
        performLoadingTask(addedTile.tileInfo, 0);
    }
    //if we removed tiles, we potentially need to update the tilemasks - also if no new tile is loaded
    updateTileMasks();
    
    notifyTilesUpdates();
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::performLoadingTask(Tiled2dMapTileInfo tile, size_t loaderIndex) {
    if (currentlyLoading.count(tile) != 0) return;

    if (currentVisibleTiles.count(tile) == 0) {
        errorTiles[loaderIndex].erase(tile);
        return;
    };
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
                                        auto isStillVisible = weakActor.syncAccess([tile](auto actor){
                                            auto strongSelf = actor.lock();
                                            return strongSelf ? strongSelf->isTileVisible(tile) : false;
                                        });
                                        if (isStillVisible == false) {
                                            weakActor.message(&Tiled2dMapSource::didFailToLoad, tile, loaderIndex, LoaderStatus::ERROR_OTHER, std::nullopt);
                                        } else {
                                            weakActor.message(&Tiled2dMapSource::didLoad, tile, loaderIndex,
                                                              strongSelf->postLoadingTask(res, tile));
                                        }
                                    }
                                }));
                    }
                } else {
                    weakActor.message(&Tiled2dMapSource::didLoad, tile, loaderIndex, strongSelf->postLoadingTask(res, tile));
                }
            } else {
                weakActor.message(&Tiled2dMapSource::didFailToLoad, tile, loaderIndex, res->status, res->errorCode);
            }
        }
    });
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::didLoad(Tiled2dMapTileInfo tile, size_t loaderIndex, const R &result) {
    currentlyLoading.erase(tile);
    std::string layerName = layerConfig->getLayerName();
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
    PolygonCoord mask({ bounds.topLeft,
        Coord(bounds.topLeft.systemIdentifier, bounds.bottomRight.x, bounds.topLeft.y, 0),
        bounds.bottomRight,
        Coord(bounds.topLeft.systemIdentifier, bounds.topLeft.x, bounds.bottomRight.y, 0),
        bounds.topLeft }, {});

    gpc_polygon tilePolygon;
    gpc_set_polygon({mask}, &tilePolygon);

    currentTiles.insert({tile, TileWrapper<R>(result, std::vector<::PolygonCoord>{  }, mask, tilePolygon, tile.tessellationFactor)});

    errorTiles[loaderIndex].erase(tile);

    updateTileMasks();

    notifyTilesUpdates();
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::didFailToLoad(Tiled2dMapTileInfo tile, size_t loaderIndex, const LoaderStatus &status, const std::optional<std::string> &errorCode) {
    currentlyLoading.erase(tile);

    const bool isVisible = currentVisibleTiles.count(tile);
    if (!isVisible) {
        errorTiles[loaderIndex].erase(tile);
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
            performLoadingTask(tile, newLoaderIndex);

            break;
        }
        case LoaderStatus::ERROR_400:
        case LoaderStatus::ERROR_404: {
            notFoundTiles.insert(tile);

            errorTiles[loaderIndex].erase(tile);

            if (errorManager) {
                errorManager->addTiledLayerError(TiledLayerError(status,
                                                                 errorCode,
                                                                 layerName,
                                                                 layerConfig->getTileUrl(tile.x, tile.y, tile.t,tile.zoomIdentifier),
                                                                 false,
                                                                 tile.bounds));
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
                errorManager->addTiledLayerError(TiledLayerError(status,
                                                                 errorCode,
                                                                 layerConfig->getLayerName(),
                                                                 layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier),
                                                                 true,
                                                                 tile.bounds));
            }

            if (!nextDelayTaskExecution || nextDelayTaskExecution > now + delay ) {
                nextDelayTaskExecution = now + delay;

                auto taskIdentifier = "Tiled2dMapSource_loadingErrorTask";

                auto strongScheduler = scheduler.lock();
                if (strongScheduler) {
                    auto weakActor = WeakActor<Tiled2dMapSource>(mailbox, std::dynamic_pointer_cast<Tiled2dMapSource>(shared_from_this()));
                    strongScheduler->addTask(std::make_shared<LambdaTask>(TaskConfig(taskIdentifier, delay, TaskPriority::NORMAL, ExecutionEnvironment::IO), [weakActor] {
                        weakActor.message(&Tiled2dMapSource::performDelayedTasks);
                    }));
                }
            }
            break;
        }
    }

    updateTileMasks();

    notifyTilesUpdates();
}


template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::performDelayedTasks() {
    nextDelayTaskExecution = std::nullopt;

    const auto now = DateHelper::currentTimeMillis();
    long long minDelay = std::numeric_limits<long long>::max();

    std::vector<std::pair<int, Tiled2dMapTileInfo>> toLoad;

    for (auto &[loaderIndex, errors]: errorTiles) {
        for(auto &[tile, errorInfo]: errors) {
            if (errorInfo.lastLoad + errorInfo.delay >= now) {
                toLoad.push_back({loaderIndex, tile});
            } else {
                minDelay = std::min(minDelay, errorInfo.delay);
            }
        }
    }

    for (auto &[loaderIndex, tile]: toLoad) {
        performLoadingTask(tile, loaderIndex);
    }

    if (minDelay != std::numeric_limits<long long>::max()) {
        nextDelayTaskExecution = now + minDelay;

        auto taskIdentifier = "Tiled2dMapSource_loadingErrorTask";

        auto strongScheduler = scheduler.lock();
        if (strongScheduler) {
            auto weakActor = WeakActor<Tiled2dMapSource>(mailbox, std::dynamic_pointer_cast<Tiled2dMapSource>(shared_from_this()));
            strongScheduler->addTask(std::make_shared<LambdaTask>(TaskConfig(taskIdentifier, minDelay, TaskPriority::NORMAL, ExecutionEnvironment::IO), [weakActor] {
                weakActor.message(&Tiled2dMapSource::performDelayedTasks);
            }));
        }
    }
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::updateTileMasks() {

    if (!zoomInfo.maskTile) {
        for (auto it = currentTiles.rbegin(); it != currentTiles.rend(); it++){
            auto &[tileInfo, tileWrapper] = *it;
            if (readyTiles.count(tileInfo) == 0) {
                tileWrapper.state = TileState::IN_SETUP;
            } else {
                tileWrapper.state = TileState::VISIBLE;
            }
        }
        return;
    }

    if (currentTiles.empty() && outdatedTiles.empty()) {
        return;
    }

    int currentZoomLevelIdentifier = this->currentZoomLevelIdentifier;

    gpc_polygon currentTileMask;
    bool freeCurrent = false;
    currentTileMask.num_contours = 0;
    bool isFirst = true;

    gpc_polygon currentViewBoundsPolygon;
    gpc_set_polygon({PolygonCoord({
        currentViewBounds.topLeft,
        currentViewBounds.topRight,
        currentViewBounds.bottomRight,
        currentViewBounds.bottomLeft,
        currentViewBounds.topLeft
    }, {})}, &currentViewBoundsPolygon);

    bool completeViewBoundsDrawn = false;

    for (auto it = currentTiles.rbegin(); it != currentTiles.rend(); it++){
        auto &[tileInfo, tileWrapper] = *it;

        tileWrapper.state = TileState::VISIBLE;

        if (readyTiles.count(tileInfo) == 0) {
            tileWrapper.state = TileState::IN_SETUP;
            continue;
        }

        if (tileInfo.zoomIdentifier != currentZoomLevelIdentifier) {

            if (currentTileMask.num_contours != 0) {
                if (!completeViewBoundsDrawn) {
                    gpc_polygon diff;
                    gpc_polygon_clip(GPC_DIFF, &currentViewBoundsPolygon, &currentTileMask, &diff);

                    if (diff.num_contours == 0) {
                        completeViewBoundsDrawn = true;
                    }

                    gpc_free_polygon(&diff);
                }
            }

            if (completeViewBoundsDrawn) {
                tileWrapper.state = TileState::CACHED;
                continue;
            }

            gpc_polygon polygonDiff;
            bool freePolygonDiff = false;
            if (currentTileMask.num_contours != 0) {
                freePolygonDiff = true;
                gpc_polygon_clip(GPC_DIFF, &tileWrapper.tilePolygon, &currentTileMask, &polygonDiff);
            } else {
                polygonDiff = tileWrapper.tilePolygon;
            }

            if (polygonDiff.contour == NULL) {
                tileWrapper.state = TileState::CACHED;
                if (freePolygonDiff) {
                    gpc_free_polygon(&polygonDiff);
                }
                continue;
            } else {
                gpc_polygon resultingMask;

                gpc_polygon_clip(GPC_INT, &polygonDiff, &currentViewBoundsPolygon, &resultingMask);

                if (resultingMask.contour == NULL) {
                    tileWrapper.state = TileState::CACHED;
                    if (freePolygonDiff) {
                        gpc_free_polygon(&polygonDiff);
                    }
                    continue;
                } else {
                    tileWrapper.masks = gpc_get_polygon_coord(&polygonDiff, tileInfo.bounds.topLeft.systemIdentifier);
                }

                gpc_free_polygon(&resultingMask);
            }

            if (freePolygonDiff) {
                gpc_free_polygon(&polygonDiff);
            }
        } else {
            tileWrapper.masks = { tileWrapper.tileBounds };
        }

        // add tileBounds to currentTileMask
        if (tileWrapper.state == TileState::VISIBLE) {
            if (isFirst) {
                gpc_set_polygon({ tileWrapper.tileBounds }, &currentTileMask);
                isFirst = false;
            } else {
                gpc_polygon result;
                gpc_polygon_clip(GPC_UNION, &currentTileMask, &tileWrapper.tilePolygon, &result);
                gpc_free_polygon(&currentTileMask);
                currentTileMask = result;
            }

            freeCurrent = true;
        }
    }

    if(freeCurrent) {
        gpc_free_polygon(&currentTileMask);
    }
    gpc_free_polygon(&currentViewBoundsPolygon);
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::setTileReady(const Tiled2dMapVersionedTileInfo &tile) {
    bool needsUpdate = false;
    
    if (readyTiles.count(tile.tileInfo) == 0) {
        if (currentTiles.count(tile.tileInfo) != 0){
            readyTiles.insert(tile.tileInfo);
            outdatedTiles.erase(tile.tileInfo);
            needsUpdate = true;
        }
    }
    
    if (!needsUpdate) { return; }
    
    updateTileMasks();
    
    notifyTilesUpdates();
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::setTilesReady(const std::vector<Tiled2dMapVersionedTileInfo> &tiles) {
    bool needsUpdate = false;
    
    for (auto const &tile: tiles) {
        if (readyTiles.count(tile.tileInfo) == 0 || outdatedTiles.count(tile.tileInfo) > 0) {
            const auto &tileEntry = currentTiles.find(tile.tileInfo);
            if (tileEntry != currentTiles.end()){
                if (!zoomInfo.maskTile) {
                    tileEntry->second.state = TileState::VISIBLE;
                }
                readyTiles.insert(tile.tileInfo);
                outdatedTiles.erase(tile.tileInfo);
                needsUpdate = true;
            }
        }
    }
    
    if (!needsUpdate) { return; }

    updateTileMasks();
    notifyTilesUpdates();
}

template<class T, class L, class R>
QuadCoord Tiled2dMapSource<T, L, R>::getCurrentViewBounds() {
    return currentViewBounds;
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::setMinZoomLevelIdentifier(std::optional<int32_t> value) {
    minZoomLevelIdentifier = value;
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::setMaxZoomLevelIdentifier(std::optional<int32_t> value) {
    maxZoomLevelIdentifier = value;
}

template<class T, class L, class R>
std::optional<int32_t> Tiled2dMapSource<T, L, R>::getMinZoomLevelIdentifier() {
    return minZoomLevelIdentifier;
}

template<class T, class L, class R>
std::optional<int32_t> Tiled2dMapSource<T, L, R>::getMaxZoomLevelIdentifier() {
    return maxZoomLevelIdentifier;
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::pause() {
    isPaused = true;
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::resume() {
    isPaused = false;
}

template<class T, class L, class R>
::LayerReadyState Tiled2dMapSource<T, L, R>::isReadyToRenderOffscreen() {
    if (notFoundTiles.size() > 0) {
        return LayerReadyState::ERROR;
    }

    for (auto const &[index, errors]: errorTiles) {
        if (errors.size() > 0) {
            return LayerReadyState::ERROR;
        }
    }

    if (!currentlyLoading.empty()) {
        return LayerReadyState::NOT_READY;
    }

    for (const auto& visible : currentVisibleTiles) {
        if (currentTiles.count(visible) == 0) {
            return LayerReadyState::NOT_READY;
        }
        if (readyTiles.count(visible) == 0) {
            return LayerReadyState::NOT_READY;
        }
    }

    return LayerReadyState::READY;
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::setErrorManager(const std::shared_ptr<::ErrorManager> &errorManager) {
    this->errorManager = errorManager;
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::forceReload() {

    //set delay to 0 for all error tiles
    for (auto &[loaderIndex, errors]: errorTiles) {
        for(auto &[tile, errorInfo]: errors) {
            errorInfo.delay = 1;
            performLoadingTask(tile, loaderIndex);
        }
    }

}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::reloadTiles() {
    outdatedTiles.clear();
    outdatedTiles.insert(currentTiles.begin(), currentTiles.end());
    
    currentTiles.clear();
    readyTiles.clear();

    for (auto it = currentlyLoading.begin(); it != currentlyLoading.end(); ++it ) {
        cancelLoad(it->first, it->second);
    }
    currentlyLoading.clear();
    errorTiles.clear();

    lastVisibleTilesHash = -1;
    onVisibleTilesChanged(currentPyramid, false,currentKeepZoomLevelOffset);
}
