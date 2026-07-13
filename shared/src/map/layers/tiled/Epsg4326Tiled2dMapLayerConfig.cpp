/*
 * Copyright (c) 2026 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Epsg4326Tiled2dMapLayerConfig.h"
#include "CoordinateSystemFactory.h"

const double Epsg4326Tiled2dMapLayerConfig::BASE_ZOOM = 500000000.0;

Epsg4326Tiled2dMapLayerConfig::Epsg4326Tiled2dMapLayerConfig(
    std::string layerName,
    std::string urlFormat)
  : RegularTiled2dMapLayerConfig(
        layerName, urlFormat, std::nullopt, 
        defaultMapZoomInfo(),
        generateLevelsFromMinMax(0, 20),
        CoordinateSystemFactory::getEpsg4326System(),
        BASE_ZOOM)
{}

Epsg4326Tiled2dMapLayerConfig::Epsg4326Tiled2dMapLayerConfig(
    std::string layerName,
    std::string urlFormat,
    const std::optional<RectCoord> &bounds,
    const Tiled2dMapZoomInfo &zoomInfo,
    const std::vector<int> &levels)
  : RegularTiled2dMapLayerConfig(
        layerName, urlFormat, bounds, zoomInfo, levels, 
        CoordinateSystemFactory::getEpsg4326System(), BASE_ZOOM)
{}
