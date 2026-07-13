/*
 * Copyright (c) 2026 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "RegularTiled2dMapLayerConfig.h"
#include "Tiled2dMapZoomInfo.h"

class Epsg4326Tiled2dMapLayerConfig : public RegularTiled2dMapLayerConfig {
public:
    Epsg4326Tiled2dMapLayerConfig(
        std::string layerName,
        std::string urlFormat
    );

    Epsg4326Tiled2dMapLayerConfig(
        std::string layerName,
        std::string urlFormat,
        const std::optional<RectCoord> &bounds,
        const Tiled2dMapZoomInfo &zoomInfo,
        const std::vector<int> &levels
    );
private:
    const static double BASE_ZOOM;
};
