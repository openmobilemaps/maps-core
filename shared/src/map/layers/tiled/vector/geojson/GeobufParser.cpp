/*
 * Copyright (c) 2026 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "GeobufParser.h"

#include "GeoJsonParser.h"
#include "json.h"

#include <cmath>
#include <limits>
#include <optional>
#include <protozero/exception.hpp>
#include <protozero/pbf_reader.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct DecoderContext {
    std::vector<std::string> keys;
    std::vector<nlohmann::json> values;
    uint32_t dimensions = 2;
    double precisionScale = 1e6;
};

std::string geometryTypeFromEnum(int32_t type) {
    switch (type) {
    case 0:
        return "Point";
    case 1:
        return "MultiPoint";
    case 2:
        return "LineString";
    case 3:
        return "MultiLineString";
    case 4:
        return "Polygon";
    case 5:
        return "MultiPolygon";
    case 6:
        return "GeometryCollection";
    default:
        return "Point";
    }
}

nlohmann::json readValue(protozero::pbf_reader valueMessage) {
    nlohmann::json value = nullptr;
    while (valueMessage.next()) {
        switch (valueMessage.tag()) {
        case 1:
            value = valueMessage.get_string();
            break;
        case 2:
            value = valueMessage.get_double();
            break;
        case 3: {
            const auto intVal = valueMessage.get_uint64();
            if (intVal <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
                value = static_cast<int64_t>(intVal);
            } else {
                value = static_cast<double>(intVal);
            }
            break;
        }
        case 4: {
            const auto intVal = valueMessage.get_uint64();
            if (intVal <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
                value = -static_cast<int64_t>(intVal);
            } else {
                value = -static_cast<double>(intVal);
            }
            break;
        }
        case 5:
            value = valueMessage.get_bool();
            break;
        case 6: {
            const auto jsonValue = valueMessage.get_string();
            try {
                value = nlohmann::json::parse(jsonValue);
            } catch (const nlohmann::json::exception &) {
                value = jsonValue;
            }
            break;
        }
        default:
            valueMessage.skip();
            break;
        }
    }
    return value;
}

std::vector<uint32_t> readPackedUInt32(protozero::pbf_reader &message) {
    std::vector<uint32_t> values;
    for (const auto value : message.get_packed_uint32()) {
        values.push_back(value);
    }
    return values;
}

std::vector<int64_t> readPackedSInt64(protozero::pbf_reader &message) {
    std::vector<int64_t> values;
    for (const auto value : message.get_packed_sint64()) {
        values.push_back(value);
    }
    return values;
}

nlohmann::json readProperties(const std::vector<uint32_t> &propertyIndexes, DecoderContext &context) {
    nlohmann::json properties = nlohmann::json::object();
    for (size_t i = 0; i + 1 < propertyIndexes.size(); i += 2) {
        const auto keyIndex = propertyIndexes[i];
        const auto valueIndex = propertyIndexes[i + 1];

        if (keyIndex >= context.keys.size() || valueIndex >= context.values.size()) {
            continue;
        }
        properties[context.keys[keyIndex]] = context.values[valueIndex];
    }
    context.values.clear();
    return properties;
}

nlohmann::json decodeLinePart(const std::vector<int64_t> &coords, size_t &coordIndex, const std::optional<uint32_t> &length,
                              uint32_t dimensions, double precisionScale, bool closed) {
    nlohmann::json line = nlohmann::json::array();
    const auto pointDimensions = std::max<uint32_t>(1, dimensions);
    const auto maxPointsFromCoords = (coords.size() - coordIndex) / pointDimensions;
    const auto pointsToRead = length ? std::min<size_t>(*length, maxPointsFromCoords) : maxPointsFromCoords;

    std::vector<int64_t> previous(pointDimensions, 0);
    for (size_t i = 0; i < pointsToRead && coordIndex + pointDimensions <= coords.size(); i++) {
        nlohmann::json point = nlohmann::json::array();
        for (uint32_t d = 0; d < pointDimensions; d++) {
            previous[d] += coords[coordIndex++];
            point.push_back(static_cast<double>(previous[d]) / precisionScale);
        }
        line.push_back(std::move(point));
    }

    if (closed && !line.empty()) {
        line.push_back(line[0]);
    }
    return line;
}

nlohmann::json decodeCoordinates(int32_t type, const std::vector<uint32_t> &lengths, const std::vector<int64_t> &coords,
                                 uint32_t dimensions, double precisionScale) {
    const auto pointDimensions = std::max<uint32_t>(1, dimensions);

    if (type == 0) { // Point
        nlohmann::json point = nlohmann::json::array();
        for (uint32_t d = 0; d < pointDimensions && d < coords.size(); d++) {
            point.push_back(static_cast<double>(coords[d]) / precisionScale);
        }
        return point;
    }

    if (type == 1 || type == 2) { // MultiPoint / LineString
        size_t coordIndex = 0;
        return decodeLinePart(coords, coordIndex, std::nullopt, pointDimensions, precisionScale, false);
    }

    if (type == 3 || type == 4) { // MultiLineString / Polygon
        nlohmann::json lines = nlohmann::json::array();
        size_t coordIndex = 0;
        if (lengths.empty()) {
            lines.push_back(decodeLinePart(coords, coordIndex, std::nullopt, pointDimensions, precisionScale, type == 4));
            return lines;
        }
        for (const auto length : lengths) {
            lines.push_back(decodeLinePart(coords, coordIndex, length, pointDimensions, precisionScale, type == 4));
        }
        return lines;
    }

    if (type == 5) { // MultiPolygon
        size_t coordIndex = 0;
        nlohmann::json polygons = nlohmann::json::array();

        if (lengths.empty()) {
            nlohmann::json polygon = nlohmann::json::array();
            polygon.push_back(decodeLinePart(coords, coordIndex, std::nullopt, pointDimensions, precisionScale, true));
            polygons.push_back(std::move(polygon));
            return polygons;
        }

        size_t lengthIndex = 0;
        const auto polygonCount = lengths[lengthIndex++];
        for (uint32_t polygonIt = 0; polygonIt < polygonCount && lengthIndex < lengths.size(); polygonIt++) {
            const auto ringCount = lengths[lengthIndex++];
            nlohmann::json rings = nlohmann::json::array();
            for (uint32_t ringIt = 0; ringIt < ringCount && lengthIndex < lengths.size(); ringIt++) {
                rings.push_back(decodeLinePart(coords, coordIndex, lengths[lengthIndex++], pointDimensions, precisionScale, true));
            }
            polygons.push_back(std::move(rings));
        }

        return polygons;
    }

    return nlohmann::json::array();
}

nlohmann::json decodeGeometry(protozero::pbf_reader geometryMessage, DecoderContext &context);

nlohmann::json decodeFeature(protozero::pbf_reader featureMessage, DecoderContext &context) {
    nlohmann::json feature = {{"type", "Feature"}};
    bool hasGeometry = false;
    while (featureMessage.next()) {
        switch (featureMessage.tag()) {
        case 1:
            feature["geometry"] = decodeGeometry(featureMessage.get_message(), context);
            hasGeometry = true;
            break;
        case 11:
            feature["id"] = featureMessage.get_string();
            break;
        case 12:
            feature["id"] = featureMessage.get_sint64();
            break;
        case 13:
            context.values.push_back(readValue(featureMessage.get_message()));
            break;
        case 14:
            feature["properties"] = readProperties(readPackedUInt32(featureMessage), context);
            break;
        case 15: {
            auto customProperties = readProperties(readPackedUInt32(featureMessage), context);
            for (auto &[key, value] : customProperties.items()) {
                feature[key] = value;
            }
            break;
        }
        default:
            featureMessage.skip();
            break;
        }
    }

    if (!hasGeometry) {
        feature["geometry"] = nullptr;
    }

    return feature;
}

nlohmann::json decodeFeatureCollection(protozero::pbf_reader featureCollectionMessage, DecoderContext &context) {
    nlohmann::json featureCollection = {
        {"type", "FeatureCollection"},
        {"features", nlohmann::json::array()},
    };

    while (featureCollectionMessage.next()) {
        switch (featureCollectionMessage.tag()) {
        case 1:
            featureCollection["features"].push_back(decodeFeature(featureCollectionMessage.get_message(), context));
            break;
        case 13:
            context.values.push_back(readValue(featureCollectionMessage.get_message()));
            break;
        case 15: {
            auto customProperties = readProperties(readPackedUInt32(featureCollectionMessage), context);
            for (auto &[key, value] : customProperties.items()) {
                featureCollection[key] = value;
            }
            break;
        }
        default:
            featureCollectionMessage.skip();
            break;
        }
    }

    return featureCollection;
}

nlohmann::json decodeGeometry(protozero::pbf_reader geometryMessage, DecoderContext &context) {
    int32_t type = 0;
    std::vector<uint32_t> lengths;
    std::vector<int64_t> coords;
    nlohmann::json geometries = nlohmann::json::array();
    nlohmann::json geometry = nlohmann::json::object();

    while (geometryMessage.next()) {
        switch (geometryMessage.tag()) {
        case 1:
            type = geometryMessage.get_enum();
            break;
        case 2:
            lengths = readPackedUInt32(geometryMessage);
            break;
        case 3:
            coords = readPackedSInt64(geometryMessage);
            break;
        case 4:
            geometries.push_back(decodeGeometry(geometryMessage.get_message(), context));
            break;
        case 13:
            context.values.push_back(readValue(geometryMessage.get_message()));
            break;
        case 15: {
            auto customProperties = readProperties(readPackedUInt32(geometryMessage), context);
            for (auto &[key, value] : customProperties.items()) {
                geometry[key] = value;
            }
            break;
        }
        default:
            geometryMessage.skip();
            break;
        }
    }

    const auto geometryType = geometryTypeFromEnum(type);
    geometry["type"] = geometryType;
    if (geometryType == "GeometryCollection") {
        geometry["geometries"] = std::move(geometries);
    } else {
        geometry["coordinates"] = decodeCoordinates(type, lengths, coords, context.dimensions, context.precisionScale);
    }
    return geometry;
}

nlohmann::json decodeGeobuf(const ::djinni::DataRef &geobuf) {
    DecoderContext context;
    nlohmann::json decodedData;
    bool hasData = false;

    protozero::pbf_reader data(reinterpret_cast<const char *>(geobuf.buf()), geobuf.len());
    while (data.next()) {
        switch (data.tag()) {
        case 1:
            context.keys.push_back(data.get_string());
            break;
        case 2:
            context.dimensions = std::max<uint32_t>(1, data.get_uint32());
            break;
        case 3: {
            const auto precision = data.get_uint32();
            context.precisionScale = std::pow(10.0, static_cast<double>(precision));
            break;
        }
        case 4:
            decodedData = decodeFeatureCollection(data.get_message(), context);
            hasData = true;
            break;
        case 5:
            decodedData = decodeFeature(data.get_message(), context);
            hasData = true;
            break;
        case 6:
            decodedData = decodeGeometry(data.get_message(), context);
            hasData = true;
            break;
        default:
            data.skip();
            break;
        }
    }

    if (!hasData) {
        throw std::runtime_error("geobuf has no decodable data payload");
    }

    return decodedData;
}

} // namespace

std::shared_ptr<GeoJson> GeobufParser::getGeoJson(const ::djinni::DataRef &geobuf, StringInterner &stringTable) {
    if (!geobuf.buf() || geobuf.len() == 0) {
        throw std::runtime_error("geobuf payload is empty");
    }

    try {
        return GeoJsonParser::getGeoJson(decodeGeobuf(geobuf), stringTable);
    } catch (const protozero::exception &ex) {
        throw std::runtime_error(std::string("invalid geobuf payload: ") + ex.what());
    }
}
