/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "GeoJsonHelper.h"
#include "VectorLayerFeatureInfo.h"
#include "CoordinateSystemIdentifiers.h"
#include "ValueKeys.h"
#include <sstream>

GeoJsonHelper::GeoJsonHelper() : conversionHelper(CoordinateConversionHelperInterface::independentInstance()) {}

std::string GeoJsonHelper::geoJsonStringFromFeatureInfo(const GeoJsonPoint &point) {
    return geoJsonStringFromFeatureInfos({point});
}

std::string GeoJsonHelper::geoJsonStringFromFeatureInfos(const std::vector<GeoJsonPoint> &points) {
    nlohmann::json geoJson = nlohmann::json();
    geoJson["type"] = "FeatureCollection";
    nlohmann::json features = nlohmann::json::array();

    for (const auto feature : points) {
        features.insert(features.end(), jsonFromFeatureInfo(feature));
    }
    geoJson["features"] = features;

    return geoJson.dump();
}

nlohmann::json GeoJsonHelper::jsonFromFeatureInfo(const GeoJsonPoint &point) {
    nlohmann::json featureJson = nlohmann::json();
    featureJson["type"] = "Feature";
    featureJson["id"] = point.featureInfo.identifier;

    auto epsg4326Coord = conversionHelper->convert(CoordinateSystemIdentifiers::EPSG4326(), point.point);
    featureJson["geometry"] = {{"coordinates", {epsg4326Coord.x, epsg4326Coord.y}}, {"type", "Point"}};

    for (const auto property: point.featureInfo.properties) {
        if (property.first == ValueKeys::TYPE_KEY || property.first == ValueKeys::IDENTIFIER_KEY) {
            continue;
        }

        if (property.second.boolVal.has_value()) {
            featureJson["properties"][property.first] = *property.second.boolVal;
        } else if (auto colorProperty = property.second.colorVal) {
            std::stringstream colorSs;
            colorSs << "rgba(" << (int) std::round(colorProperty->r * 255) << ", "
                    << (int) std::round(colorProperty->g * 255) << ", "
                    << (int) std::round(colorProperty->b * 255) << ", "
                    << colorProperty->a << ")";
            featureJson["properties"][property.first] = colorSs.str();
        } else if (property.second.doubleVal.has_value()) {
            featureJson["properties"][property.first] = *property.second.doubleVal;
        } else if (property.second.intVal.has_value()) {
            featureJson["properties"][property.first] = *property.second.intVal;
        } else if (property.second.listFloatVal.has_value()) {
            featureJson["properties"][property.first] = *property.second.listFloatVal;
        } else if (property.second.listStringVal.has_value()) {
            featureJson["properties"][property.first] = *property.second.listStringVal;
        } else if (property.second.stringVal.has_value()) {
            featureJson["properties"][property.first] = *property.second.stringVal;
        }
    }

    return featureJson;
}
