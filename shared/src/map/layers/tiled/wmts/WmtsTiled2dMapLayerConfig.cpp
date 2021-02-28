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

WmtsTiled2dMapLayerConfig::WmtsTiled2dMapLayerConfig(const WmtsLayerConfiguration &configuration,
                          std::vector<Tiled2dMapZoomLevelInfo> zoomLevelInfo,
                                                     const Tiled2dMapZoomInfo &zoomInfo):
configuration(configuration),
zoomLevelInfo(zoomLevelInfo),
zoomInfo(zoomInfo)
{}

::RectCoord WmtsTiled2dMapLayerConfig::getBounds() {
    return configuration.bounds;
}

std::string WmtsTiled2dMapLayerConfig::getTileUrl(int32_t x, int32_t y, int32_t zoom) {
    std::string urlFormat = configuration.urlFormat;

    if (auto it = urlFormat.find("{TileMatrix}")) {
        urlFormat.replace(it, std::strlen("{TileMatrix}"), std::to_string(zoom));
    }
    if (auto it = urlFormat.find("{TileRow}")) {
        urlFormat.replace(it, std::strlen("{TileRow}"), std::to_string(y));
    }
    if (auto it = urlFormat.find("{TileCol}")) {
        urlFormat.replace(it, std::strlen("{TileCol}"), std::to_string(x));
    }

    for (auto const &dimension: configuration.dimensions) {
        auto placeHolder = "{" + dimension.first + "}";
        if (auto it = urlFormat.find(placeHolder)) {
            urlFormat.replace(it, placeHolder.length(), dimension.second);
        }
    }

    return urlFormat;
}

std::string WmtsTiled2dMapLayerConfig::getTileIdentifier(int32_t x, int32_t y, int32_t zoom) {
    return "<" + configuration.indentifier + "x:" + std::to_string(x) + "y:" + std::to_string(y) + "zoom:" +  std::to_string(zoom) + ">";
}

std::vector<Tiled2dMapZoomLevelInfo> WmtsTiled2dMapLayerConfig::getZoomLevelInfos() {
    return zoomLevelInfo;
}

Tiled2dMapZoomInfo WmtsTiled2dMapLayerConfig::getZoomInfo() {
    return zoomInfo;
}
