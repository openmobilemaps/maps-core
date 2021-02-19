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

#include "PolygonInfo.h"

namespace std {
template <> struct hash<PolygonInfo> {
    inline size_t operator()(const PolygonInfo &obj) const { return std::hash<std::string>{}(obj.identifier); }
};
template <> struct equal_to<PolygonInfo> {
    inline bool operator()(const PolygonInfo &lhs, const PolygonInfo &rhs) const { return lhs.identifier == rhs.identifier; }
};
}; // namespace std
