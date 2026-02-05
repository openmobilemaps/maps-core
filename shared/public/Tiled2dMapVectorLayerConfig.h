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

#include "RectCoord.h"
#include "Tiled2dMapLayerConfig.h"
#include "Tiled2dMapVectorSettings.h"
#include "Tiled2dMapZoomInfo.h"
#include <optional>
#include <vector>
#include <string>

/** 
* Abstract base class for different layer configurations.
*/
class Tiled2dMapVectorLayerConfig : public Tiled2dMapLayerConfig {
public:
    virtual double getZoomIdentifier(double zoom) = 0;
    virtual double getZoomFactorAtIdentifier(double zoomIdentifier) = 0;

public:
    Tiled2dMapVectorLayerConfig(
        std::string layerName,
        std::string urlFormat,
        const std::optional<RectCoord> &bounds,
        const Tiled2dMapZoomInfo &zoomInfo,
        const std::vector<int> &levels
    );

    virtual ~Tiled2dMapVectorLayerConfig() = default;

public:
    virtual std::string getLayerName() override {
        return layerName;
    }

		virtual std::string getTileUrl(int32_t x, int32_t y, int32_t t, int32_t zoom) override;

    virtual Tiled2dMapZoomInfo getZoomInfo() override {
        return zoomInfo;
    }

    std::optional<::RectCoord> getBounds() override {
        return bounds;
    }

    virtual std::optional<Tiled2dMapVectorSettings> getVectorSettings() override {
        return std::nullopt;
    }

public:
    // Helper to initialize `zoomInfo`
    static Tiled2dMapZoomInfo defaultMapZoomInfo();

    // Helper to initalize `levels` from min/max zoom levels value
    static std::vector<int> generateLevelsFromMinMax(int minZoomLevel, int maxZoomLevel);

protected:
    std::string layerName;
    std::string urlFormat;
    Tiled2dMapZoomInfo zoomInfo;
    std::optional<RectCoord> bounds;
    std::vector<int> levels; // zoom level indices (kept sorted ascending)
};
