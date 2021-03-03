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

#include "Tiled2dMapLayerConfig.h"
#include "WmtsLayerDescription.h"
#include "Tiled2dMapZoomInfo.h"
#include "Tiled2dMapZoomLevelInfo.h"

class WmtsTiled2dMapLayerConfig: public Tiled2dMapLayerConfig {
public:
    WmtsTiled2dMapLayerConfig(const WmtsLayerDescription &description,
                              std::vector<Tiled2dMapZoomLevelInfo> zoomLevelInfo,
                              const Tiled2dMapZoomInfo &zoomInfo,
                              const std::string &coordinateSystemIdentifier);

    virtual std::string getCoordinateSystemIdentifier() override;

    virtual std::string getTileUrl(int32_t x, int32_t y, int32_t zoom) override;

    virtual std::string getTileIdentifier(int32_t x, int32_t y, int32_t zoom) override;

    virtual std::vector<Tiled2dMapZoomLevelInfo> getZoomLevelInfos() override;

    virtual Tiled2dMapZoomInfo getZoomInfo() override;
private:
    const WmtsLayerDescription description;
    std::vector<Tiled2dMapZoomLevelInfo> zoomLevelInfo;
    const Tiled2dMapZoomInfo zoomInfo;
    const std::string coordinateSystemIdentifier;
};
