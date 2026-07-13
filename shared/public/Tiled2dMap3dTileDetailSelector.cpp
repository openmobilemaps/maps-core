/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMap3dTileDetailSelector.h"

#include "Matrix.h"
#include "TrigonometryLUT.h"
#include "Vec2D.h"
#include "Vec2DHelper.h"

#include <algorithm>
#include <cmath>

Vec3D Tiled2dMap3dScreenSpaceEdgeLengthSelector::transformToView(const Coord &position, const std::vector<float> &viewMatrix,
                                                                 const Vec3D &origin,
                                                                 CoordinateConversionHelperInterface &conversionHelper) {
    const auto mapCoord = conversionHelper.convertToRenderSystem(position);

    double sinX, cosX, sinY, cosY;
    lut::sincos(mapCoord.y, sinY, cosY);
    lut::sincos(mapCoord.x, sinX, cosX);

    static thread_local std::vector<float> inVec(4);
    static thread_local std::vector<float> outVec(4);

    inVec[0] = static_cast<float>(mapCoord.z * sinY * cosX - origin.x);
    inVec[1] = static_cast<float>(mapCoord.z * cosY - origin.y);
    inVec[2] = static_cast<float>(-mapCoord.z * sinY * sinX - origin.z);
    inVec[3] = 1.0f;

    Matrix::multiply(viewMatrix, inVec, outVec);

    return Vec3D(outVec[0] / outVec[3], outVec[1] / outVec[3], outVec[2] / outVec[3]);
}

Vec3D Tiled2dMap3dScreenSpaceEdgeLengthSelector::projectToScreen(const Vec3D &position,
                                                                 const std::vector<float> &projectionMatrix) {
    static thread_local std::vector<float> inVec(4);
    static thread_local std::vector<float> outVec(4);

    inVec[0] = static_cast<float>(position.x);
    inVec[1] = static_cast<float>(position.y);
    inVec[2] = static_cast<float>(position.z);
    inVec[3] = 1.0f;

    Matrix::multiply(projectionMatrix, inVec, outVec);

    return Vec3D(outVec[0] / outVec[3], outVec[1] / outVec[3], outVec[2] / outVec[3]);
}

bool Tiled2dMap3dScreenSpaceEdgeLengthSelector::isPreciseEnough(const Tiled2dMap3dTileDetailSelectionContext &context) const {
    constexpr double sampleSize = 0.25;
    const auto focusPointClampedToTile = Coord(
        context.layerSystemId,
        std::clamp(context.focusPointInLayerCoords.x, std::min(context.tileBounds.topLeft.x, context.tileBounds.bottomRight.x),
                   std::max(context.tileBounds.topLeft.x, context.tileBounds.bottomRight.x)),
        std::clamp(context.focusPointInLayerCoords.y, std::min(context.tileBounds.topLeft.y, context.tileBounds.bottomRight.y),
                   std::max(context.tileBounds.topLeft.y, context.tileBounds.bottomRight.y)),
        context.focusPointAltitude);
    const bool toRight = focusPointClampedToTile.x < context.tileCenter.x;
    const bool toTop = focusPointClampedToTile.y < context.tileCenter.y;

    const auto focusPointSampleX =
        Coord(context.layerSystemId,
              focusPointClampedToTile.x +
                  (toRight ? 1.0 : -1.0) * std::abs(context.tileBounds.bottomRight.x - context.tileBounds.topLeft.x) * sampleSize,
              focusPointClampedToTile.y, focusPointClampedToTile.z);

    const auto focusPointSampleY =
        Coord(context.layerSystemId, focusPointClampedToTile.x,
              focusPointClampedToTile.y +
                  (toTop ? -1.0 : 1.0) * std::abs(context.tileBounds.bottomRight.y - context.tileBounds.topLeft.y) * sampleSize,
              focusPointClampedToTile.z);

    const auto samplePointOriginViewScreen =
        projectToScreen(transformToView(focusPointClampedToTile, context.viewMatrix, context.origin, context.conversionHelper),
                        context.projectionMatrix);
    const auto samplePointYViewScreen = projectToScreen(
        transformToView(focusPointSampleY, context.viewMatrix, context.origin, context.conversionHelper), context.projectionMatrix);
    const auto samplePointXViewScreen = projectToScreen(
        transformToView(focusPointSampleX, context.viewMatrix, context.origin, context.conversionHelper), context.projectionMatrix);

    Vec2D samplePointOriginViewScreenPx(samplePointOriginViewScreen.x * (context.width / 2.0),
                                        samplePointOriginViewScreen.y * (context.height / 2.0));
    Vec2D samplePointYViewScreenPx(samplePointYViewScreen.x * (context.width / 2.0),
                                   samplePointYViewScreen.y * (context.height / 2.0));
    Vec2D samplePointXViewScreenPx(samplePointXViewScreen.x * (context.width / 2.0),
                                   samplePointXViewScreen.y * (context.height / 2.0));

    const double xLengthPx = Vec2DHelper::distance(samplePointOriginViewScreenPx, samplePointXViewScreenPx);
    const double yLengthPx = Vec2DHelper::distance(samplePointOriginViewScreenPx, samplePointYViewScreenPx);
    const double maxLength = sampleSize * (std::min(context.width, context.height) * 0.5 / context.zoomLevelScaleFactor);
    return xLengthPx <= maxLength || yLengthPx <= maxLength;
}
