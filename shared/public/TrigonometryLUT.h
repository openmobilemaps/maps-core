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
#include <array>
#include <cmath>

namespace lut {
    constexpr int tableSize = 8192;
    constexpr double twoPi = 2.0 * M_PI;
    constexpr double invTwoPi = 1.0 / twoPi;

    namespace detail {
        inline std::array<double, tableSize> generateTable(bool isSin) {
            std::array<double, tableSize> table{};
            for (int i = 0; i < tableSize; ++i) {
                double angle = (twoPi * i) / tableSize;
                table[i] = isSin ? std::sin(angle) : std::cos(angle);
            }
            return table;
        }

        inline const std::array<double, tableSize> sinTable = generateTable(true);
        inline const std::array<double, tableSize> cosTable = generateTable(false);

        inline double catmullRom(double y0, double y1, double y2, double y3, double t) {
            double a0 = -0.5 * y0 + 1.5 * y1 - 1.5 * y2 + 0.5 * y3;
            double a1 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3;
            double a2 = -0.5 * y0 + 0.5 * y2;
            double a3 = y1;
            return ((a0 * t + a1) * t + a2) * t + a3;
        }

        inline double interpolate(const std::array<double, tableSize>& table, double x) {
            x -= std::floor(x * invTwoPi) * twoPi;  // wrap into [0, 2π)
            double indexF = x * (tableSize * invTwoPi);
            int i1 = static_cast<int>(indexF);
            double t = indexF - i1;

            // Wrap-safe indices
            int i0 = (i1 - 1 + tableSize) % tableSize;
            int i2 = (i1 + 1) % tableSize;
            int i3 = (i1 + 2) % tableSize;

            return catmullRom(
                table[i0], table[i1], table[i2], table[i3], t
            );
        }
    }

    inline double sin(double x) {
        return detail::interpolate(detail::sinTable, x);
    }

    inline double cos(double x) {
        return detail::interpolate(detail::cosTable, x);
    }

    inline void sincos(double x, double& outSin, double& outCos) {
        x -= std::floor(x * invTwoPi) * twoPi;
        double indexF = x * (tableSize * invTwoPi);
        int i1 = static_cast<int>(indexF);
        double t = indexF - i1;

        // Wrap-safe indices
        int i0 = (i1 - 1 + tableSize) % tableSize;
        int i2 = (i1 + 1) % tableSize;
        int i3 = (i1 + 2) % tableSize;

        outSin = detail::catmullRom(
            detail::sinTable[i0], detail::sinTable[i1],
            detail::sinTable[i2], detail::sinTable[i3], t
        );
        outCos = detail::catmullRom(
            detail::cosTable[i0], detail::cosTable[i1],
            detail::cosTable[i2], detail::cosTable[i3], t
        );
    }
}
