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
#include <random>
#include "geojsonvt.hpp"
#include "json.h"
#include "GeoJsonTypes.h"

class GeoJsonVTFactory {
public:
    static std::shared_ptr<GeoJSONVTInterface> getGeoJsonVt(const std::shared_ptr<GeoJson> &geoJson) {
        return std::static_pointer_cast<GeoJSONVTInterface>(std::make_shared<GeoJSONVT>(geoJson));
    }

    static std::shared_ptr<GeoJSONVTInterface> getGeoJsonVt(const std::string &geoJsonUrl, const std::vector<std::shared_ptr<::LoaderInterface>> &loaders) {
        std::shared_ptr<GeoJSONVT> vt = std::make_shared<GeoJSONVT>(geoJsonUrl, loaders);
        vt->load();
        return vt;
    }
};
