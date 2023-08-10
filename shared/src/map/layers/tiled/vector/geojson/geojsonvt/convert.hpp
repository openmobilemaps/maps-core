#pragma once

#include "simplify.hpp"
#include "CoordinateSystemIdentifiers.h"
#include "GeoJsonTypes.h"
#include "Logger.h"

#include <algorithm>
#include <cmath>


struct project {
    const double tolerance;

    Coord operator()(const Coord& p) {
        const double sine = std::sin(p.y * M_PI / 180);
        const double x = p.x / 360 + 0.5;
        const double y =
            std::max(std::min(0.5 - 0.25 * std::log((1 + sine) / (1 - sine)) / M_PI, 1.0), 0.0);
        return Coord {-1, x, y, p.z };
    }

    void convert(const std::shared_ptr<GeoJsonGeometry> &geometry) {
        std::vector<std::vector<::Coord>> result;

        if (geometry->coordinates.empty() && geometry->holes.empty()) {
            return;
        }

        result.reserve(geometry->coordinates.size());

        for (const auto& points : geometry->coordinates) {
            std::vector<::Coord> temp;
            temp.reserve(points.size());
            for (const auto& point : points) {
                temp.push_back(operator()(point));
                geometry->bboxMin.x = std::min(temp.back().x, geometry->bboxMin.x);
                geometry->bboxMin.y = std::min(temp.back().y, geometry->bboxMin.y);
                geometry->bboxMax.x = std::max(temp.back().x, geometry->bboxMax.x);
                geometry->bboxMax.y = std::max(temp.back().y, geometry->bboxMax.y);
            }

            simplify(temp, tolerance);
            result.push_back(temp);
        }
        geometry->coordinates = result;

        std::vector<std::vector<std::vector<::Coord>>> holes;
        holes.reserve(geometry->holes.size());

        for (const auto& pointsVec : geometry->holes) {
            std::vector<std::vector<::Coord>> temp;
            temp.reserve(pointsVec.size());
            for (const auto& points : pointsVec) {
                std::vector<::Coord> temp1;
                temp1.reserve(points.size());
                for (const auto& point : points) {
                    temp1.push_back(operator()(point));
                    geometry->bboxMin.x = std::min(temp1.back().x, geometry->bboxMin.x);
                    geometry->bboxMin.y = std::min(temp1.back().y, geometry->bboxMin.y);
                    geometry->bboxMax.x = std::max(temp1.back().x, geometry->bboxMax.x);
                    geometry->bboxMax.y = std::max(temp1.back().y, geometry->bboxMax.y);
                }
                simplify(temp1, tolerance);
                temp.push_back(temp1);
            }
            holes.push_back(temp);
        }
        geometry->holes = holes;
    }
};

inline void convert(const std::vector<std::shared_ptr<GeoJsonGeometry>>& features, const double tolerance) {
    project projector{ tolerance };
    for (const auto& feature : features) {
        projector.convert(feature);
    }
}
