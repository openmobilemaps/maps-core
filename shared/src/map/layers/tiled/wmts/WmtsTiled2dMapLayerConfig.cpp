/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "WmtsTiled2dMapLayerConfig.h"

#include "Tiled2dMapVectorSettings.h"
#include "Logger.h"
#include <cstring>

WmtsTiled2dMapLayerConfig::WmtsTiled2dMapLayerConfig(const WmtsLayerDescription &description,
                                                     const std::vector<Tiled2dMapZoomLevelInfo> &zoomLevelInfo,
                                                     const Tiled2dMapZoomInfo &zoomInfo,
                                                     const int32_t coordinateSystemIdentifier,
                                                     const std::string &matrixSetIdentifier)
    : description(description)
    , zoomLevelInfo(zoomLevelInfo)
    , zoomInfo(zoomInfo)
    , coordinateSystemIdentifier(coordinateSystemIdentifier)
    , matrixSetIdentifier(matrixSetIdentifier) {}

int32_t WmtsTiled2dMapLayerConfig::getCoordinateSystemIdentifier() { return coordinateSystemIdentifier; }

std::string WmtsTiled2dMapLayerConfig::getTileUrl(int32_t x, int32_t y, int32_t t, int32_t zoom) {
    std::string urlFormat = description.resourceTemplate;

    if (auto it = urlFormat.find("{TileMatrix}"); it != std::string::npos) {
        urlFormat.replace(it, std::strlen("{TileMatrix}"), std::to_string(zoom));
    }
    if (auto it = urlFormat.find("{TileMatrixSet}"); it != std::string::npos) {
        urlFormat.replace(it, std::strlen("{TileMatrixSet}"), matrixSetIdentifier);
    }
    if (auto it = urlFormat.find("{TileRow}"); it != std::string::npos) {
        urlFormat.replace(it, std::strlen("{TileRow}"), std::to_string(y));
    }
    if (auto it = urlFormat.find("{TileCol}"); it != std::string::npos) {
        urlFormat.replace(it, std::strlen("{TileCol}"), std::to_string(x));
    }

    for (auto const &dimension : description.dimensions) {
        auto placeHolder = "{" + dimension.identifier + "}";
        auto it = urlFormat.find(placeHolder);
        if (it != std::string::npos) {
            if (0 <= t && t < dimension.values.size()) {
                urlFormat.replace(it, placeHolder.length(), dimension.values[t]);
            } else {
                urlFormat.replace(it, placeHolder.length(), dimension.defaultValue);
            }
        }
    }

    return urlFormat;
}

std::string WmtsTiled2dMapLayerConfig::getLayerName() { return description.identifier; }

std::vector<Tiled2dMapZoomLevelInfo> WmtsTiled2dMapLayerConfig::getZoomLevelInfos() { return zoomLevelInfo; }

Tiled2dMapZoomInfo WmtsTiled2dMapLayerConfig::getZoomInfo() { return zoomInfo; }

std::optional<Tiled2dMapVectorSettings> WmtsTiled2dMapLayerConfig::getVectorSettings() {
    return std::nullopt;
}

std::optional<::RectCoord> WmtsTiled2dMapLayerConfig::getBounds() {
    return description.bounds;
}
