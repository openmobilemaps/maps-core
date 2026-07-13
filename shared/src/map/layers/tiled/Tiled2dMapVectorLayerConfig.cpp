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
#include "Logger.h"
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
    // GeoJSON sources carry an empty urlFormat (see
    // Tiled2dMapVectorLayer::getGeoJSONLayerConfig). They never fetch tiles through
    // this path, but `Tiled2dMapSource::didLoad` still asks the config for a tile URL
    // when clearing the error manager. Return an empty string there so the error
    // manager gets an empty key instead of aborting the worker thread via an
    // unhandled `std::invalid_argument`.
    if (urlFormat.empty()) {
        return {};
    }
    std::string url = urlFormat;
    auto hasAll = url.find("{z}") != std::string::npos
               && url.find("{x}") != std::string::npos
               && url.find("{y}") != std::string::npos;
    if (!hasAll) {
        // Returning the raw template instead of aborting a worker thread via an
        // unhandled `std::invalid_argument`. `Tiled2dMapSource::didLoad` calls this
        // from the tile worker, and any escape would reach `std::terminate`.
        LogError <<= "Tiled2dMapVectorLayerConfig[" + layerName + "]: urlFormat '" + urlFormat + "' lacks {z}/{x}/{y}; returning template as-is";
        return urlFormat;
    }
    size_t zoomIndex = url.find("{z}", 0);
    url = url.replace(zoomIndex, 3, std::to_string(zoom));
    size_t xIndex = url.find("{x}", 0);
    url = url.replace(xIndex, 3, std::to_string(x));
    size_t yIndex = url.find("{y}", 0);
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
