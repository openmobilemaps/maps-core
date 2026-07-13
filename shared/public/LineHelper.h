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
#include "Coord.h"
#include "Vec2D.h"
#include "Vec2F.h"

class LineHelper {
  public:
    static bool pointWithin(const std::shared_ptr<LineInfoInterface> &line,
                            const Coord &point,
                            double pointSystemDistance,
                            const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper);

    static bool pointWithin(const std::vector<::Coord> &coordinates,
                            const Coord &point,
                            double pointSystemDistance,
                            const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper);

    static bool pointWithin(const std::vector<::Vec2D> &coordinates,
                            const Coord &point,
                            const int32_t systemIdentifier,
                            double pointSystemDistance,
                            const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper);

    static std::vector<Vec2D> subdividePolyline(const std::vector<Vec2D>& polyline, double maxSegmentLength);

    /**
     * Distance squared from pt to line [segmentStart,segmentEnd]
     * @returns distance squared, t in [0,1]
     */
    static std::pair<float, float> distanceSquared(const Vec2F &pt, const Vec2F &segmentStart, const Vec2F &segmentEnd);
    /**
     * Distance squared from pt to line [segmentStart,segmentEnd]
     * @returns distance squared, t in [0,1]
     */
    static std::pair<double, double> distanceSquared(const Vec2D &pt, const Vec2D &segmentStart, const Vec2D &segmentEnd);
    /**
     * Distance squared from pt to line [segmentStart,segmentEnd].
     *
     * Considers only x, y dimensions. Returns distance in coordinate system units.
     *
     * @pre Coords must be in same coordinate system
     * @returns distance squared, t in [0,1]
     */
    static std::pair<double, double> distanceSquared(const Coord &pt, const Coord &segmentStart, const Coord &segmentEnd);
};
