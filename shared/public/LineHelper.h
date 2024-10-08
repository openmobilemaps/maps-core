/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "CoordinateConversionHelperInterface.h"
#include "LineInfoInterface.h"
#include <cmath>

class LineHelper {
  public:
    static bool pointWithin(const std::shared_ptr<LineInfoInterface> &line,
                            const Coord &point,
                            double pointSystemDistance,
                            const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper) {
        return LineHelper::pointWithin(line->getCoordinates(), point, pointSystemDistance, conversionHelper);
    }

    static bool pointWithin(const std::vector<::Coord> &coordinates,
                            const Coord &point,
                            double pointSystemDistance,
                            const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper) {

        auto pointInRenderSystem = conversionHelper->convertToRenderSystem(point);

        std::optional<Coord> lastPoint = std::nullopt;
        for (auto const &coord: coordinates) {
            auto linePointInRenderSystem = conversionHelper->convertToRenderSystem(coord);

            if (lastPoint.has_value()) {
                auto distance = sqrt(LineHelper::distanceSquared(pointInRenderSystem, *lastPoint, linePointInRenderSystem));
                if (distance < pointSystemDistance) {
                    return true;
                }
            }

            lastPoint = linePointInRenderSystem;
        }
        return false;
    }

    static std::vector<Coord> subdividePolyline(const std::vector<Coord>& polyline, double maxSegmentLength) {
        std::vector<Coord> newPolyline;
        newPolyline.push_back(polyline[0]);

        for (size_t i = 1; i < polyline.size(); ++i) {
            const Coord start = newPolyline.back();
            const Coord& end = polyline[i];

            double segmentLength = distanceTo(start, end);

            if (segmentLength > maxSegmentLength) {
                // Calculate number of divisions needed
                int numDivisions = static_cast<int>(std::ceil(segmentLength / maxSegmentLength));

                for (int j = 1; j <= numDivisions; ++j) {
                    double t = static_cast<double>(j) / numDivisions;
                    newPolyline.push_back(interpolate(start, end, t));
                }
            } else {
                newPolyline.push_back(end);
            }
        }

        return newPolyline;
    }



private:
    static float distanceSquared(const Coord& pt, const Coord &p1, const Coord &p2)
    {
        // distance squared from pt to line [p1,p2]

        // line dx/dy
        float lx = (p2.x - p1.x);
        float ly = (p2.y - p1.y);

        if ((fabs(lx) <= std::numeric_limits<float>::epsilon()) &&
            (fabs(ly) <= std::numeric_limits<float>::epsilon()))
        {
            // It's a point not a line segment.
            float dx = pt.x - p1.x;
            float dy = pt.y - p1.y;
            return dx * dx + dy * dy;
        }

        // Calculate the t that minimizes the distance.
        float t = ((pt.x - p1.x) * lx + (pt.y - p1.y) * ly) / (lx * lx + ly * ly);

        float dx = 0.0;
        float dy = 0.0;

        // See if this represents one of the segment's
        // end points or a point in the middle.
        if (t < 0)
        {
            dx = pt.x - p1.x;
            dy = pt.y - p1.y;
        }
        else if (t > 1)
        {
            dx = pt.x - p2.x;
            dy = pt.y - p2.y;
        }
        else
        {
            dx = pt.x - (p1.x + t * lx);
            dy = pt.y - (p1.y + t * ly);
        }

        return (dx * dx + dy * dy);
    }

    // Compute Euclidean distance to another Coord
    static double distanceTo(const Coord& a, const Coord& b) {
        return std::sqrt((a.x - b.x) * (a.x - b.x) +
                         (a.y - b.y) * (a.y - b.y) +
                         (a.z - b.z) * (a.z - b.z));
    }

    // Linear interpolation between this Coord and another Coord
    static Coord interpolate(const Coord& start, const Coord& end, double t) {
        return Coord(start.systemIdentifier,
                     start.x + (end.x - start.x) * t,
                     start.y + (end.y - start.y) * t,
                     start.z + (end.z - start.z) * t);
    }
};
