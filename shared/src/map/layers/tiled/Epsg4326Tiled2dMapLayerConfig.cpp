/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Epsg4326Tiled2dMapLayerConfig.h"
#include "Tiled2dMapVectorSettings.h"
#include "CoordinateSystemIdentifiers.h"
#include <stdexcept>
#include <cmath>

const RectCoord Epsg4326Tiled2dMapLayerConfig::EPSG_4326_BOUNDS = RectCoord(Coord(CoordinateSystemIdentifiers::EPSG4326(), -180.0, 90.0, 0.0),
                                                                            Coord(CoordinateSystemIdentifiers::EPSG4326(), 180.0, -90.0, 0.0));
const double Epsg4326Tiled2dMapLayerConfig::BASE_ZOOM = 500000000.0;
const double Epsg4326Tiled2dMapLayerConfig::BASE_WIDTH = 360.0;

Epsg4326Tiled2dMapLayerConfig::Epsg4326Tiled2dMapLayerConfig(std::string layerName, std::string urlFormat)
    : layerName(layerName), urlFormat(urlFormat)
     {}

Epsg4326Tiled2dMapLayerConfig::Epsg4326Tiled2dMapLayerConfig(std::string layerName, std::string urlFormat,
                                                             const Tiled2dMapZoomInfo &zoomInfo, int32_t minZoomLevel,
                                                             int32_t maxZoomLevel)
        : layerName(layerName), urlFormat(urlFormat), zoomInfo(zoomInfo), minZoomLevel(minZoomLevel), maxZoomLevel(maxZoomLevel) {}

int32_t Epsg4326Tiled2dMapLayerConfig::getCoordinateSystemIdentifier() { return CoordinateSystemIdentifiers::EPSG4326(); }

std::string Epsg4326Tiled2dMapLayerConfig::getTileUrl(int32_t x, int32_t y, int32_t t, int32_t zoom) {
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

std::string Epsg4326Tiled2dMapLayerConfig::getLayerName() { return layerName; }

std::vector<Tiled2dMapZoomLevelInfo> Epsg4326Tiled2dMapLayerConfig::getZoomLevelInfos() {
    std::vector<Tiled2dMapZoomLevelInfo> levels;
    levels.reserve(maxZoomLevel - minZoomLevel + 1);
    for (int32_t i = minZoomLevel; i <= maxZoomLevel; ++i) {
        levels.emplace_back(getZoomLevelInfo(i));
    }
    return levels;
}

std::vector<Tiled2dMapZoomLevelInfo> Epsg4326Tiled2dMapLayerConfig::getVirtualZoomLevelInfos() {
    std::vector<Tiled2dMapZoomLevelInfo> levels;
    levels.reserve(minZoomLevel);
    for (int32_t i = 0; i < minZoomLevel; ++i) {
        levels.emplace_back(getZoomLevelInfo(i));
    }
    return levels;
}

Tiled2dMapZoomInfo Epsg4326Tiled2dMapLayerConfig::getZoomInfo() {
    return zoomInfo;
}

std::optional<Tiled2dMapVectorSettings> Epsg4326Tiled2dMapLayerConfig::getVectorSettings() {
    return std::nullopt;
}

std::optional<::RectCoord> Epsg4326Tiled2dMapLayerConfig::getBounds() {
    return EPSG_4326_BOUNDS;
}

Tiled2dMapZoomLevelInfo Epsg4326Tiled2dMapLayerConfig::getZoomLevelInfo(int32_t zoomLevel) {
    double tileCount = std::pow(2.0,zoomLevel);
    double zoom = BASE_ZOOM / tileCount;
    return {zoom, (float) (BASE_WIDTH / tileCount), (int32_t) tileCount, (int32_t) tileCount, 1, zoomLevel, EPSG_4326_BOUNDS};
}
