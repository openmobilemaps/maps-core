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
    RectCoord bounds = RectCoord(Coord(CoordinateSystemIdentifiers::EPSG4326(), -180.0, 90.0, 0.0),
                                 Coord(CoordinateSystemIdentifiers::EPSG4326(), 180.0, -90.0, 0.0));
    double baseZoom = 500000000.0;
    std::vector<Tiled2dMapZoomLevelInfo> levels;

    for (int32_t i = minZoomLevel; i <= maxZoomLevel; ++i) {
        int32_t tileCount = pow(2,i);
        double zoom = baseZoom / tileCount;
        levels.emplace_back(zoom, 360.0 / tileCount, tileCount, tileCount, 1, i, bounds);
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
    return RectCoord(Coord(CoordinateSystemIdentifiers::EPSG4326(), -180.0, 90.0, 0.0),
                     Coord(CoordinateSystemIdentifiers::EPSG4326(), 180.0, -90.0, 0.0));
}
