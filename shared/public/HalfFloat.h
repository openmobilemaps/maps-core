///*
// * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
// *
// *  This Source Code Form is subject to the terms of the Mozilla Public
// *  License, v. 2.0. If a copy of the MPL was not distributed with this
// *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
// *
// *  SPDX-License-Identifier: MPL-2.0
// */

#pragma once

#ifdef __APPLE__ // For iOS and macOS
#include <cstdint> // For uint16_t
using HalfFloat = uint16_t;

inline HalfFloat toHalfFloat(float value) {
    uint32_t bits = *reinterpret_cast<uint32_t*>(&value);
    uint16_t sign = (bits >> 31) & 0x1;
    uint16_t exponent = ((bits >> 23) & 0xFF) - 127 + 15;
    uint16_t mantissa = (bits >> 13) & 0x3FF;

    if (exponent <= 0) {
        exponent = 0;
        mantissa = 0; // Subnormal or zero
    } else if (exponent > 0x1F) {
        exponent = 0x1F; // Clamp to max
        mantissa = 0;    // Infinity or NaN
    }

    return (sign << 15) | (exponent << 10) | mantissa;
}
#else // For Android or other platforms
using HalfFloat = float;

inline HalfFloat toHalfFloat(float value) { return value; }
#endif
