/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */


#include "ReverseGeocoder.h"
#include "CoordinateConversionHelperInterface.h"
#include "DataLoaderResult.h"
#include "VectorTileGeometryHandler.h"
#include "Value.h"
#include "Vec2DHelper.h"
#include "vtzero/vector_tile.hpp"
#include "CoordinateSystemIdentifiers.h"
#include "VectorLayerFeatureCoordInfo.h"
#include "CoordinateSystemIdentifiers.h"
#include "Logger.h"

ReverseGeocoder::ReverseGeocoder(const /*not-null*/ std::shared_ptr<::LoaderInterface> & loader, const std::string & tileUrlTemplate, int32_t zoomLevel): loader(loader), tileUrlTemplate(tileUrlTemplate), zoomLevel(zoomLevel) {}


double ReverseGeocoder::distance(const Coord &c1, const Coord &c2) {
    const auto coordinateConverter = CoordinateConversionHelperInterface::independentInstance();
    Coord wgsC1 = coordinateConverter->convert(CoordinateSystemIdentifiers::EPSG4326(), c1);
    Coord wgsC2 = coordinateConverter->convert(CoordinateSystemIdentifiers::EPSG4326(), c2);

    const double R = 6371000; // Radius of the earth in meters
    double latDistance = (wgsC2.y - wgsC1.y) * M_PI / 180.0;
    double lonDistance = (wgsC2.x - wgsC1.x) * M_PI / 180.0;
    double a = std::sin(latDistance / 2) * std::sin(latDistance / 2) +
               std::cos(wgsC1.y * M_PI / 180.0) * std::cos(wgsC2.y * M_PI / 180.0) *
               std::sin(lonDistance / 2) * std::sin(lonDistance / 2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    return R * c;
}


std::vector<::VectorLayerFeatureCoordInfo> ReverseGeocoder::reverseGeocode(const ::Coord & coord, int64_t thresholdMeters) {

    auto conversionHelper = CoordinateConversionHelperInterface::independentInstance();
    auto converted = conversionHelper->convert(CoordinateSystemIdentifiers::EPSG3857(), coord);
    auto converted4326 = conversionHelper->convert(CoordinateSystemIdentifiers::EPSG4326(), coord);

    const double ORIGIN_SHIFT = 20037508.34;
    int tileCount = pow(2, zoomLevel);

    double tileSize = (2 * ORIGIN_SHIFT) / (tileCount);

    // Calculate the tile coordinates
    int x = std::clamp(int(floor((converted.x + ORIGIN_SHIFT) / tileSize)), 0, tileCount - 1);
    int y = std::clamp(int(floor((ORIGIN_SHIFT - converted.y) / tileSize)), 0, tileCount - 1);

    std::string url = tileUrlTemplate;
    size_t zoomIndex = url.find("{z}", 0);
    if (zoomIndex == std::string::npos)
        throw std::invalid_argument("Layer url \'" + url + "\' has no valid format!");
    url = url.replace(zoomIndex, 3, std::to_string(zoomLevel));
    size_t xIndex = url.find("{x}", 0);
    if (xIndex == std::string::npos)
        throw std::invalid_argument("Layer url \'" + url + "\' has no valid format!");
    url = url.replace(xIndex, 3, std::to_string(x));
    size_t yIndex = url.find("{y}", 0);
    if (yIndex == std::string::npos)
        throw std::invalid_argument("Layer url \'" + url + "\' has no valid format!");
    url = url.replace(yIndex, 3, std::to_string(y));

    auto result = loader->loadData(url, std::nullopt);

    std::vector<::VectorLayerFeatureCoordInfo> resultVector;

    if (result.status != LoaderStatus::OK) {
        // TODO: errorhandling
        return resultVector;
    }

    if (!result.data.has_value()) {
        // TODO: errorhandling
        return resultVector;
    }

    auto targetVec = Vec2D(converted.x, converted.y);


    // Calculate the bounds
    double minx = x * tileSize - ORIGIN_SHIFT;
    double maxx = (x + 1) * tileSize - ORIGIN_SHIFT;
    double miny = ORIGIN_SHIFT - (y + 1) * tileSize;
    double maxy = ORIGIN_SHIFT - y * tileSize;

    auto topLeft = Coord(CoordinateSystemIdentifiers::EPSG3857(), minx, maxy, 0.0);
    auto bottomRight = Coord(CoordinateSystemIdentifiers::EPSG3857(), maxx, miny, 0.0);
    auto tileBounds = RectCoord(topLeft, bottomRight);

    auto conv = conversionHelper->convertRect(4326, tileBounds);

    StringInterner stringTable = ValueKeys::newStringInterner();
    try {
        vtzero::vector_tile tileData((char*)result.data->buf(), result.data->len());

        while (auto layer = tileData.next_layer()) {
            std::string sourceLayerName = std::string(layer.name());

            while (const auto &feature = layer.next_feature()) {
                if (feature.geometry_type() != vtzero::GeomType::POINT) {
                    continue;
                }
                try {
                    int extent = (int) layer.extent();
                    auto const featureContext = std::make_shared<FeatureContext>(stringTable, feature);
                    std::shared_ptr<VectorTileGeometryHandler> geometryHandler = std::make_shared<VectorTileGeometryHandler>(tileBounds, extent, std::nullopt, conversionHelper);
                    vtzero::decode_geometry(feature.geometry(), *geometryHandler);

                    for (auto points: geometryHandler->getPointCoordinates()) {
                        for (auto point: points) {
                            auto coord = Coord(CoordinateSystemIdentifiers::EPSG3857(), point.x, point.y, 0.0);
                            auto d = distance(converted4326, coord);
                            if (d < thresholdMeters) {
                                resultVector.push_back(VectorLayerFeatureCoordInfo(featureContext->getFeatureInfo(stringTable), coord));
                            }
                        }
                    }
                } catch (const vtzero::geometry_exception &geometryException) {
                    LogError <<= "geometryException";
                    continue;
                }
            }
        }
    }
    catch (const protozero::invalid_tag_exception &tagException) {
        LogError <<= "Invalid tag exception";
    }
    catch (const protozero::unknown_pbf_wire_type_exception &typeException) {
        LogError <<= "Unknown wire type exception";
    }

    return resultVector;
}

std::optional<::VectorLayerFeatureCoordInfo> ReverseGeocoder::reverseGeocodeClosest(const ::Coord & coord, int64_t thresholdMeters) {

    auto results = reverseGeocode(coord, thresholdMeters);
    auto minDistance = std::numeric_limits<float>::max();

    std::optional<::VectorLayerFeatureCoordInfo> closestFeature = std::nullopt;

    for(const auto& r : results) {
        const auto d = distance(coord, r.coordinates);

        if(d < minDistance) {
            closestFeature = r;
            minDistance = d;
        }
    }

    return closestFeature;
}
