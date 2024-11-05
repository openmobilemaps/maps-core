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
#include "Tiled2dMapZoomInfo.h"
#include "Tiled2dMapZoomLevelInfo.h"
#include "WmtsLayerDescription.h"

class WmtsTiled2dMapLayerConfig : public Tiled2dMapLayerConfig {
  public:
    WmtsTiled2dMapLayerConfig(const WmtsLayerDescription &description, const std::vector<Tiled2dMapZoomLevelInfo> &zoomLevelInfo,
                              const Tiled2dMapZoomInfo &zoomInfo, const int32_t coordinateSystemIdentifier,
                              const std::string &matrixSetIdentifier);

    virtual int32_t getCoordinateSystemIdentifier() override;

    virtual std::string getTileUrl(int32_t x, int32_t y, int32_t t, int32_t zoom) override;

    virtual std::string getLayerName() override;

    virtual std::vector<Tiled2dMapZoomLevelInfo> getZoomLevelInfos() override;

    virtual std::vector<Tiled2dMapZoomLevelInfo> getVirtualZoomLevelInfos() override { return {}; };

    virtual Tiled2dMapZoomInfo getZoomInfo() override;

    virtual std::optional<Tiled2dMapVectorSettings> getVectorSettings() override;

    std::optional<::RectCoord> getBounds() override;

private:
    const WmtsLayerDescription description;
    std::vector<Tiled2dMapZoomLevelInfo> zoomLevelInfo;
    const Tiled2dMapZoomInfo zoomInfo;
    const int32_t coordinateSystemIdentifier;
    const std::string matrixSetIdentifier;
};
