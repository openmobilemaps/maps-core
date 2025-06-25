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

        inline int fastIndex(double x) {
            x -= std::floor(x * invTwoPi) * twoPi;  // wrap into [0, 2π)
            double norm = x * (tableSize * invTwoPi);
            int index = static_cast<int>(norm);
            return index >= tableSize ? 0 : index;  // safe fallback
        }
    }

    inline double sin(double x) {
        return detail::sinTable[detail::fastIndex(x)];
    }

    inline double cos(double x) {
        return detail::cosTable[detail::fastIndex(x)];
    }

    inline void sincos(double x, double& outSin, double& outCos) {
        int i = detail::fastIndex(x);
        outSin = detail::sinTable[i];
        outCos = detail::cosTable[i];
    }
}
