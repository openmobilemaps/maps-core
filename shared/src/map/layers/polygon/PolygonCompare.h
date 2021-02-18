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

#include "Polygon.h"

namespace std {
template <> struct hash<Polygon> {
    inline size_t operator()(const Polygon &obj) const { return std::hash<std::string>{}(obj.identifier); }
};
template <> struct equal_to<Polygon> {
    inline bool operator()(const Polygon &lhs, const Polygon &rhs) const { return lhs.identifier == rhs.identifier; }
};
}; // namespace std
