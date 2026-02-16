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
#include "MapCoordinateSystem.h"

/** Base class for Tiled2dMapVectorLayerConfig implementations for _regular_
 * tile pyramids, i.e. for tile systems with a simple power-of-2 relation
 * between zoom levels.
 */
class RegularTiled2dMapLayerConfig : public Tiled2dMapVectorLayerConfig {
public:
    RegularTiled2dMapLayerConfig(
        std::string layerName,
        std::string urlFormat,
        const std::optional<RectCoord> &bounds,
        const Tiled2dMapZoomInfo &zoomInfo,
        const std::vector<int> &levels,
        const MapCoordinateSystem &coordinateSystem,
        double baseZoom)
      : Tiled2dMapVectorLayerConfig(layerName, urlFormat, bounds, zoomInfo, levels)
      , coordinateSystem(coordinateSystem)
      , baseZoom(baseZoom)
    {}

    virtual int32_t getCoordinateSystemIdentifier() override { 
        return coordinateSystem.identifier;
    }

    virtual std::vector<Tiled2dMapZoomLevelInfo> getZoomLevelInfos() override;

    virtual std::vector<Tiled2dMapZoomLevelInfo> getVirtualZoomLevelInfos() override;
    
    virtual double getZoomIdentifier(double zoom) override;

    virtual double getZoomFactorAtIdentifier(double zoomIdentifier) override;

protected:
    Tiled2dMapZoomLevelInfo getRegularZoomLevelInfo(int zoomLevel);

protected:
    double baseZoom;
    MapCoordinateSystem coordinateSystem;
};
