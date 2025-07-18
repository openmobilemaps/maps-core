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
#include "GeoJsonLine.h"
#include "GeoJsonPoint.h"
#include "GeoJsonTypes.h"
#include "json.h"

class GeoJsonParser {
  public:
    /**
     * Decode GeoJSON object
     * @throws json::exception on malformed data
     * @returns not-null
     */
    static std::shared_ptr<GeoJson> getGeoJson(const nlohmann::json &geojson, StringInterner &stringTable);
    /**
     * Decode GeoJSON object, looking only for Points in a FeatureCollection
     * @throws json::exception on malformed data
     */
    static std::vector<::GeoJsonPoint> getPointsWithProperties(const nlohmann::json &geojson);
    /**
     * Decode GeoJSON object, looking only at LineStrings in a FeatureCollection
     * @throws json::exception on malformed data
     */
    static std::vector<::GeoJsonLine> getLinesWithProperties(const nlohmann::json &geojson);
};
