/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "MapCoordinateSystem.h"
#include "Tiled2dMapLayerConfig.h"
#include "Tiled2dMapZoomInfo.h"
#include "Tiled2dMapZoomLevelInfo.h"

class CustomTiled2dMapLayerConfig : public Tiled2dMapLayerConfig {
public:
    CustomTiled2dMapLayerConfig(std::string layerName, std::string urlFormat, MapCoordinateSystem coordinateSystem);

    CustomTiled2dMapLayerConfig(std::string layerName, std::string urlFormat, MapCoordinateSystem coordinateSystem,
                                const Tiled2dMapZoomInfo &zoomInfo, int32_t minZoomLevel, int32_t maxZoomLevel);

    int32_t getCoordinateSystemIdentifier() override;

    std::string getTileUrl(int32_t x, int32_t y, int32_t t, int32_t zoom) override;

    std::string getLayerName() override;

    std::vector<Tiled2dMapZoomLevelInfo> getZoomLevelInfos() override;

    std::vector<Tiled2dMapZoomLevelInfo> getVirtualZoomLevelInfos() override;

    Tiled2dMapZoomInfo getZoomInfo() override;

    std::optional<Tiled2dMapVectorSettings> getVectorSettings() override;

    std::optional<::RectCoord> getBounds() override;

private:
    Tiled2dMapZoomLevelInfo getZoomLevelInfo(int32_t zoomLevel) const;

    static constexpr double BASE_ZOOM = 500000000.0;

    std::string layerName;
    std::string urlFormat;
    MapCoordinateSystem coordinateSystem;

    int32_t minZoomLevel = 0;
    int32_t maxZoomLevel = 20;
    Tiled2dMapZoomInfo zoomInfo = Tiled2dMapZoomInfo(1.0, 0, 0, true, false, true, true);
};
