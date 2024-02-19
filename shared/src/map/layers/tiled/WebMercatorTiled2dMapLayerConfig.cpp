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

#include "Logger.h"

WebMercatorTiled2dMapLayerConfig::WebMercatorTiled2dMapLayerConfig(std::string layerName, std::string urlFormat)
    : layerName(layerName), urlFormat(urlFormat)
     {}

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
    int32_t identifer = CoordinateSystemIdentifiers::EPSG3857();
    Coord topLeft = Coord(identifer, -20037508.34, 20037508.34, 0.0);
    Coord bottomRight = Coord(identifer, 20037508.34, -20037508.34, 0.0);
    auto bounds = RectCoord(topLeft, bottomRight);

    std::vector<Tiled2dMapZoomLevelInfo> levels = {
        Tiled2dMapZoomLevelInfo(559082264.029, 40075016, 1, 1, 1, 0, bounds),
        Tiled2dMapZoomLevelInfo(279541132.015, 20037508, 2, 2, 1, 1, bounds),
        Tiled2dMapZoomLevelInfo(139770566.007, 10018754, 4, 4, 1, 2, bounds),
        Tiled2dMapZoomLevelInfo(69885283.0036, 5009377.1, 8, 8, 1, 3, bounds),
        Tiled2dMapZoomLevelInfo(34942641.5018, 2504688.5, 16, 16, 1, 4, bounds),
        Tiled2dMapZoomLevelInfo(17471320.7509, 1252344.3, 32, 32, 1, 5, bounds),
        Tiled2dMapZoomLevelInfo(8735660.37545, 626172.1, 64, 64, 1, 6, bounds),
        Tiled2dMapZoomLevelInfo(4367830.18773, 313086.1, 128, 128, 1, 7, bounds),
        Tiled2dMapZoomLevelInfo(2183915.09386, 156543, 256, 256, 1, 8, bounds),
        Tiled2dMapZoomLevelInfo(1091957.54693, 78271.5, 512, 512, 1, 9, bounds),
        Tiled2dMapZoomLevelInfo(545978.773466, 39135.8, 1024, 1024, 1, 10, bounds),
        Tiled2dMapZoomLevelInfo(272989.386733, 19567.9, 2048, 2048, 1, 11, bounds),
        Tiled2dMapZoomLevelInfo(136494.693366, 9783.94, 4096, 4096, 1, 12, bounds),
        Tiled2dMapZoomLevelInfo(68247.3466832, 4891.97, 8192, 8192, 1, 13, bounds),
        Tiled2dMapZoomLevelInfo(34123.6733416, 2445.98, 16384, 16384, 1, 14, bounds),
        Tiled2dMapZoomLevelInfo(17061.8366708, 1222.99, 32768, 32768, 1, 15, bounds),
        Tiled2dMapZoomLevelInfo(8530.91833540, 611.496, 65536, 65536, 1, 16, bounds),
        Tiled2dMapZoomLevelInfo(4265.45916770, 305.748, 131072, 131072, 1, 17, bounds),
        Tiled2dMapZoomLevelInfo(2132.72958385, 152.874, 262144, 262144, 1, 18, bounds),
        Tiled2dMapZoomLevelInfo(1066.36479193, 76.437, 524288, 524288, 1, 19, bounds),
        Tiled2dMapZoomLevelInfo(533.18239597, 38.2185, 1048576, 1048576, 1, 20, bounds),
    };

    return levels;
}

Tiled2dMapZoomInfo WebMercatorTiled2dMapLayerConfig::getZoomInfo() {
    return Tiled2dMapZoomInfo(0.65, 0, true, false, true, true);
}

std::optional<Tiled2dMapVectorSettings> WebMercatorTiled2dMapLayerConfig::getVectorSettings() {
    return std::nullopt;
}

std::optional<::RectCoord> WebMercatorTiled2dMapLayerConfig::getBounds() {
    int32_t identifer = CoordinateSystemIdentifiers::EPSG3857();
    Coord topLeft = Coord(identifer, -20037508.34, 20037508.34, 0.0);
    Coord bottomRight = Coord(identifer, 20037508.34, -20037508.34, 0.0);
    return RectCoord(topLeft, bottomRight);
}
