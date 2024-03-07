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
#include "GeoJsonTypes.h"
#include "json.h"
#include <random>
#include "simplify.hpp"
#include "CoordinateSystemIdentifiers.h"
#include "GeoJsonPoint.h"

class UUIDGenerator {
private:
    std::random_device rd;
    std::mt19937_64 gen;

public:
    UUIDGenerator() : gen(rd()) {}

    uint64_t generateUUID() {
        std::uniform_int_distribution<uint64_t> distribution;
        return distribution(gen);
    }
};


class GeoJsonParser {
public:
    static std::shared_ptr<GeoJson> getGeoJson(const nlohmann::json &geojson) {
        // preconditions
        if (!geojson.contains("type") || (!geojson["type"].is_string() ||
            geojson["type"] != "FeatureCollection" ||
            !geojson["features"].is_array())) {
            LogError <<= "Geojson is not valid";
            return nullptr;
        }

        UUIDGenerator generator;

        std::shared_ptr<GeoJson> geoJson = std::make_shared<GeoJson>();
        for (const auto &feature: geojson["features"]) {
            if (!feature["geometry"].is_object() ||
                !feature["geometry"]["type"].is_string() ||
                !feature["geometry"]["coordinates"].is_array()) {
                LogError <<= "Geojson feature is not valid";
                continue;
            }
            const auto &geometryType = feature["geometry"]["type"];
            const auto &coordinates = feature["geometry"]["coordinates"];
            std::shared_ptr<GeoJsonGeometry> geometry;
            vtzero::GeomType geomType;
            if (geometryType == "Point") {
                geometry = parsePoint(coordinates);
                geomType = vtzero::GeomType::POINT;
            } else if (geometryType == "MultiPoint") {
                geometry = parseMultiPoint(coordinates);
                geomType = vtzero::GeomType::POINT;
            } else if(geometryType == "LineString") {
                geometry = parseLineString(coordinates);
                geomType = vtzero::GeomType::LINESTRING;
                geoJson->hasOnlyPoints = false;
            } else if(geometryType == "MultiLineString") {
                geometry = parseMultiLineString(coordinates);
                geomType = vtzero::GeomType::LINESTRING;
                geoJson->hasOnlyPoints = false;
            } else if(geometryType == "Polygon") {
                geometry = parsePolygon(coordinates);
                geomType = vtzero::GeomType::POLYGON;
                geoJson->hasOnlyPoints = false;
            } else if(geometryType == "MultiPolygon") {
                geometry = parseMultiPolygon(coordinates);
                geomType = vtzero::GeomType::POLYGON;
                geoJson->hasOnlyPoints = false;
            }

            if (!geometry) {
                continue;
            }

            const auto &properties = parseProperties(feature["properties"]);

            if(feature.contains("id") && feature["id"].is_string()) {
                geometry->featureContext = std::make_shared<FeatureContext>(geomType, properties, feature["id"].get<std::string>());
            } else {
                geometry->featureContext = std::make_shared<FeatureContext>(geomType, properties, generator.generateUUID());
            }

            geoJson->geometries.push_back(geometry);
        }
        
        return geoJson;
    }

    static std::vector<::GeoJsonPoint> getPointsWithProperties(const nlohmann::json &geojson) {
        // preconditions
        std::vector<::GeoJsonPoint> points;

        if (!geojson["type"].is_string() ||
            geojson["type"] != "FeatureCollection" ||
            !geojson["features"].is_array()) {
            LogError <<= "Geojson is not valid";
            assert(false);
            return points;
        }

        for (const auto &feature: geojson["features"]) {
            const auto &geometry = feature["geometry"];

            if (!geometry.is_object()) { 
                LogError <<= "Geojson feature is not valid";
                continue;
            }

            const auto &geometryType = geometry["type"];
            if (!geometryType.is_string() || geometryType != "Point") {
                continue;
            }

            if(!feature.contains("id") || !feature["id"].is_string()) {
                continue;
            }

            const auto &coordinates = geometry["coordinates"];

            if(!coordinates.is_array()) {
                continue;
            }

            points.emplace_back(getPoint(coordinates), GeoJsonParser::getFeatureInfo(feature["properties"], feature["id"].get<std::string>()));
        }

        return points;
    }

private:
    static FeatureContext::mapType parseProperties(const nlohmann::json &properties) {
        FeatureContext::mapType propertyMap;
        if (properties.is_object()) {
            for (const auto &[key, val] : properties.items()) {
                if (val.is_string()){
                    propertyMap.emplace_back(key, val.get<std::string>());
                } else if (val.is_number_integer()){
                    propertyMap.emplace_back(key, val.get<int64_t>());
                } else if (val.is_number_float()){
                    propertyMap.emplace_back(key, val.get<float>());
                } else if (val.is_boolean()){
                    propertyMap.emplace_back(key, val.get<bool>());
                } else if (val.is_array() && !val.empty() && val[0].is_number()){
                    std::vector<float> valueArray;
                    valueArray.reserve(val.size());
                    for(const auto &value: val) {
                        valueArray.emplace_back(value.get<float>());
                    }
                    propertyMap.emplace_back(key, FeatureContext::valueType(valueArray));
                } else if (val.is_array() && !val.empty() && val[0].is_string()){
                    std::vector<std::string> valueArray;
                    valueArray.reserve(val.size());
                    for(const auto &value: val) {
                        valueArray.push_back(value.get<std::string>());
                    }
                    propertyMap.emplace_back(key, valueArray);
                }
            }
        }

        return propertyMap;
    }

