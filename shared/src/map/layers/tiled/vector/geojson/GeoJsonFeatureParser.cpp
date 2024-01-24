/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "GeoJsonFeatureParser.h"
#include "GeoJsonFeatureParserInterface.h"
#include "GeoJsonParser.h"
#include "GeoJsonPoint.h"

GeoJsonFeatureParser::GeoJsonFeatureParser() {}

std::optional<std::vector<::VectorLayerFeatureInfo>> GeoJsonFeatureParser::parse(const std::string & geoJson) {
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(geoJson);
        auto geoJsonObject = GeoJsonParser::getGeoJson(json);
        std::vector<::VectorLayerFeatureInfo> features = {};
        for (auto &geometry: geoJsonObject->geometries) {
            features.push_back(geometry->featureContext->getFeatureInfo());
        }
        return features;
    }
    catch (nlohmann::json::parse_error &ex) {
        return std::nullopt;
    }
}

std::optional<std::vector<GeoJsonPoint>> GeoJsonFeatureParser::parseWithPointGeometry(const std::string & geoJson) {
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(geoJson);
        auto geoJsonObject = GeoJsonParser::getGeoJson(json);
        std::vector<::GeoJsonPoint> points;
        for (auto &geometry: geoJsonObject->geometries) {
            if(geometry->coordinates.size() == 1 && geometry->coordinates.front().size() == 1) {
                points.emplace_back(geometry->coordinates.front().front(), geometry->featureContext->getFeatureInfo());
            }
        }
        return points;
    }
    catch (nlohmann::json::parse_error &ex) {
        return std::nullopt;
    }
}
