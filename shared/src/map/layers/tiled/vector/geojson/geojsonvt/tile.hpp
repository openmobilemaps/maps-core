#pragma once

#include <algorithm>
#include <cmath>
#include "GeoJsonTypes.h"
#include "Vec2D.h"
#include "CoordinateConversionHelperInterface.h"

struct Tile: public GeoJSONTileInterface {
    std::vector<std::shared_ptr<GeoJsonGeometry>> features;
    uint32_t num_points = 0;
    uint32_t num_simplified = 0;

public:
    const std::vector<std::shared_ptr<GeoJsonGeometry>>& getFeatures() const override {
        return features;
    };
};

class InternalTile {
public:
    const uint16_t extent;
    const uint8_t z;
    const uint32_t x;
    const uint32_t y;

    const double z2;
    const double tolerance;
    const double sq_tolerance;

    std::vector<std::shared_ptr<GeoJsonGeometry>> source_features;
    Vec2D bboxMin = Vec2D(2.0, 1.0);
    Vec2D bboxMax = Vec2D(-1.0, 0.0);

    Tile tile;

    InternalTile(const std::vector<std::shared_ptr<GeoJsonGeometry>>& source,
                 const uint8_t z_,
                 const uint32_t x_,
                 const uint32_t y_,
                 const uint16_t extent_,
                 const double tolerance_)
    : extent(extent_),
    z(z_),
    x(x_),
    y(y_),
    z2(std::pow(2, z)),
    tolerance(tolerance_),
    sq_tolerance(tolerance_ * tolerance_) {

        tile.features.reserve(source.size());
        for (const auto& feature : source) {
            addFeature(feature);

            bboxMin.x = std::min(feature->bboxMin.x, bboxMin.x);
            bboxMin.y = std::min(feature->bboxMin.y, bboxMin.y);
            bboxMax.x = std::max(feature->bboxMax.x, bboxMax.x);
            bboxMax.y = std::max(feature->bboxMax.y, bboxMax.y);
        }
    }

private:
    void addFeature(const std::shared_ptr<GeoJsonGeometry> &geometry) {
        uint32_t num_points = 0;

        std::shared_ptr<FeatureContext> featureContext = geometry->featureContext;
        std::vector<std::vector<::Coord>> coordinates;
        std::vector<std::vector<std::vector<::Coord>>> holes;

        featureContext = geometry->featureContext;
        coordinates.reserve(geometry->coordinates.size());

        for (const auto& points : geometry->coordinates) {
            std::vector<::Coord> temp;
            temp.reserve(points.size());
            for (const auto& point : points) {
                temp.push_back(transform(point));
            }
            simplify(temp, tolerance);
            num_points += temp.size();
            coordinates.push_back(temp);
        }

        holes.reserve(geometry->holes.size());

        for (const auto& pointsVec : geometry->holes) {
            std::vector<std::vector<::Coord>> temp;
            temp.reserve(pointsVec.size());
            for (const auto& points : pointsVec) {
                std::vector<::Coord> temp1;
                temp1.reserve(points.size());
                for (const auto& point : points) {
                    temp1.push_back(transform(point));
                }
                simplify(temp1, tolerance);
                num_points += temp1.size();
                temp.push_back(temp1);
            }
            holes.push_back(temp);
        }

        if (num_points != 0) {
            tile.num_points += num_points;
            tile.features.push_back(std::make_shared<GeoJsonGeometry>(featureContext, coordinates, holes));
        }

    }

    Coord transform(const Coord& p) {
        const double scale = 20037508.34 * 2;
        const double x = p.x * scale - scale / 2.0;
        const double y = -p.y * scale + scale / 2.0;
        return Coord{CoordinateSystemIdentifiers::EPSG3857(), x, y, p.z};
    }
};
