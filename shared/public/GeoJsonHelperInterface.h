// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from geo_json_parser.djinni

#pragma once

#include <memory>
#include <string>
#include <vector>

struct GeoJsonPoint;

class GeoJsonHelperInterface {
public:
    virtual ~GeoJsonHelperInterface() = default;

    static /*not-null*/ std::shared_ptr<GeoJsonHelperInterface> independentInstance();

    virtual std::string geoJsonStringFromFeatureInfo(const GeoJsonPoint & point) = 0;

    virtual std::string geoJsonStringFromFeatureInfos(const std::vector<GeoJsonPoint> & points) = 0;
};