    static VectorLayerFeatureInfo getFeatureInfo(const nlohmann::json &properties, const std::string& identifier) {

        auto info = VectorLayerFeatureInfo(identifier, {});
        
        if (properties.is_object()) {
            for (const auto &[key, val] : properties.items()) {
                if (val.is_string()){
                    info.properties.insert({ key, VectorLayerFeatureInfoValue(val.get<std::string>(), std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt) });
                } else if (val.is_number_integer()) {
                    info.properties.insert({ key, VectorLayerFeatureInfoValue(std::nullopt, std::nullopt, val.get<int64_t>(), std::nullopt, std::nullopt, std::nullopt, std::nullopt) });
                } else if (val.is_number_float()){
                    info.properties.insert({ key, VectorLayerFeatureInfoValue(std::nullopt, val.get<float>(), std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt) });
                } else if (val.is_boolean()){
                    info.properties.insert({ key, VectorLayerFeatureInfoValue(std::nullopt, std::nullopt, std::nullopt, val.get<bool>(), std::nullopt, std::nullopt, std::nullopt) });
                } else if (val.is_array() && !val.empty() && val[0].is_number()){
                    std::vector<float> valueArray;
                    valueArray.reserve(val.size());
                    for(const auto &value: val) {
                        valueArray.emplace_back(value.get<float>());
                    }

                    info.properties.insert({ key, VectorLayerFeatureInfoValue(std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, valueArray, std::nullopt) });
                } else if (val.is_array() && !val.empty() && val[0].is_string()){
                    std::vector<std::string> valueArray;
                    valueArray.reserve(val.size());
                    for(const auto &value: val) {
                        valueArray.push_back(value.get<std::string>());
                    }

                    info.properties.insert({ key, VectorLayerFeatureInfoValue(std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, valueArray) });
                }
            }
        }

        return info;
    }

    static std::shared_ptr<GeoJsonGeometry> parsePoint(const nlohmann::json &coordinates) {
        auto geometry = std::make_shared<GeoJsonGeometry>();
        geometry->coordinates.emplace_back(std::vector<Coord>{parseCoordinate(coordinates)});
        return geometry;
    }

    static Coord getPoint(const nlohmann::json &coordinates) {
        return parseCoordinate(coordinates);
    }

    static std::shared_ptr<GeoJsonGeometry> parseMultiPoint(const nlohmann::json &coordinates) {
        auto geometry = std::make_shared<GeoJsonGeometry>();
        for (const auto &coord : coordinates) {
            geometry->coordinates.emplace_back(std::vector<Coord>{parseCoordinate(coord)});
        }
        return geometry;
    }

    static std::shared_ptr<GeoJsonGeometry> parseLineString(const nlohmann::json &coordinates) {
        auto geometry = std::make_shared<GeoJsonGeometry>();
        std::vector<Coord> lineCoords;
        for (const auto &coord : coordinates) {
            lineCoords.emplace_back(parseCoordinate(coord));
        }
        geometry->coordinates.emplace_back(lineCoords);
        return geometry;
    }

    static std::shared_ptr<GeoJsonGeometry> parseMultiLineString(const nlohmann::json &coordinates) {
        auto geometry = std::make_shared<GeoJsonGeometry>();
        for (const auto &line : coordinates) {
            std::vector<Coord> lineCoords;
            for (const auto &coord : line) {
                lineCoords.emplace_back(parseCoordinate(coord));
            }
            geometry->coordinates.emplace_back(lineCoords);
        }
        return geometry;
    }

    static std::shared_ptr<GeoJsonGeometry> parsePolygon(const nlohmann::json &coordinates) {
        auto geometry = std::make_shared<GeoJsonGeometry>();
        std::vector<Coord> outside;
        std::vector<std::vector<Coord>> holes;

        for (const auto &ring : coordinates) {
            if (ring.is_array()) {
                std::vector<Coord> parsedRing;
                for (const auto &coord : ring) {
                    parsedRing.emplace_back(parseCoordinate(coord));
                }
                if (outside.empty()) {
                    outside = parsedRing;
                } else {
                    holes.emplace_back(parsedRing);
                }
            }
        }

        geometry->coordinates.emplace_back(outside);
        geometry->holes.emplace_back(holes);
        return geometry;
    }

    static std::shared_ptr<GeoJsonGeometry> parseMultiPolygon(const nlohmann::json &coordinates) {
        auto geometry = std::make_shared<GeoJsonGeometry>();

        for (const auto &polygons : coordinates) {
            std::vector<Coord> outside;
            std::vector<std::vector<Coord>> holes;

            for (const auto &ring : polygons) {
                if (ring.is_array()) {
                    std::vector<Coord> parsedRing;
                    for (const auto &coord : ring) {
                        parsedRing.emplace_back(parseCoordinate(coord));
                    }
                    if (outside.empty()) {
                        outside = parsedRing;
                    } else {
                        holes.emplace_back(parsedRing);
                    }
                }
            }

            geometry->coordinates.emplace_back(outside);
            geometry->holes.emplace_back(holes);
        }

        return geometry;
    }

    static Coord parseCoordinate(const nlohmann::json &json) {
        return Coord(CoordinateSystemIdentifiers::EPSG4326(), json[0].get<double>(), json[1].get<double>(), 0.0f);
    }
};
