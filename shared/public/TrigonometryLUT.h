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
    namespace {
        constexpr int coreTableSize = 1024;

        constexpr double pi        = M_PI;
        constexpr double twoPi     = 2.0 * pi;
        constexpr double invPi     = 1.0 / pi;
        constexpr double invTwoPi  = 1.0 / twoPi;
        constexpr double scale     = coreTableSize * invPi;
        constexpr int paddedSize   = coreTableSize + 6;

        // Precomputed sin/cos tables with 3-point padding
        std::array<double, paddedSize> generateTable(bool isSin) {
            std::array<double, paddedSize> table{};
            for (int i = 0; i < coreTableSize; ++i) {
                double angle = (pi * i) / coreTableSize;
                table[i + 3] = isSin ? std::sin(angle) : std::cos(angle);
            }
            for (int i = -3; i < 0; ++i) {
                double angle = (pi * i) / coreTableSize;
                table[3 + i] = isSin ? std::sin(angle) : std::cos(angle);
            }
            for (int i = 0; i < 3; ++i) {
                double angle = pi + (pi * i) / coreTableSize;
                table[coreTableSize + 3 + i] = isSin ? std::sin(angle) : std::cos(angle);
            }
            return table;
        }

        const auto sinTable = generateTable(true);
        const auto cosTable = generateTable(false);

        inline double catmullRom(double y0, double y1, double y2, double y3, double t) {
            double a0 = -0.5 * y0 + 1.5 * y1 - 1.5 * y2 + 0.5 * y3;
            double a1 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3;
            double a2 = -0.5 * y0 + 0.5 * y2;
            double a3 = y1;
            return ((a0 * t + a1) * t + a2) * t + a3;
        }

        struct LookupInfo {
            int startIndex;
            double t;
            bool negate;
        };

        inline LookupInfo reduce(double x) {
            x -= std::floor(x * invTwoPi) * twoPi;

            bool negate = false;
            if (x >= pi) {
                x = twoPi - x;
                negate = true;
            }

            double indexF = x * scale;
            int i1 = static_cast<int>(indexF);
            double t = indexF - i1;

            return { i1 + 3, t, negate };
        }

        inline double interpolate(const std::array<double, paddedSize>& table, const LookupInfo& info, bool isSin = true) {
            const double* base = &table[info.startIndex - 1];
            double value = catmullRom(base[0], base[1], base[2], base[3], info.t);

            // For cosine, don't flip sign on symmetry reduction
            return isSin && info.negate ? -value : value;
        }
    }

    inline double sin(double x) {
        return interpolate(sinTable, reduce(x));
    }

    inline double cos(double x) {
        return interpolate(cosTable, reduce(x), false);
    }

    inline void sincos(double x, double& s, double& c) {
        LookupInfo info = reduce(x);
        s = interpolate(sinTable, info);
        c = interpolate(cosTable, info, false);
    }
}
