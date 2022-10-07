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
#include <cmath>
#include <utility>
#include <tuple>

// https://github.com/mapbox/mapbox-gl-native/blob/b8edc2399b9640498ccbbbb5b8f058c63d070933/include/mbgl/util/unitbezier.hpp
class UnitBezier {
public:
    // Calculate the polynomial coefficients, implicit first and last control points are (0,0) and (1,1).
    UnitBezier(double p1x, double p1y, double p2x, double p2y)
        : cx(3.0 * p1x)
        , bx(3.0 * (p2x - p1x) - (3.0 * p1x))
        , ax(1.0 - (3.0 * p1x) - (3.0 * (p2x - p1x) - (3.0 * p1x)))
        , cy(3.0 * p1y)
        , by(3.0 * (p2y - p1y) - (3.0 * p1y))
        , ay(1.0 - (3.0 * p1y) - (3.0 * (p2y - p1y) - (3.0 * p1y))) {
    }

    std::pair<double, double> getP1() const {
        return { cx / 3.0, cy / 3.0 };
    }

    std::pair<double, double> getP2() const {
        return {
            (bx + (3.0 * cx / 3.0) + cx) / 3.0,
            (by + (3.0 * cy / 3.0) + cy) / 3.0,
        };
    }

    double sampleCurveX(double t) const {
        // `ax t^3 + bx t^2 + cx t' expanded using Horner's rule.
        return ((ax * t + bx) * t + cx) * t;
    }

    double sampleCurveY(double t) const {
        return ((ay * t + by) * t + cy) * t;
    }

    double sampleCurveDerivativeX(double t) const {
        return (3.0 * ax * t + 2.0 * bx) * t + cx;
    }

    // Given an x value, find a parametric value it came from.
    double solveCurveX(double x, double epsilon) const {
        double t0;
        double t1;
        double t2;
        double x2;
        double d2;
        int i;

        // First try a few iterations of Newton's method -- normally very fast.
        for (t2 = x, i = 0; i < 8; ++i) {
            x2 = sampleCurveX(t2) - x;
            if (fabs (x2) < epsilon)
                return t2;
            d2 = sampleCurveDerivativeX(t2);
            if (fabs(d2) < 1e-6)
                break;
            t2 = t2 - x2 / d2;
        }

        // Fall back to the bisection method for reliability.
        t0 = 0.0;
        t1 = 1.0;
        t2 = x;

        if (t2 < t0)
            return t0;
        if (t2 > t1)
            return t1;

        while (t0 < t1) {
            x2 = sampleCurveX(t2);
            if (fabs(x2 - x) < epsilon)
                return t2;
            if (x > x2)
                t0 = t2;
            else
                t1 = t2;
            t2 = (t1 - t0) * .5 + t0;
        }

        // Failure.
        return t2;
    }

    double solve(double x, double epsilon) const {
        return sampleCurveY(solveCurveX(x, epsilon));
    }

    bool operator==(const UnitBezier& rhs) const {
        return std::tie(cx, bx, ax, cy, by, ay) ==
            std::tie(rhs.cx, rhs.bx, rhs.ax, rhs.cy, rhs.by, rhs.ay);
    }

  private:
    const double cx;
    const double bx;
    const double ax;

    const double cy;
    const double by;
    const double ay;
};
