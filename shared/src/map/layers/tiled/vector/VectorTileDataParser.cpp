/*
 * Copyright (c) 2026 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "VectorTileDataParser.h"

#include "Logger.h"
#include "VectorTileGeometryHandler.h"
#include "mlt/decoder.hpp"
#include "vtzero/vector_tile.hpp"

#include <cmath>
#include <limits>

namespace {

using FeatureMapType = Tiled2dMapVectorTileInfo::FeatureMap;
using FeatureTupleVector = std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>;

FeatureMapType createEmptyFeatureMap() {
    return std::make_shared<std::unordered_map<std::string, std::shared_ptr<FeatureTupleVector>>>();
}

static void internAllLayerKeys(const vtzero::layer &layer,
                               StringInterner &stringTable,
                               std::vector<std::string> &outKeys,
                               std::vector<InternedString> &outInternedKeys) {
    outKeys.clear();
    outKeys.reserve(layer.key_table_size());
    for (auto &key : layer.key_table()) {
        outKeys.push_back(std::string{key});
    }
    outInternedKeys.clear();
    outInternedKeys.reserve(outKeys.size());
    stringTable.add(outKeys.begin(), outKeys.end(), std::back_inserter(outInternedKeys));
}

vtzero::GeomType toGeomType(mlt::metadata::tileset::GeometryType type) {
    using MltGeomType = mlt::metadata::tileset::GeometryType;
    switch (type) {
        case MltGeomType::POINT:
        case MltGeomType::MULTIPOINT:
            return vtzero::GeomType::POINT;
        case MltGeomType::LINESTRING:
        case MltGeomType::MULTILINESTRING:
            return vtzero::GeomType::LINESTRING;
        case MltGeomType::POLYGON:
        case MltGeomType::MULTIPOLYGON:
            return vtzero::GeomType::POLYGON;
        default:
            return vtzero::GeomType::UNKNOWN;
    }
}

ValueVariant toValueVariant(const mlt::Property &property) {
    return std::visit(
        overloaded{
            [](std::nullptr_t) -> ValueVariant { return std::monostate{}; },
            [](bool value) -> ValueVariant { return value; },
            [](std::optional<bool> value) -> ValueVariant { return value ? ValueVariant(*value) : ValueVariant(std::monostate{}); },
            [](int32_t value) -> ValueVariant { return static_cast<int64_t>(value); },
            [](std::optional<int32_t> value) -> ValueVariant {
                return value ? ValueVariant(static_cast<int64_t>(*value)) : ValueVariant(std::monostate{});
            },
            [](int64_t value) -> ValueVariant { return value; },
            [](std::optional<int64_t> value) -> ValueVariant { return value ? ValueVariant(*value) : ValueVariant(std::monostate{}); },
            [](uint32_t value) -> ValueVariant { return static_cast<int64_t>(value); },
            [](std::optional<uint32_t> value) -> ValueVariant {
                return value ? ValueVariant(static_cast<int64_t>(*value)) : ValueVariant(std::monostate{});
            },
            [](uint64_t value) -> ValueVariant { return static_cast<int64_t>(value); },
            [](std::optional<uint64_t> value) -> ValueVariant {
                return value ? ValueVariant(static_cast<int64_t>(*value)) : ValueVariant(std::monostate{});
            },
            [](float value) -> ValueVariant { return static_cast<double>(value); },
            [](std::optional<float> value) -> ValueVariant {
                return value ? ValueVariant(static_cast<double>(*value)) : ValueVariant(std::monostate{});
            },
            [](double value) -> ValueVariant { return value; },
            [](std::optional<double> value) -> ValueVariant { return value ? ValueVariant(*value) : ValueVariant(std::monostate{}); },
            [](std::string_view value) -> ValueVariant { return std::string(value); }},
        property);
}

std::shared_ptr<FeatureContext> convertToFeatureContext(const vtzero::feature &feature,
                                                        const vtzero::layer &layer,
                                                        const std::vector<InternedString> &internedLayerKeys) {
    FeatureContext::mapType propertiesMap;
    propertiesMap.reserve(feature.num_properties());
    feature.for_each_property_indexes([&](vtzero::index_value_pair property) {
        auto key = internedLayerKeys[property.key().value()];
        auto value = vtzero::convert_property_value<ValueVariant, property_value_mapping>(layer.value(property.value().value()));
        propertiesMap.emplace_back(key, std::move(value));
        return true;
    });

    uint64_t identifier;
    if (feature.has_id()) {
        identifier = feature.id();
    } else {
        size_t hash = 0;
        for (auto const &[key, value] : propertiesMap) {
            std::hash_combine(hash, std::hash<FeatureContext::valueType>{}(value));
        }
        identifier = hash;
    }
    return std::make_shared<FeatureContext>(feature.geometry_type(), std::move(propertiesMap), identifier);
}

std::shared_ptr<FeatureContext> convertToFeatureContext(const mlt::Feature &feature,
                                                        const mlt::Layer &layer,
                                                        StringInterner &stringTable) {
    FeatureContext::mapType propertiesMap;
    const auto &layerProperties = layer.getProperties();
    propertiesMap.reserve(layerProperties.size());
    for (const auto &entry : layerProperties) {
        const auto &key = entry.first;
        auto property = feature.getProperty(key, layer);
        if (!property.has_value()) {
            continue;
        }
        propertiesMap.emplace_back(stringTable.add(key), toValueVariant(*property));
    }

    return std::make_shared<FeatureContext>(toGeomType(feature.getGeometry().type), std::move(propertiesMap), feature.getID());
}

void appendMapCoordinates(const std::vector<mlt::CoordVec> &rings, VectorTileGeometryHandler &geometryHandler) {
    auto &coordinates = geometryHandler.getLineCoordinates();
    for (const auto &ring : rings) {
        coordinates.emplace_back();
        coordinates.back().reserve(ring.size());
        for (const auto &point : ring) {
            coordinates.back().push_back(geometryHandler.convertTileCoordinate(point.x, point.y, false));
        }
    }
}

bool appendPreTriangulatedPolygon(const std::vector<mlt::CoordVec> &rings,
                                  std::span<const uint32_t> triangles,
                                  VectorTileGeometryHandler &geometryHandler) {
    if (triangles.empty() || rings.empty()) {
        return false;
    }

    appendMapCoordinates(rings, geometryHandler);

    std::vector<Vec2D> coordinates;
    size_t coordinateCount = 0;
    for (const auto &ring : rings) {
        coordinateCount += ring.size();
    }
    coordinates.reserve(coordinateCount);
    for (const auto &ring : rings) {
        for (const auto &point : ring) {
            coordinates.push_back(geometryHandler.convertTileCoordinate(point.x, point.y, true));
        }
    }

    std::vector<uint16_t> indices;
    indices.reserve(triangles.size());
    for (const auto index : triangles) {
        if (index >= coordinates.size() || index > std::numeric_limits<uint16_t>::max()) {
            return false;
        }
        indices.push_back(static_cast<uint16_t>(index));
    }

    geometryHandler.addTriangulatedPolygon(std::move(coordinates), std::move(indices));
    return true;
}

void triangulateRings(const std::vector<mlt::CoordVec> &rings,
                      VectorTileGeometryHandler &geometryHandler,
                      mapbox::detail::Earcut<uint16_t> &earcutter) {
    if (rings.empty()) {
        return;
    }

    for (size_t i = 0; i < rings.size(); ++i) {
        geometryHandler.ring_begin(static_cast<uint32_t>(rings[i].size()));
        for (const auto &point : rings[i]) {
            geometryHandler.ring_point(vtzero::point(static_cast<int32_t>(std::lround(point.x)), static_cast<int32_t>(std::lround(point.y))));
        }
        geometryHandler.ring_end(i == 0 ? vtzero::ring_type::outer : vtzero::ring_type::inner);
    }

    size_t polygonCount = geometryHandler.beginTriangulatePolygons();
    for (size_t i = 0; i < polygonCount; ++i) {
        geometryHandler.triangulatePolygons(i, earcutter);
    }
    geometryHandler.endTringulatePolygons();
}

bool decodeMltGeometry(const mlt::geometry::Geometry &geometry,
                       VectorTileGeometryHandler &geometryHandler,
                       mapbox::detail::Earcut<uint16_t> &earcutter) {
    using GeometryType = mlt::metadata::tileset::GeometryType;
    const auto &triangles = geometry.getTriangles();
    switch (geometry.type) {
        case GeometryType::POINT: {
            const auto *point = dynamic_cast<const mlt::geometry::Point *>(&geometry);
            if (!point) {
                return false;
            }
            auto &coordinates = geometryHandler.getLineCoordinates();
            coordinates.emplace_back();
            coordinates.back().push_back(geometryHandler.convertTileCoordinate(point->getCoordinate().x, point->getCoordinate().y, false));
            return true;
        }
        case GeometryType::MULTIPOINT: {
            const auto *multiPoint = dynamic_cast<const mlt::geometry::MultiPoint *>(&geometry);
            if (!multiPoint) {
                return false;
            }
            auto &coordinates = geometryHandler.getLineCoordinates();
            coordinates.emplace_back();
            coordinates.back().reserve(multiPoint->getCoordinates().size());
            for (const auto &point : multiPoint->getCoordinates()) {
                coordinates.back().push_back(geometryHandler.convertTileCoordinate(point.x, point.y, false));
            }
            return true;
        }
        case GeometryType::LINESTRING: {
            const auto *lineString = dynamic_cast<const mlt::geometry::LineString *>(&geometry);
            if (!lineString) {
                return false;
            }
            auto &coordinates = geometryHandler.getLineCoordinates();
            coordinates.emplace_back();
            coordinates.back().reserve(lineString->getCoordinates().size());
            for (const auto &point : lineString->getCoordinates()) {
                coordinates.back().push_back(geometryHandler.convertTileCoordinate(point.x, point.y, false));
            }
            return true;
        }
        case GeometryType::MULTILINESTRING: {
            const auto *multiLineString = dynamic_cast<const mlt::geometry::MultiLineString *>(&geometry);
            if (!multiLineString) {
                return false;
            }
            auto &coordinates = geometryHandler.getLineCoordinates();
            for (const auto &line : multiLineString->getLineStrings()) {
                coordinates.emplace_back();
                coordinates.back().reserve(line.size());
                for (const auto &point : line) {
                    coordinates.back().push_back(geometryHandler.convertTileCoordinate(point.x, point.y, false));
                }
            }
            return true;
        }
        case GeometryType::POLYGON: {
            const auto *polygon = dynamic_cast<const mlt::geometry::Polygon *>(&geometry);
            if (!polygon) {
                return false;
            }
            if (appendPreTriangulatedPolygon(polygon->getRings(), triangles, geometryHandler)) {
                return true;
            }
            triangulateRings(polygon->getRings(), geometryHandler, earcutter);
            return true;
        }
        case GeometryType::MULTIPOLYGON: {
            const auto *multiPolygon = dynamic_cast<const mlt::geometry::MultiPolygon *>(&geometry);
            if (!multiPolygon) {
                return false;
            }

            if (!triangles.empty()) {
                std::vector<mlt::CoordVec> flattenedRings;
                for (const auto &polygon : multiPolygon->getPolygons()) {
                    flattenedRings.insert(flattenedRings.end(), polygon.begin(), polygon.end());
                }
                if (appendPreTriangulatedPolygon(flattenedRings, triangles, geometryHandler)) {
                    return true;
                }
            }

            for (const auto &polygon : multiPolygon->getPolygons()) {
                triangulateRings(polygon, geometryHandler, earcutter);
            }
            return true;
        }
        default:
            return false;
    }
}

FeatureMapType parseMvt(const uint8_t *data,
                        size_t dataLength,
                        Tiled2dMapTileInfo tileInfo,
                        const RectCoord &tileBounds,
                        const std::optional<Tiled2dMapVectorSettings> &vectorSettings,
                        const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                        StringInterner &stringTable,
                        const std::unordered_set<std::string> &layersToDecode,
                        const std::function<bool()> &shouldContinue) {
    auto layerFeatureMap = createEmptyFeatureMap();

    vtzero::vector_tile tileData((char *)data, dataLength);
    std::vector<std::string> layerKeys;
    std::vector<InternedString> internedLayerKeys;
    mapbox::detail::Earcut<uint16_t> earcutter;

    while (auto layer = tileData.next_layer()) {
        std::string sourceLayerName = std::string(layer.name());
        if ((!layersToDecode.empty() && layersToDecode.count(sourceLayerName) == 0) || layer.empty()) {
            continue;
        }

        int extent = static_cast<int>(layer.extent());
        layerFeatureMap->emplace(sourceLayerName, std::make_shared<FeatureTupleVector>());
        layerFeatureMap->at(sourceLayerName)->reserve(layer.num_features());

        internAllLayerKeys(layer, stringTable, layerKeys, internedLayerKeys);
        while (const auto &feature = layer.next_feature()) {
            if (shouldContinue && !shouldContinue()) {
                return createEmptyFeatureMap();
            }

            auto featureContext = convertToFeatureContext(feature, layer, internedLayerKeys);
            auto geometryHandler = std::make_shared<VectorTileGeometryHandler>(tileBounds, extent, vectorSettings, conversionHelper);
            try {
                vtzero::decode_geometry(feature.geometry(), *geometryHandler);
                size_t polygonCount = geometryHandler->beginTriangulatePolygons();
                for (size_t i = 0; i < polygonCount; ++i) {
                    if (shouldContinue && !shouldContinue()) {
                        return createEmptyFeatureMap();
                    }
                    geometryHandler->triangulatePolygons(i, earcutter);
                }
                geometryHandler->endTringulatePolygons();
            } catch (const vtzero::geometry_exception &) {
                LogError <<= "geometryException for tile " + std::to_string(tileInfo.zoomIdentifier) + "/" + std::to_string(tileInfo.x) + "/" +
                             std::to_string(tileInfo.y);
                continue;
            }

            layerFeatureMap->at(sourceLayerName)->push_back({featureContext, geometryHandler});
        }

        if (layerFeatureMap->at(sourceLayerName)->empty()) {
            layerFeatureMap->erase(sourceLayerName);
        }
    }

    return layerFeatureMap;
}

FeatureMapType parseMlt(const uint8_t *data,
                        size_t dataLength,
                        Tiled2dMapTileInfo tileInfo,
                        const RectCoord &tileBounds,
                        const std::optional<Tiled2dMapVectorSettings> &vectorSettings,
                        const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                        StringInterner &stringTable,
                        const std::unordered_set<std::string> &layersToDecode,
                        const std::function<bool()> &shouldContinue) {
    auto layerFeatureMap = createEmptyFeatureMap();

    mlt::Decoder decoder;
    auto tileData = decoder.decode(mlt::DataView(reinterpret_cast<const char *>(data), dataLength));
    mapbox::detail::Earcut<uint16_t> earcutter;

    for (const auto &layer : tileData.getLayers()) {
        const std::string sourceLayerName = layer.getName();
        const auto &features = layer.getFeatures();
        if ((!layersToDecode.empty() && layersToDecode.count(sourceLayerName) == 0) || features.empty()) {
            continue;
        }

        const int extent = static_cast<int>(layer.getExtent());
        layerFeatureMap->emplace(sourceLayerName, std::make_shared<FeatureTupleVector>());
        layerFeatureMap->at(sourceLayerName)->reserve(features.size());

        for (const auto &feature : features) {
            if (shouldContinue && !shouldContinue()) {
                return createEmptyFeatureMap();
            }

            auto featureContext = convertToFeatureContext(feature, layer, stringTable);
            auto geometryHandler = std::make_shared<VectorTileGeometryHandler>(tileBounds, extent, vectorSettings, conversionHelper);
            if (!decodeMltGeometry(feature.getGeometry(), *geometryHandler, earcutter)) {
                LogError <<= "Unsupported MLT geometry for tile " + std::to_string(tileInfo.zoomIdentifier) + "/" + std::to_string(tileInfo.x) +
                             "/" + std::to_string(tileInfo.y);
                continue;
            }
            layerFeatureMap->at(sourceLayerName)->push_back({featureContext, geometryHandler});
        }

        if (layerFeatureMap->at(sourceLayerName)->empty()) {
            layerFeatureMap->erase(sourceLayerName);
        }
    }

    return layerFeatureMap;
}

} // namespace

FeatureMapType VectorTileDataParser::parse(const uint8_t *data,
                                           size_t dataLength,
                                           Tiled2dMapTileInfo tileInfo,
                                           const RectCoord &tileBounds,
                                           const std::optional<Tiled2dMapVectorSettings> &vectorSettings,
                                           const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                           StringInterner &stringTable,
                                           const std::unordered_set<std::string> &layersToDecode,
                                           VectorTileSourceFormat format,
                                           const std::function<bool()> &shouldContinue) {
    switch (format) {
        case VectorTileSourceFormat::MLT:
            try {
                return parseMlt(data, dataLength, tileInfo, tileBounds, vectorSettings, conversionHelper, stringTable, layersToDecode, shouldContinue);
            } catch (const std::exception &) {
                LogError <<= "MLT decode exception for tile " + std::to_string(tileInfo.zoomIdentifier) + "/" + std::to_string(tileInfo.x) + "/" +
                             std::to_string(tileInfo.y);
                return createEmptyFeatureMap();
            }
        case VectorTileSourceFormat::MVT:
        default:
            try {
                return parseMvt(data, dataLength, tileInfo, tileBounds, vectorSettings, conversionHelper, stringTable, layersToDecode, shouldContinue);
            } catch (const protozero::invalid_tag_exception &) {
                LogError <<= "Invalid tag exception for tile " + std::to_string(tileInfo.zoomIdentifier) + "/" + std::to_string(tileInfo.x) + "/" +
                             std::to_string(tileInfo.y);
                return createEmptyFeatureMap();
            } catch (const protozero::unknown_pbf_wire_type_exception &) {
                LogError <<= "Unknown wire type exception for tile " + std::to_string(tileInfo.zoomIdentifier) + "/" +
                             std::to_string(tileInfo.x) + "/" + std::to_string(tileInfo.y);
                return createEmptyFeatureMap();
            }
    }
}

