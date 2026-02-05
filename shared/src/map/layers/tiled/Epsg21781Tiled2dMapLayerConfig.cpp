/*
 * Copyright (c) 2026 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Epsg21781Tiled2dMapLayerConfig.h"
#include "CoordinateSystemIdentifiers.h"
#include "Epsg2056Tiled2dMapLayerConfig.h"
#include "RectCoord.h"

Epsg21781Tiled2dMapLayerConfig::Epsg21781Tiled2dMapLayerConfig(
    std::string layerName,
    std::string urlFormat)
  : IrregularTiled2dMapLayerConfig(
        layerName, urlFormat, std::nullopt, 
        defaultMapZoomInfo(),
        generateLevelsFromMinMax(0, 28), 
        swisstopoZoomLevelInfos())
{}

Epsg21781Tiled2dMapLayerConfig::Epsg21781Tiled2dMapLayerConfig(
    std::string layerName,
    std::string urlFormat,
    const std::optional<RectCoord> &bounds,
    const Tiled2dMapZoomInfo &zoomInfo,
    const std::vector<int> &levels)
  : IrregularTiled2dMapLayerConfig(
        layerName, urlFormat, bounds, zoomInfo, levels, swisstopoZoomLevelInfos())
{}

static Tiled2dMapZoomLevelInfo makeZoomLevelInfo(
        const Coord &topLeft, double zoom, float tileWidthLayerSystemUnits,
        int32_t numTilesX, int32_t numTilesY, int32_t numTilesT, int32_t zoomLevelIdentifier) 
{
    const auto bounds = RectCoord(
        topLeft,
        Coord(topLeft.systemIdentifier,
              topLeft.x + tileWidthLayerSystemUnits * numTilesX,
              topLeft.y - tileWidthLayerSystemUnits * numTilesY, 0));

    return Tiled2dMapZoomLevelInfo(zoom, tileWidthLayerSystemUnits, numTilesX,
                                   numTilesY, numTilesT, zoomLevelIdentifier,
                                   bounds);
}

std::vector<Tiled2dMapZoomLevelInfo> Epsg21781Tiled2dMapLayerConfig::swisstopoZoomLevelInfos() {
    // same zoom level pyramid as for 2056, just different coordinate origin.
    const Coord topLeft{CoordinateSystemIdentifiers::EPSG21781(), 420000.0, 350000.0, 0.};

    std::vector<Tiled2dMapZoomLevelInfo> result;
    const auto epsg2056ZoomLevelInfos = Epsg2056Tiled2dMapLayerConfig::swisstopoZoomLevelInfos();
    for(const auto &other : epsg2056ZoomLevelInfos) {
          result.push_back(makeZoomLevelInfo(
              topLeft, other.zoom, other.tileWidthLayerSystemUnits,
              other.numTilesX, other.numTilesY, other.numTilesT,
              other.zoomLevelIdentifier));
    }
    return result;
}
