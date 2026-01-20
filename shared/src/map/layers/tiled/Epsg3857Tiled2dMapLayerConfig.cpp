/*
 * Copyright (c) 2026 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Epsg3857Tiled2dMapLayerConfig.h"
#include "CoordinateSystemFactory.h"

const double Epsg3857Tiled2dMapLayerConfig::BASE_ZOOM = 559082264.029;

Epsg3857Tiled2dMapLayerConfig::Epsg3857Tiled2dMapLayerConfig(
    std::string layerName,
    std::string urlFormat)
  : RegularTiled2dMapLayerConfig(
        layerName, urlFormat, std::nullopt, 
        defaultMapZoomInfo(),
        generateLevelsFromMinMax(0, 20),
        CoordinateSystemFactory::getEpsg3857System(),
        BASE_ZOOM)
{}

Epsg3857Tiled2dMapLayerConfig::Epsg3857Tiled2dMapLayerConfig(
    std::string layerName,
    std::string urlFormat,
    const std::optional<RectCoord> &bounds,
    const Tiled2dMapZoomInfo &zoomInfo,
    const std::vector<int> &levels)
  : RegularTiled2dMapLayerConfig(
        layerName, urlFormat, bounds, zoomInfo, levels, 
        CoordinateSystemFactory::getEpsg3857System(), BASE_ZOOM)
{}

std::string Epsg3857Tiled2dMapLayerConfig::getTileUrl(int32_t x, int32_t y, int32_t t, int32_t zoom) {
    std::string url = urlFormat;
    size_t epsg3857Index = url.find("{bbox-epsg-3857}", 0);
    if (epsg3857Index != std::string::npos) {
        const auto &zoomLevelInfo = getRegularZoomLevelInfo(zoom);
        RectCoord levelBounds = zoomLevelInfo.bounds;
        const double tileWidth = zoomLevelInfo.tileWidthLayerSystemUnits;

        const bool leftToRight = levelBounds.topLeft.x < levelBounds.bottomRight.x;
        const bool topToBottom = levelBounds.topLeft.y < levelBounds.bottomRight.y;
        const double tileWidthAdj = leftToRight ? tileWidth : -tileWidth;
        const double tileHeightAdj = topToBottom ? tileWidth : -tileWidth;

        const double boundsLeft = levelBounds.topLeft.x;
        const double boundsTop = levelBounds.topLeft.y;

        const Coord topLeft = Coord(coordinateSystem.identifier, x * tileWidthAdj + boundsLeft, y * tileHeightAdj + boundsTop, 0);
        const Coord bottomRight = Coord(coordinateSystem.identifier, topLeft.x + tileWidthAdj, topLeft.y + tileHeightAdj, 0);

        std::string boxString = std::to_string(topLeft.x) + "," + std::to_string(bottomRight.y) + "," + std::to_string(bottomRight.x) + "," + std::to_string(topLeft.y);
        url = url.replace(epsg3857Index, 16, boxString);
        return url;
    }
    return RegularTiled2dMapLayerConfig::getTileUrl(x, y, t, zoom);
}
