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

#include "LineInfo.h"

namespace std {
template <> struct hash<LineInfo> {
    inline size_t operator()(const LineInfo &obj) const { return std::hash<std::string>{}(obj.identifier); }
};
template <> struct equal_to<LineInfo> {
    inline bool operator()(const LineInfo &lhs, const LineInfo &rhs) const { return lhs.identifier == rhs.identifier; }
};
}; // namespace std
