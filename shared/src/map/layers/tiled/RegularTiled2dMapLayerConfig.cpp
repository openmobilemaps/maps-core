/*
 * Copyright (c) 2026 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "RegularTiled2dMapLayerConfig.h"
#include <cmath>

std::vector<Tiled2dMapZoomLevelInfo> RegularTiled2dMapLayerConfig::getZoomLevelInfos() {
    std::vector<Tiled2dMapZoomLevelInfo> result;
    result.reserve(levels.size());
    for(int level : levels) {
        result.push_back(getRegularZoomLevelInfo(level));
    }
    return result;
}

std::vector<Tiled2dMapZoomLevelInfo> RegularTiled2dMapLayerConfig::getVirtualZoomLevelInfos() {
    int minZoomLevel = levels.front();
    std::vector<Tiled2dMapZoomLevelInfo> result;
    result.reserve(minZoomLevel);
    for(int level = 0; level < minZoomLevel; level++) {
        result.push_back(getRegularZoomLevelInfo(level));
    }
    return result;
};

double RegularTiled2dMapLayerConfig::getZoomIdentifier(double zoom) {
    return std::max(0.0, std::round(log(baseZoom * zoomInfo.zoomLevelScaleFactor / zoom) / log(2) * 100) / 100);
}

double RegularTiled2dMapLayerConfig::getZoomFactorAtIdentifier(double zoomIdentifier) {
    double factor = pow(2, zoomIdentifier);
    return baseZoom * zoomInfo.zoomLevelScaleFactor / factor;
}

Tiled2dMapZoomLevelInfo RegularTiled2dMapLayerConfig::getRegularZoomLevelInfo(int zoomLevel) {
    const double baseValueWidth = (coordinateSystem.bounds.bottomRight.x - coordinateSystem.bounds.topLeft.x);
    double factor = pow(2, zoomLevel);
    double zoom = baseZoom / factor;
    double width = baseValueWidth / factor;
    return Tiled2dMapZoomLevelInfo(zoom, width, factor, factor, 1, zoomLevel, coordinateSystem.bounds);
}
