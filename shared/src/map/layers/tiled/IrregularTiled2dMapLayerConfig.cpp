/*
 * Copyright (c) 2026 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "IrregularTiled2dMapLayerConfig.h"

std::vector<Tiled2dMapZoomLevelInfo> IrregularTiled2dMapLayerConfig::getZoomLevelInfos() {
    std::vector<Tiled2dMapZoomLevelInfo> result;
    result.reserve(levels.size());
    for(int level : levels) {
        result.push_back(zoomLevelInfos[level]);
    }
    return result;
}
    
std::vector<Tiled2dMapZoomLevelInfo> IrregularTiled2dMapLayerConfig::getVirtualZoomLevelInfos() {
        const int minZoomLevel = levels.front();
        std::vector<Tiled2dMapZoomLevelInfo> result(zoomLevelInfos.begin(), zoomLevelInfos.begin() + minZoomLevel);
        return result;
    };
    
double IrregularTiled2dMapLayerConfig::getZoomIdentifier(double zoom) {
    const int minZoomLevel = levels.front();
    const int maxZoomLevel = levels.back();

    Tiled2dMapZoomLevelInfo prevZoom = zoomLevelInfos.back();

    for (auto it = zoomLevelInfos.rbegin() + maxZoomLevel; it != zoomLevelInfos.rend() + minZoomLevel; ++it) {
        if ((*it).zoom <= zoom) {
            prevZoom = *it;
        } else {
            // Linear Interpolation
            auto y0 = prevZoom.zoomLevelIdentifier;
            auto x0 = prevZoom.zoom;
            auto y1 = (*it).zoomLevelIdentifier;
            auto x1 = (*it).zoom;

            return y0 + (zoom - x0) * (y1 - y0) / (x1 - x0);
        }
    }

    return prevZoom.zoomLevelIdentifier;
}

double IrregularTiled2dMapLayerConfig::getZoomFactorAtIdentifier(double zoomIdentifier) {
    const int minZoomLevel = levels.front();
    const int maxZoomLevel = levels.back();

    Tiled2dMapZoomLevelInfo prevZoom = zoomLevelInfos.back();

    for (auto it = zoomLevelInfos.begin() + minZoomLevel; it != zoomLevelInfos.end() + maxZoomLevel; ++it) {
        if ((*it).zoomLevelIdentifier <= zoomIdentifier) {
            prevZoom = *it;
        } else {
            // Linear Interpolation
            auto y0 = prevZoom.zoom;
            auto x0 = prevZoom.zoomLevelIdentifier;
            auto y1 = (*it).zoom;
            auto x1 = (*it).zoomLevelIdentifier;

            return y0 + (zoomIdentifier - x0) * (y1 - y0) / (x1 - x0);
        }
    }

    return prevZoom.zoom;
}
