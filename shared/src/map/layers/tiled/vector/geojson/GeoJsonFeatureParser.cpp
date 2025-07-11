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
    StringInterner stringTable{};
    try {
        const auto json = nlohmann::json::parse(geoJson);
        auto geoJsonObject = GeoJsonParser::getGeoJson(json, stringTable);
        std::vector<::VectorLayerFeatureInfo> features = {};
        for (auto &geometry: geoJsonObject->geometries) {
            features.push_back(geometry->featureContext->getFeatureInfo(stringTable));
        }
        return features;
    }
    catch (nlohmann::json::exception &ex) {
        LogError << "Geojson is not valid:" <<= ex.what();
        return std::nullopt;
    }
}

std::optional<std::vector<GeoJsonPoint>> GeoJsonFeatureParser::parseWithPointGeometry(const std::string & geoJson) {
    try {
        const auto& json = nlohmann::json::parse(geoJson);
        return GeoJsonParser::getPointsWithProperties(json);
    }
    catch (nlohmann::json::exception &ex) {
        LogError << "Geojson is not valid:" <<= ex.what();
        return std::nullopt;
    }
}


std::optional<std::vector<GeoJsonLine>> GeoJsonFeatureParser::parseWithLineGeometry(const std::string & geoJson) {
    try {
        const auto& json = nlohmann::json::parse(geoJson);
        return GeoJsonParser::getLinesWithProperties(json);
    }
    catch (nlohmann::json::exception &ex) {
        LogError << "Geojson is not valid:" <<= ex.what();
        return std::nullopt;
    }
}
