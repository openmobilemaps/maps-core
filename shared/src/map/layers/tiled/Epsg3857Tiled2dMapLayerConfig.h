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

class Epsg3857Tiled2dMapLayerConfig : public RegularTiled2dMapLayerConfig {
public:
    Epsg3857Tiled2dMapLayerConfig(
        std::string layerName,
        std::string urlFormat
    );

    Epsg3857Tiled2dMapLayerConfig(
        std::string layerName,
        std::string urlFormat,
        const std::optional<RectCoord> &bounds,
        const Tiled2dMapZoomInfo &zoomInfo,
        const std::vector<int> &levels
    );

    virtual std::string getTileUrl(int32_t x, int32_t y, int32_t t, int32_t zoom) override;

private:
    const static double BASE_ZOOM;
};
