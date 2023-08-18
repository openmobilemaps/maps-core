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

        size_t len = geometry->coordinates.size();

        featureContext = geometry->featureContext;
        coordinates.reserve(len);

        std::vector<size_t> polygonIndices;
        polygonIndices.reserve(len);

        for (size_t i = 0; i != len; i++) {
            std::vector<::Coord> temp;
            temp.reserve(geometry->coordinates.at(i).size());
            simplify(geometry->coordinates.at(i), tolerance);
            for (const auto& point : geometry->coordinates.at(i)) {
                if (point.z > sq_tolerance) {
                    temp.push_back(transform(point));
                }
            }

            if (!temp.empty()) {
                num_points += temp.size();
                coordinates.push_back(std::move(temp));
                polygonIndices.push_back(i);
            }
        }

        holes.reserve(polygonIndices.size());

        for (auto const &index: polygonIndices) {
            std::vector<std::vector<::Coord>> temp;
            temp.reserve(geometry->holes.at(index).size());
            for (auto &points : geometry->holes.at(index)) {
                std::vector<::Coord> temp1;
                simplify(temp1, tolerance);
                temp1.reserve(points.size());
                for (const auto &point : points) {
                    if (point.z > sq_tolerance) {
                        temp1.push_back(transform(point));
                    }
                }
                num_points += temp1.size();
                temp.push_back(std::move(temp1));
            }
            holes.push_back(std::move(temp));
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
