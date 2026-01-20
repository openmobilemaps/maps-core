/*
 * Copyright (c) 2026 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorLayerConfig.h"
#include <algorithm>
#include <numeric>
#include <stdexcept>

Tiled2dMapVectorLayerConfig::Tiled2dMapVectorLayerConfig(
    std::string layerName,
    std::string urlFormat,
    const std::optional<RectCoord> &bounds,
    const Tiled2dMapZoomInfo &zoomInfo,
    const std::vector<int> &levels_
)
  : layerName(layerName)
  , urlFormat(urlFormat)
  , bounds(bounds)
  , zoomInfo(zoomInfo)
  , levels(levels_)
{
    std::sort(levels.begin(), levels.end());
}

std::string Tiled2dMapVectorLayerConfig::getTileUrl(int32_t x, int32_t y, int32_t t, int32_t zoom) {
    std::string url = urlFormat;
    size_t zoomIndex = url.find("{z}", 0);
    if (zoomIndex == std::string::npos) throw std::invalid_argument("Layer url \'" + url + "\' has no valid format!");
    url = url.replace(zoomIndex, 3, std::to_string(zoom));
    size_t xIndex = url.find("{x}", 0);
    if (xIndex == std::string::npos) throw std::invalid_argument("Layer url \'" + url + "\' has no valid format!");
    url = url.replace(xIndex, 3, std::to_string(x));
    size_t yIndex = url.find("{y}", 0);
    if (yIndex == std::string::npos) throw std::invalid_argument("Layer url \'" + url + "\' has no valid format!");
    return url.replace(yIndex, 3, std::to_string(y));
}

Tiled2dMapZoomInfo Tiled2dMapVectorLayerConfig::defaultMapZoomInfo() {
    return Tiled2dMapZoomInfo(1.0, 0, 0, true, false, true, true);
}

std::vector<int> Tiled2dMapVectorLayerConfig::generateLevelsFromMinMax(int minZoomLevel, int maxZoomLevel) {
    std::vector<int> levels(maxZoomLevel - minZoomLevel + 1);
    std::iota(levels.begin(), levels.end(), minZoomLevel);
    return levels;
}
