/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "GeoJsonParser.h"
#include "CoordinateSystemIdentifiers.h"
#include "GeoJsonLine.h"

#include <random>

class UUIDGenerator {
  private:
    std::random_device rd;
    std::mt19937_64 gen;

  public:
    UUIDGenerator()
        : gen(rd()) {}

    uint64_t generateUUID() {
        std::uniform_int_distribution<uint64_t> distribution;
        return distribution(gen);
    }
};

// Check if a JSON value is an array. This is useful to check explicitly before
// expecting to iterate over an array; iteration will "work" for other types
// too, but might allow too far off-spec data structures (e.g. ["foo", "bar"] vs. {"_whatever_": "foo", "_something_": "bar"})
static void checkIsArray(const nlohmann::json &val, const char *name) {
    if (!val.is_array()) {
        throw nlohmann::json::type_error::create(399, std::string("\"") + name + "\" must be array, but is " + val.type_name(),
                                                 &val);
    }
}

static Coord parseCoordinate(const nlohmann::json &json) {
    return Coord(CoordinateSystemIdentifiers::EPSG4326(), json.at(0).get<double>(), json.at(1).get<double>(), 0.0f);
}

static std::shared_ptr<GeoJsonGeometry> parsePoint(const nlohmann::json &coordinates) {
    auto geometry = std::make_shared<GeoJsonGeometry>();
    geometry->coordinates.emplace_back(std::vector<Coord>{parseCoordinate(coordinates)});
    return geometry;
}

static Coord getPoint(const nlohmann::json &coordinates) { return parseCoordinate(coordinates); }

static std::shared_ptr<GeoJsonGeometry> parseMultiPoint(const nlohmann::json &coordinates) {
    auto geometry = std::make_shared<GeoJsonGeometry>();
    checkIsArray(coordinates, "coordinates");
    for (const auto &coord : coordinates) {
        geometry->coordinates.emplace_back(std::vector<Coord>{parseCoordinate(coord)});
    }
    return geometry;
}

static std::shared_ptr<GeoJsonGeometry> parseLineString(const nlohmann::json &coordinates) {
    auto geometry = std::make_shared<GeoJsonGeometry>();
    std::vector<Coord> lineCoords;
    checkIsArray(coordinates, "coordinates");
    for (const auto &coord : coordinates) {
        lineCoords.emplace_back(parseCoordinate(coord));
    }
    geometry->coordinates.emplace_back(lineCoords);
    return geometry;
}

