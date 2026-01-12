/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "CustomTiled2dMapLayerConfig.h"
#include "Tiled2dMapVectorSettings.h"
#include <cmath>
#include <stdexcept>

CustomTiled2dMapLayerConfig::CustomTiled2dMapLayerConfig(std::string layerName, std::string urlFormat,
                                                         MapCoordinateSystem coordinateSystem)
    : layerName(layerName), urlFormat(urlFormat), coordinateSystem(coordinateSystem) {}

CustomTiled2dMapLayerConfig::CustomTiled2dMapLayerConfig(std::string layerName, std::string urlFormat,
                                                         MapCoordinateSystem coordinateSystem,
                                                         const Tiled2dMapZoomInfo &zoomInfo, int32_t minZoomLevel,
                                                         int32_t maxZoomLevel)
        : layerName(layerName), urlFormat(urlFormat), coordinateSystem(coordinateSystem), zoomInfo(zoomInfo),
          minZoomLevel(minZoomLevel), maxZoomLevel(maxZoomLevel) {}

int32_t CustomTiled2dMapLayerConfig::getCoordinateSystemIdentifier() {
    return coordinateSystem.identifier;
}

std::string CustomTiled2dMapLayerConfig::getTileUrl(int32_t x, int32_t y, int32_t t, int32_t zoom) {
    std::string url = urlFormat;
    size_t zoomIndex = url.find("{z}", 0);
    if (zoomIndex == std::string::npos) {
        throw std::invalid_argument("Layer url \'" + url + "\' has no valid format!");
    }
    url = url.replace(zoomIndex, 3, std::to_string(zoom));
    size_t xIndex = url.find("{x}", 0);
    if (xIndex == std::string::npos) {
        throw std::invalid_argument("Layer url \'" + url + "\' has no valid format!");
    }
    url = url.replace(xIndex, 3, std::to_string(x));
    size_t yIndex = url.find("{y}", 0);
    if (yIndex == std::string::npos) {
        throw std::invalid_argument("Layer url \'" + url + "\' has no valid format!");
    }
    return url.replace(yIndex, 3, std::to_string(y));
}

std::string CustomTiled2dMapLayerConfig::getLayerName() {
    return layerName;
}

std::vector<Tiled2dMapZoomLevelInfo> CustomTiled2dMapLayerConfig::getZoomLevelInfos() {
    std::vector<Tiled2dMapZoomLevelInfo> levels;
    levels.reserve(maxZoomLevel - minZoomLevel + 1);
    for (int32_t i = minZoomLevel; i <= maxZoomLevel; ++i) {
        levels.emplace_back(getZoomLevelInfo(i));
    }
    return levels;
}

std::vector<Tiled2dMapZoomLevelInfo> CustomTiled2dMapLayerConfig::getVirtualZoomLevelInfos() {
    std::vector<Tiled2dMapZoomLevelInfo> levels;
    levels.reserve(minZoomLevel);
    for (int32_t i = 0; i < minZoomLevel; ++i) {
        levels.emplace_back(getZoomLevelInfo(i));
    }
    return levels;
}

Tiled2dMapZoomInfo CustomTiled2dMapLayerConfig::getZoomInfo() {
    return zoomInfo;
}

std::optional<Tiled2dMapVectorSettings> CustomTiled2dMapLayerConfig::getVectorSettings() {
    return std::nullopt;
}

std::optional<::RectCoord> CustomTiled2dMapLayerConfig::getBounds() {
    return coordinateSystem.bounds;
}

Tiled2dMapZoomLevelInfo CustomTiled2dMapLayerConfig::getZoomLevelInfo(int32_t zoomLevel) const {
    double tileCount = std::pow(2.0, zoomLevel);
    double zoom = BASE_ZOOM / tileCount;
    double baseWidth = std::abs(coordinateSystem.bounds.bottomRight.x - coordinateSystem.bounds.topLeft.x);
    return {zoom, static_cast<float>(baseWidth / tileCount), static_cast<int32_t>(tileCount),
            static_cast<int32_t>(tileCount), 1, zoomLevel, coordinateSystem.bounds};
}
