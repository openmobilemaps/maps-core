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
#include "Tiled2dMapVectorLayerConfig.h"

class Tiled2dMapVectorGeoJSONLayerConfig : public Tiled2dMapVectorLayerConfig {
public:
    Tiled2dMapVectorGeoJSONLayerConfig(const std::string &sourceName, const std::weak_ptr<GeoJSONVTInterface> geoJSON, const Tiled2dMapZoomInfo &zoomInfo = Tiled2dMapZoomInfo(1.0, 0, 0, false, true, false, true))
            : Tiled2dMapVectorLayerConfig(nullptr, zoomInfo), geoJSON(geoJSON), sourceName(sourceName) {}

    ~Tiled2dMapVectorGeoJSONLayerConfig() {}

    std::string getTileUrl(int32_t x, int32_t y, int32_t t, int32_t zoom) override {
        static std::string empty = "";
        return empty;
    }

    std::vector<Tiled2dMapZoomLevelInfo> getZoomLevelInfos() override {
        int maxZoom = 0;
        if (auto geoJSON = this->geoJSON.lock()){
            maxZoom = geoJSON->getMaxZoom();
        }
        return getDefaultEpsg3857ZoomLevels(0, maxZoom);
    }

    std::string getLayerName() override {
        return sourceName;
    }

protected:
    std::weak_ptr<GeoJSONVTInterface> geoJSON;
    const std::string sourceName;
};
