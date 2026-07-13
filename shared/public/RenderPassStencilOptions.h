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

#include <cstdint>

namespace StencilBits {
constexpr uint8_t none = 0;
constexpr uint8_t vectorClip = 0b1000'0000;
constexpr uint8_t tileBoundary = 0b0100'0000;
constexpr uint8_t geometryMask = vectorClip;
constexpr uint8_t selfMask = 0b0111'1111;
constexpr uint8_t all = 0b1111'1111;
}

struct StencilClearOptions {
    uint8_t clearMask = StencilBits::none;

    bool isEnabled() const {
        return clearMask != StencilBits::none;
    }

    bool operator==(const StencilClearOptions &other) const {
        return clearMask == other.clearMask;
    }
};

struct StencilWriteOptions {
    uint8_t writeMask = StencilBits::none;
    uint8_t reference = StencilBits::none;

    bool isEnabled() const {
        return writeMask != StencilBits::none;
    }

    bool operator==(const StencilWriteOptions &other) const {
        return writeMask == other.writeMask &&
               reference == other.reference;
    }
};

struct StencilReadOptions {
    uint8_t readMask = StencilBits::none;
    uint8_t reference = StencilBits::none;

    bool isEnabled() const {
        return readMask != StencilBits::none;
    }

    bool operator==(const StencilReadOptions &other) const {
        return readMask == other.readMask &&
               reference == other.reference;
    }
};

struct RenderPassStencilOptions {
    StencilClearOptions clearBefore;
    StencilWriteOptions write;
    StencilReadOptions read;
    StencilClearOptions clearAfter;

    bool isEnabled() const {
        return clearBefore.isEnabled() ||
               write.isEnabled() ||
               read.isEnabled() ||
               clearAfter.isEnabled();
    }

    bool operator==(const RenderPassStencilOptions &other) const {
        return clearBefore == other.clearBefore &&
               write == other.write &&
               read == other.read &&
               clearAfter == other.clearAfter;
    }

    bool operator!=(const RenderPassStencilOptions &other) const {
        return !(*this == other);
    }
};