static std::shared_ptr<GeoJsonGeometry> parseMultiLineString(const nlohmann::json &coordinates) {
    auto geometry = std::make_shared<GeoJsonGeometry>();
    checkIsArray(coordinates, "coordinates");
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

    checkIsArray(coordinates, "coordinates");
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

    checkIsArray(coordinates, "coordinates");
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

static VectorLayerFeatureInfo getFeatureInfo(const nlohmann::json &properties, const std::string &identifier) {

    auto info = VectorLayerFeatureInfo(identifier, {});

    if (properties.is_object()) {
        for (const auto &[key, val] : properties.items()) {
            if (val.is_string()) {
                info.properties.insert({key, VectorLayerFeatureInfoValue(val.get<std::string>(), std::nullopt, std::nullopt,
                                                                         std::nullopt, std::nullopt, std::nullopt, std::nullopt)});
            } else if (val.is_number_integer()) {
                info.properties.insert({key, VectorLayerFeatureInfoValue(std::nullopt, std::nullopt, val.get<int64_t>(),
                                                                         std::nullopt, std::nullopt, std::nullopt, std::nullopt)});
            } else if (val.is_number_float()) {
                info.properties.insert({key, VectorLayerFeatureInfoValue(std::nullopt, val.get<float>(), std::nullopt, std::nullopt,
                                                                         std::nullopt, std::nullopt, std::nullopt)});
            } else if (val.is_boolean()) {
                info.properties.insert({key, VectorLayerFeatureInfoValue(std::nullopt, std::nullopt, std::nullopt, val.get<bool>(),
                                                                         std::nullopt, std::nullopt, std::nullopt)});
            } else if (val.is_array() && !val.empty() && val[0].is_number()) {
                std::vector<float> valueArray;
                valueArray.reserve(val.size());
                for (const auto &value : val) {
                    valueArray.emplace_back(value.get<float>());
                }

                info.properties.insert({key, VectorLayerFeatureInfoValue(std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                                         std::nullopt, valueArray, std::nullopt)});
            } else if (val.is_array() && !val.empty() && val[0].is_string()) {
                std::vector<std::string> valueArray;
                valueArray.reserve(val.size());
                for (const auto &value : val) {
                    valueArray.push_back(value.get<std::string>());
                }

                info.properties.insert({key, VectorLayerFeatureInfoValue(std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                                         std::nullopt, std::nullopt, valueArray)});
            }
        }
    }

    return info;
}

static FeatureContext::mapType parseProperties(const nlohmann::json &properties, StringInterner &stringTable) {
    FeatureContext::mapType propertyMap;
    if (properties.is_object()) {
        for (const auto &[keyStr, val] : properties.items()) {
            InternedString key = stringTable.add(keyStr);
            if (val.is_string()) {
                propertyMap.emplace_back(key, val.get<std::string>());
            } else if (val.is_number_integer()) {
                propertyMap.emplace_back(key, val.get<int64_t>());
            } else if (val.is_number_float()) {
                propertyMap.emplace_back(key, val.get<float>());
            } else if (val.is_boolean()) {
                propertyMap.emplace_back(key, val.get<bool>());
            } else if (val.is_array() && !val.empty() && val[0].is_number()) {
                std::vector<float> valueArray;
                valueArray.reserve(val.size());
                for (const auto &value : val) {
                    valueArray.emplace_back(value.get<float>());
                }
                propertyMap.emplace_back(key, FeatureContext::valueType(valueArray));
            } else if (val.is_array() && !val.empty() && val[0].is_string()) {
                std::vector<std::string> valueArray;
                valueArray.reserve(val.size());
                for (const auto &value : val) {
                    valueArray.push_back(value.get<std::string>());
                }
                propertyMap.emplace_back(key, valueArray);
            }
        }
    }

    return propertyMap;
}

/** Decode `geometry` as a Geometry Object.
 * @throws
 */
static std::tuple<std::shared_ptr<GeoJsonGeometry>, vtzero::GeomType> parseGeometry(const nlohmann::json &geometryJson) {

    const auto &geometryTypeNode = geometryJson.at("type");
    const auto &geometryType = geometryTypeNode.get<std::string>();
    const auto &coordinates = geometryJson.at("coordinates");
    std::shared_ptr<GeoJsonGeometry> geometry;
    vtzero::GeomType geomType;
    if (geometryType == "Point") {
        geometry = parsePoint(coordinates);
        geomType = vtzero::GeomType::POINT;
    } else if (geometryType == "MultiPoint") {
        geometry = parseMultiPoint(coordinates);
        geomType = vtzero::GeomType::POINT;
    } else if (geometryType == "LineString") {
        geometry = parseLineString(coordinates);
        geomType = vtzero::GeomType::LINESTRING;
    } else if (geometryType == "MultiLineString") {
        geometry = parseMultiLineString(coordinates);
        geomType = vtzero::GeomType::LINESTRING;
    } else if (geometryType == "Polygon") {
        geometry = parsePolygon(coordinates);
        geomType = vtzero::GeomType::POLYGON;
    } else if (geometryType == "MultiPolygon") {
        geometry = parseMultiPolygon(coordinates);
        geomType = vtzero::GeomType::POLYGON;
    } else if (geometryType == "GeometryCollection") {
        // Note: GeometryCollection explicitly not supported.
        // Could be returned as individual geometries and then clone the properties of the associated Feature.
        throw nlohmann::json::other_error::create(599, "unsupported geometry type \"GeometryCollection\"", &geometryTypeNode);
    } else {
        throw nlohmann::json::other_error::create(599, std::string("unknown geometry type \"") + geometryType + "\"",
                                                  &geometryTypeNode);
    }
    return std::make_tuple(std::move(geometry), geomType);
}

/** Decode `feature` as a Feature Object.
 * @throws
 */
static std::shared_ptr<GeoJsonGeometry> parseFeature(const nlohmann::json &feature, UUIDGenerator &uuidGenerator, StringInterner &stringTable) {

    auto [geometry, geomType] = parseGeometry(feature.at("geometry"));

    const auto &properties = feature.contains("properties") ? parseProperties(feature["properties"], stringTable) : FeatureContext::mapType();

    if (feature.contains("id") && feature["id"].is_string()) {
        geometry->featureContext = std::make_shared<FeatureContext>(geomType, properties, feature["id"].get<std::string>());
    } else {
        geometry->featureContext = std::make_shared<FeatureContext>(geomType, properties, uuidGenerator.generateUUID());
    }
    return geometry;
}

std::shared_ptr<GeoJson> GeoJsonParser::getGeoJson(const nlohmann::json &geojson, StringInterner &stringTable) {
    UUIDGenerator generator;

    std::shared_ptr<GeoJson> geoJson = std::make_shared<GeoJson>();
    const auto &type = geojson.at("type").get<std::string>();
    if (type == "Feature") {
        auto geometry = parseFeature(geojson, generator, stringTable);
        geoJson->geometries.emplace_back(std::move(geometry));
    } else if (type == "FeatureCollection") {
        const auto &features = geojson.at("features");
        checkIsArray(features, "features");
        for (const auto &feature : features) {
            // Keep going even if there are malformed features.
            try {
                // Note: from context this must be a "Feature", so if "type" is missing we can be a bit lenient.
                if (feature.contains("type")) {
                    auto type = feature.at("type").get<std::string>();
                    if (type != "Feature") {
                        throw nlohmann::json::other_error::create(
                            599, std::string("\"type\" must be \"Feature\" but is \"") + type + "\"", &feature.at("type"));
                    }
                }
                auto geometry = parseFeature(feature, generator, stringTable);
                geoJson->geometries.emplace_back(std::move(geometry));
            } catch (nlohmann::json::exception &ex) {
                LogError << "Geojson feature is not valid: " <<= ex.what();
            }
        }
    } else {
        // Attempt to parse as a Geometry Object (without properties)
        auto [geometry, geomType] = parseGeometry(geojson);
        geometry->featureContext = std::make_shared<FeatureContext>(geomType, FeatureContext::mapType(), generator.generateUUID());
        geoJson->geometries.emplace_back(std::move(geometry));
    }
    geoJson->hasOnlyPoints = std::all_of(geoJson->geometries.begin(), geoJson->geometries.end(),
                                         [](auto &geo) { return geo->featureContext->geomType == vtzero::GeomType::POINT; });

    return geoJson;
}

std::vector<::GeoJsonPoint> GeoJsonParser::getPointsWithProperties(const nlohmann::json &geojson) {
    std::vector<::GeoJsonPoint> points;
    if (geojson.at("type") != "FeatureCollection") {
        return points;
    }
    const nlohmann::json j_null;
    const auto &features = geojson.at("features");
    checkIsArray(features, "features");
    for (const auto &feature : features) {
        try {
            const auto &geometry = feature.at("geometry");
            if (geometry.at("type") != "Point") {
                continue;
            }
            if (!feature.contains("id") || !feature["id"].is_string()) {
                continue;
            }
            const auto &coordinates = geometry.at("coordinates");
            const auto &properties = feature.contains("properties") ? feature["properties"] : j_null;
            points.emplace_back(getPoint(coordinates), getFeatureInfo(properties, feature["id"].get<std::string>()));
        } catch (nlohmann::json::exception &ex) {
            LogError << "Geojson feature is not valid:" <<= ex.what();
        }
    }

    return points;
}

std::vector<::GeoJsonLine> GeoJsonParser::getLinesWithProperties(const nlohmann::json &geojson) {
    std::vector<::GeoJsonLine> lines;
    if (geojson.at("type") != "FeatureCollection") {
        return lines;
    }
    const nlohmann::json j_null;
    const auto &features = geojson.at("features");
    checkIsArray(features, "features");
    for (const auto &feature : features) {
        try {
            const auto &geometry = feature.at("geometry");
            if (geometry.at("type") != "LineString") {
                continue;
            }
            if (!feature.contains("id") || !feature["id"].is_string()) {
                continue;
            }
            std::vector<Coord> lineCoords;
            for (const auto &coord : geometry.at("coordinates")) {
                lineCoords.emplace_back(parseCoordinate(coord));
            }
            const auto &properties = feature.contains("properties") ? feature["properties"] : j_null;
            lines.emplace_back(lineCoords, getFeatureInfo(properties, feature["id"].get<std::string>()));
        } catch (nlohmann::json::exception &ex) {
            LogError << "Geojson feature is not valid:" <<= ex.what();
        }
    }

    return lines;
}
