/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "earcut.hpp"
#include "Vec2D.h"
#include "Vec2F.h"

namespace mapbox {
namespace util {

template <>
struct nth<0, ::Vec2D> {
    inline static auto get(const ::Vec2D &t) {
        return t.x;
    };
};
template <>
struct nth<1, ::Vec2D> {
    inline static auto get(const ::Vec2D &t) {
        return t.y;
    };
};

template <>
struct nth<0, ::Vec2F> {
    inline static auto get(const ::Vec2F &t) {
        return t.x;
    };
};
template <>
struct nth<1, ::Vec2F> {
    inline static auto get(const ::Vec2F &t) {
        return t.y;
    };
};

} // namespace util
} // namespace mapbox
