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

#include "Tiled2dMapVectorLayerConfig.h"
#include "Tiled2dMapZoomLevelInfo.h"

/** Base class for Tiled2dMapVectorLayerConfig implementations for _irregular_
 * tile pyramids, i.e. for tile systems that do not follow a simple power-of-2
 * relation between zoom levels, but are defined with a table of fixed zoom
 * levels.
 */
class IrregularTiled2dMapLayerConfig : public Tiled2dMapVectorLayerConfig {
public:
    IrregularTiled2dMapLayerConfig(
        std::string layerName,
        std::string urlFormat,
        const std::optional<RectCoord> &bounds,
        const Tiled2dMapZoomInfo &zoomInfo,
        const std::vector<int> &levels,
        const std::vector<Tiled2dMapZoomLevelInfo> &zoomLevelInfos
    )
      : Tiled2dMapVectorLayerConfig(layerName, urlFormat, bounds, zoomInfo, levels)
      , zoomLevelInfos(zoomLevelInfos)
    {}

    virtual int32_t getCoordinateSystemIdentifier() override { 
        return zoomLevelInfos.front().bounds.topLeft.systemIdentifier;
    }
    
    virtual std::vector<Tiled2dMapZoomLevelInfo> getZoomLevelInfos() override;
    virtual std::vector<Tiled2dMapZoomLevelInfo> getVirtualZoomLevelInfos() override;

    virtual double getZoomIdentifier(double zoom) override;
    virtual double getZoomFactorAtIdentifier(double zoomIdentifier) override;

protected:
    std::vector<Tiled2dMapZoomLevelInfo> zoomLevelInfos;
};
