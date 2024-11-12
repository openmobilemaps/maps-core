/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "WebMercatorTiled2dMapLayerConfig.h"
#include "Tiled2dMapVectorSettings.h"
#include "CoordinateSystemIdentifiers.h"
#include <stdexcept>
#include <cmath>


const RectCoord WebMercatorTiled2dMapLayerConfig::WEB_MERCATOR_BOUNDS = RectCoord(Coord(CoordinateSystemIdentifiers::EPSG3857(), -20037508.34, 20037508.34, 0.0),
                                                                            Coord(CoordinateSystemIdentifiers::EPSG3857(), 20037508.34, -20037508.34, 0.0));
const double WebMercatorTiled2dMapLayerConfig::BASE_ZOOM = 559082264.029;
const double WebMercatorTiled2dMapLayerConfig::BASE_WIDTH = 40075016;

WebMercatorTiled2dMapLayerConfig::WebMercatorTiled2dMapLayerConfig(std::string layerName, std::string urlFormat)
    : layerName(layerName), urlFormat(urlFormat)
     {}

WebMercatorTiled2dMapLayerConfig::WebMercatorTiled2dMapLayerConfig(std::string layerName, std::string urlFormat,
                                                                   const Tiled2dMapZoomInfo &zoomInfo, int32_t minZoomLevel,
                                                                   int32_t maxZoomLevel)
        : layerName(layerName), urlFormat(urlFormat), zoomInfo(zoomInfo), minZoomLevel(minZoomLevel), maxZoomLevel(maxZoomLevel) {}

int32_t WebMercatorTiled2dMapLayerConfig::getCoordinateSystemIdentifier() { return CoordinateSystemIdentifiers::EPSG3857(); }

std::string WebMercatorTiled2dMapLayerConfig::getTileUrl(int32_t x, int32_t y, int32_t t, int32_t zoom) {
    std::string url = urlFormat;
    size_t zoomIndex = url.find("{z}", 0);
    if (zoomIndex == std::string::npos)
        throw std::invalid_argument("Layer url \'" + url + "\' has no valid format!");
    url = url.replace(zoomIndex, 3, std::to_string(zoom));
    size_t xIndex = url.find("{x}", 0);
    if (xIndex == std::string::npos)
        throw std::invalid_argument("Layer url \'" + url + "\' has no valid format!");
    url = url.replace(xIndex, 3, std::to_string(x));
    size_t yIndex = url.find("{y}", 0);
    if (yIndex == std::string::npos)
        throw std::invalid_argument("Layer url \'" + url + "\' has no valid format!");
    return url.replace(yIndex, 3, std::to_string(y));
}

std::string WebMercatorTiled2dMapLayerConfig::getLayerName() { return layerName; }

std::vector<Tiled2dMapZoomLevelInfo> WebMercatorTiled2dMapLayerConfig::getZoomLevelInfos() {
    std::vector<Tiled2dMapZoomLevelInfo> levels;
    levels.reserve(maxZoomLevel - minZoomLevel + 1);
    for (int32_t i = minZoomLevel; i <= maxZoomLevel; ++i) {
        levels.emplace_back(getZoomLevelInfo(i));
    }
    return levels;
}

std::vector<Tiled2dMapZoomLevelInfo> WebMercatorTiled2dMapLayerConfig::getVirtualZoomLevelInfos() {
    std::vector<Tiled2dMapZoomLevelInfo> levels;
    levels.reserve(minZoomLevel);
    for (int32_t i = 0; i < minZoomLevel; ++i) {
        levels.emplace_back(getZoomLevelInfo(i));
    }
    return levels;
};

Tiled2dMapZoomInfo WebMercatorTiled2dMapLayerConfig::getZoomInfo() {
    return zoomInfo;
}

std::optional<Tiled2dMapVectorSettings> WebMercatorTiled2dMapLayerConfig::getVectorSettings() {
    return std::nullopt;
}

std::optional<::RectCoord> WebMercatorTiled2dMapLayerConfig::getBounds() {
    return WEB_MERCATOR_BOUNDS;
}

Tiled2dMapZoomLevelInfo WebMercatorTiled2dMapLayerConfig::getZoomLevelInfo(int32_t zoomLevel) {
    double tileCount = std::pow(2.0,zoomLevel);
    double zoom = BASE_ZOOM / tileCount;
    return {zoom, (float) (BASE_WIDTH / tileCount), (int32_t) tileCount, (int32_t) tileCount, 1, zoomLevel, WEB_MERCATOR_BOUNDS};
}
