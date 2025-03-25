#include "CoordinateConversionHelperInterface.h"
#include "RectCoord.h"
#include "json.h"
#include <iostream>
#include <json.h>

#pragma once


using json = nlohmann::json;

class GeoJsonGenerator {
public:
    GeoJsonGenerator() {
        geojson["type"] = "FeatureCollection";
        geojson["features"] = json::array();
    }

    void addRect(const RectCoord &coordinates, const std::string& color) {
        json feature;
        feature["type"] = "Feature";
        feature["properties"] = { {"fill", color} };
        feature["geometry"] = {
            {"type", "Polygon"},
            {"coordinates", json::array({ createCoordinateArray(coordinates) })}
        };
        geojson["features"].push_back(feature);
    }

    void printGeoJson() const {
        std::cout << geojson.dump(4) << std::endl;
    }

private:
    json geojson;

    json createCoordinateArray(const RectCoord& coordinates) {
        const auto converted = CoordinateConversionHelperInterface::independentInstance()->convertRect(4326, coordinates);
        json coordArray = json::array();
        coordArray.push_back({ converted.topLeft.x, converted.topLeft.y });
        coordArray.push_back({ converted.bottomRight.x, converted.topLeft.y });
        coordArray.push_back({ converted.bottomRight.x, converted.bottomRight.y });
        coordArray.push_back({ converted.topLeft.x, converted.bottomRight.y });
        coordArray.push_back({ converted.topLeft.x, converted.topLeft.y });
        return coordArray;
    }
};