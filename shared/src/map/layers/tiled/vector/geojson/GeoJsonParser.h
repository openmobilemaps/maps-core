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
#include "geojsonvt.hpp"
#include "simplify.hpp"

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
    static std::shared_ptr<GeoJSONVT> parse(const nlohmann::json &geojson) {
        // preconditions
        if (!geojson["type"].is_string() ||
            geojson["type"] != "FeatureCollection" ||
            !geojson["features"].is_array()) {
            LogError <<= "Geojson is not valid";
            assert(false);
            return nullptr;
        }

        UUIDGenerator generator;

        std::shared_ptr<GeoJson> geoJson = std::make_shared<GeoJson>();
        for (const auto feature: geojson["features"]) {
            if (!feature["geometry"].is_object() ||
                !feature["geometry"]["type"].is_string() ||
                !feature["geometry"]["coordinates"].is_array()) {
                LogError <<= "Geojson feature is not valid";
                return;
            }
            const auto geometryType = feature["geometry"]["type"];
            const auto coordinates = feature["geometry"]["coordinates"];
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
            } else if(geometryType == "MultiLineString") {
                geometry = parseMultiLineString(coordinates);
                geomType = vtzero::GeomType::LINESTRING;
            } else if(geometryType == "Polygon") {
                geometry = parsePolygon(coordinates);
                geomType = vtzero::GeomType::POLYGON;
            } else if(geometryType == "MultiPolygon") {
                geometry = parseMultiPolygon(coordinates);
                geomType = vtzero::GeomType::POLYGON;
            }

            if (!geometry) {
                continue;
            }

            const auto properties = parseProperties(feature["properties"]);

            geometry->featureContext = std::make_shared<FeatureContext>(geomType, properties, generator.generateUUID());

            geoJson->geometries.push_back(geometry);
        }

        return std::make_shared<GeoJSONVT>(geoJson);
    }

    static FeatureContext::mapType parseProperties(const nlohmann::json &properties) {
        FeatureContext::mapType propertyMap;
        if (properties.is_object()) {
            for (auto&[key, val]: properties.items()) {
                if (val.is_string()){
                    propertyMap.push_back(std::make_pair(std::string(key), FeatureContext::valueType(val.get<std::string>())));
                } else if (val.is_number_integer()){
                    propertyMap.push_back(std::make_pair(std::string(key), FeatureContext::valueType(val.get<int64_t>())));
                } else if (val.is_number_float()){
                    propertyMap.push_back(std::make_pair(std::string(key), FeatureContext::valueType(val.get<float>())));
                } else if (val.is_boolean()){
                    propertyMap.push_back(std::make_pair(std::string(key), FeatureContext::valueType(val.get<bool>())));
                } else if (val.is_array() && !val.empty() && val[0].is_number()){
                    std::vector<float> valueArray;
                    valueArray.reserve(val.size());
                    for(const auto value: val) {
                        valueArray.push_back(value.get<float>());
                    }
                    propertyMap.push_back(std::make_pair(std::string(key), FeatureContext::valueType(valueArray)));
                } else if (val.is_array() && !val.empty() && val[0].is_string()){
                    std::vector<std::string> valueArray;
                    valueArray.reserve(val.size());
                    for(const auto value: val) {
                        valueArray.push_back(value.get<std::string>());
                    }
                    propertyMap.push_back(std::make_pair(std::string(key), FeatureContext::valueType(valueArray)));

                }
            }
        }
        return propertyMap;
    }

    static std::shared_ptr<GeoJsonGeometry> parsePoint(const nlohmann::json &coordinates) {
        auto geometry = std::make_shared<GeoJsonGeometry>();
        geometry->coordinates.emplace_back(std::vector<Coord>{parseCoordinate(coordinates)});
        return geometry;
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
