// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from geo_json_parser.djinni

#pragma once

#include "Coord.h"
#include "VectorLayerFeatureInfo.h"
#include <utility>
#include <vector>

struct GeoJsonLine final {
    std::vector<::Coord> points;
    ::VectorLayerFeatureInfo featureInfo;

    GeoJsonLine(std::vector<::Coord> points_,
                ::VectorLayerFeatureInfo featureInfo_)
    : points(std::move(points_))
    , featureInfo(std::move(featureInfo_))
    {}
};
