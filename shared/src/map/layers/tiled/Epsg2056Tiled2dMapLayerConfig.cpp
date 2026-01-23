/*
 * Copyright (c) 2026 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "RectCoord.h"
#include "CoordinateSystemIdentifiers.h"
#include "Epsg2056Tiled2dMapLayerConfig.h"

Epsg2056Tiled2dMapLayerConfig::Epsg2056Tiled2dMapLayerConfig(
    std::string layerName,
    std::string urlFormat)
  : IrregularTiled2dMapLayerConfig(
        layerName, urlFormat, std::nullopt, 
        defaultMapZoomInfo(),
        generateLevelsFromMinMax(0, 28), 
        swisstopoZoomLevelInfos())
{}

Epsg2056Tiled2dMapLayerConfig::Epsg2056Tiled2dMapLayerConfig(
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

std::vector<Tiled2dMapZoomLevelInfo> Epsg2056Tiled2dMapLayerConfig::swisstopoZoomLevelInfos() {
    const Coord topLeft{CoordinateSystemIdentifiers::EPSG2056(), 2420000.0, 1350000.0, 0.f};
    return {
        makeZoomLevelInfo(topLeft, 14285714.2857, 1024000, 1, 1, 1, 0),
        makeZoomLevelInfo(topLeft, 13392857.1429, 960000, 1, 1, 1, 1),
        makeZoomLevelInfo(topLeft, 12500000.0, 896000, 1, 1, 1, 2),
        makeZoomLevelInfo(topLeft, 11607142.8571, 832000, 1, 1, 1, 3),
        makeZoomLevelInfo(topLeft, 10714285.7143, 768000, 1, 1, 1, 4),
        makeZoomLevelInfo(topLeft, 9821428.57143, 704000, 1, 1, 1, 5),
        makeZoomLevelInfo(topLeft, 8928571.42857, 640000, 1, 1, 1, 6),
        makeZoomLevelInfo(topLeft, 8035714.28571, 576000, 1, 1, 1, 7),
        makeZoomLevelInfo(topLeft, 7142857.14286, 512000, 1, 1, 1, 8),
        makeZoomLevelInfo(topLeft, 6250000.0, 448000, 2, 1, 1, 9),
        makeZoomLevelInfo(topLeft, 5357142.85714, 384000, 2, 1, 1, 10),
        makeZoomLevelInfo(topLeft, 4464285.71429, 320000, 2, 1, 1, 11),
        makeZoomLevelInfo(topLeft, 3571428.57143, 256000, 2, 2, 1, 12),
        makeZoomLevelInfo(topLeft, 2678571.42857, 192000, 3, 2, 1, 13),
        makeZoomLevelInfo(topLeft, 2321428.57143, 166400, 3, 2, 1, 14),
        makeZoomLevelInfo(topLeft, 1785714.28571, 128000, 4, 3, 1, 15),
        makeZoomLevelInfo(topLeft, 892857.142857, 64000, 8, 5, 1, 16),
        makeZoomLevelInfo(topLeft, 357142.857143, 25600, 19, 13, 1, 17),
        makeZoomLevelInfo(topLeft, 178571.428571, 12800, 38, 25, 1, 18),
        makeZoomLevelInfo(topLeft, 71428.5714286, 5120, 94, 63, 1, 19),
        makeZoomLevelInfo(topLeft, 35714.2857143, 2560, 188, 125, 1, 20),
        makeZoomLevelInfo(topLeft, 17857.1428571, 1280, 375, 250, 1, 21),
        makeZoomLevelInfo(topLeft, 8928.57142857, 640, 750, 500, 1, 22),
        makeZoomLevelInfo(topLeft, 7142.85714286, 512, 938, 625, 1, 23),
        makeZoomLevelInfo(topLeft, 5357.14285714, 384, 1250, 834, 1, 24),
        makeZoomLevelInfo(topLeft, 3571.42857143, 256, 1875, 1250, 1, 25),
        makeZoomLevelInfo(topLeft, 1785.71428571, 128, 3750, 2500, 1, 26),
        makeZoomLevelInfo(topLeft, 892.857142857, 64, 7500, 5000, 1, 27),
        makeZoomLevelInfo(topLeft, 357.142857143, 25.6, 18750, 12500, 1, 28)
    };
}
