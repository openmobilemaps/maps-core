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

#include "Tiled2dMapZoomInfo.h"
#include "Tiled2dMapZoomLevelInfo.h"

class RegionLayerConfig : public Tiled2dMapLayerConfig {
  public:
    RegionLayerConfig(const std::string &urlTemplate, const std::vector<Tiled2dMapZoomLevelInfo> &zoomLevelInfos, const std::string &coordinateSystemIdentifier)
        : urlTemplate(urlTemplate),
          zoomLevelInfos(zoomLevelInfos),
          coordinateSystemIdentifier(coordinateSystemIdentifier) {}

    ~RegionLayerConfig() {}

    std::string getCoordinateSystemIdentifier() override { return coordinateSystemIdentifier; }

    std::string getTileUrl(int32_t x, int32_t y, int32_t t, int32_t zoom) override {
        std::string url = urlTemplate;
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

    std::vector<Tiled2dMapZoomLevelInfo> getZoomLevelInfos() override { return zoomLevelInfos;}

    Tiled2dMapZoomInfo getZoomInfo() override { return defaultZoomInfo; }

    std::string getLayerName() override { return urlTemplate; }

  private:
    const std::string urlTemplate;

    const std::string coordinateSystemIdentifier;

    const Tiled2dMapZoomInfo defaultZoomInfo = Tiled2dMapZoomInfo(0.65, 0, true, true);
    const std::vector<Tiled2dMapZoomLevelInfo> zoomLevelInfos;
};
