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
#include <cstdint>
#include <functional>

struct VisibleTileCandidate {
    int x;
    int y;
    int levelIndex;
    bool operator==(const VisibleTileCandidate &other) const {
        return x == other.x && y == other.y && levelIndex == other.levelIndex;
    }
};

namespace std {
template <> struct hash<VisibleTileCandidate> {
    size_t operator()(const VisibleTileCandidate &candidate) const {
        size_t h1 = hash<int>()(candidate.x);
        size_t h2 = hash<int>()(candidate.y);
        size_t h3 = hash<int>()(candidate.levelIndex);

        // Combine hashes using a simple combining function, based on the one from Boost library
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};
} // namespace std

static int64_t quantizeHashValue(double value, double scale = 1000.0) { return static_cast<int64_t>(std::llround(value * scale)); }

template <typename T> static void hash_combine(size_t &seed, const T &value) {
    std::hash<T> hasher;
    auto v = hasher(value);
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
