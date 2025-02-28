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

#ifdef __APPLE__   // For iOS and macOS
#include <cstdint> // For uint16_t
using HalfFloat = uint16_t;

// from: https://github.com/shiltian/half_precision/tree/master
inline HalfFloat toHalfFloat(float value) {
#ifndef __F16CINTRIN_H
    const uint32_t bits = *reinterpret_cast<const uint32_t *>(&value);

    // Extract the sign from the float value
    const uint16_t sign = (bits & 0x80000000) >> 16;
    // Extract the fraction from the float value
    const uint32_t frac32 = bits & 0x7fffff;
    // Extract the exponent from the float value
    const uint8_t exp32 = (bits & 0x7f800000) >> 23;
    const int8_t exp32_diff = exp32 - 127;

    uint16_t exp16 = 0;
    uint16_t frac16 = frac32 >> 13;

    if (__builtin_expect(exp32 == 0xff || exp32_diff > 15, 0)) {
        exp16 = 0x1f;
    } else if (__builtin_expect(exp32 == 0 || exp32_diff < -14, 0)) {
        exp16 = 0;
    } else {
        exp16 = exp32_diff + 15;
    }

    if (__builtin_expect(exp32 == 0xff && frac32 != 0 && frac16 == 0, 0)) {
        // corner case 1: NaN
        // This case happens when FP32 value is NaN whose the fraction part
        // transformed to FP16 counterpart is truncated to 0. We need to flip the
        // high bit to 1 to make it distinguished from inf.
        frac16 = 0x200;
    } else if (__builtin_expect(exp32 == 0 || (exp16 == 0x1f && exp32 != 0xff), 0)) {
        // corner case 2: subnormal
        // All FP32 subnormal values are under the range of FP16 so the fraction
        // part is set to 0.
        // corner case 3: overflow
        frac16 = 0;
    } else if (__builtin_expect(exp16 == 0 && exp32 != 0, 0)) {
        // corner case 4: underflow
        // We use `truncate` mode here.
        frac16 = 0x100 | (frac16 >> 2);
    }

    // Compose the final FP16 binary
    uint16_t ret = 0;
    ret |= sign;
    ret |= exp16 << 10;
    ret |= frac16;

    return ret;
#else
    return _cvtss_sh(val, 0);
#endif
}

inline float fromHalfFloat(HalfFloat value) {
#ifndef __F16CINTRIN_H
    // Extract the sign from the bits
    const uint32_t sign = static_cast<uint32_t>(value & 0x8000) << 16;
    // Extract the exponent from the bits
    const uint8_t exp16 = (value & 0x7c00) >> 10;
    // Extract the fraction from the bits
    uint16_t frac16 = value & 0x3ff;

    uint32_t exp32 = 0;
    if (__builtin_expect(exp16 == 0x1f, 0)) {
        exp32 = 0xff;
    } else if (__builtin_expect(exp16 == 0, 0)) {
        exp32 = 0;
    } else {
        exp32 = static_cast<uint32_t>(exp16) + 112;
    }

    // corner case: subnormal -> normal
    // The denormal number of FP16 can be represented by FP32, therefore we need
    // to recover the exponent and recalculate the fration.
    if (__builtin_expect(exp16 == 0 && frac16 != 0, 0)) {
        uint8_t OffSet = 0;
        do {
            ++OffSet;
            frac16 <<= 1;
        } while ((frac16 & 0x400) != 0x400);
        // mask the 9th bit
        frac16 &= 0x3ff;
        exp32 = 113 - OffSet;
    }

    uint32_t frac32 = frac16 << 13;

    // Compose the final FP32 binary
    uint32_t bits = 0;

    bits |= sign;
    bits |= (exp32 << 23);
    bits |= frac32;

    return *reinterpret_cast<float *>(&bits);
#else
    return _cvtsh_ss(val);
#endif
}
#else // For Android or other platforms
using HalfFloat = float;

inline HalfFloat toHalfFloat(float value) { return value; }
#endif
