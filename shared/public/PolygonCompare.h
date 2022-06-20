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
#include "Coord.h"
#include <cmath>
#include <limits>

namespace std {
template <> struct equal_to<PolygonInfo> {
    inline bool operator()(const PolygonInfo &lhs, const PolygonInfo &rhs) const {  if(lhs.identifier != rhs.identifier) { return false; }
        if(lhs.coordinates.positions.size() != rhs.coordinates.positions.size()) { return false; }

        for(auto i=0; i<lhs.coordinates.positions.size(); ++i) {
            if(std::fabs(lhs.coordinates.positions[i].x - rhs.coordinates.positions[i].x) > std::numeric_limits<double>::epsilon() ||
               std::fabs(lhs.coordinates.positions[i].y - rhs.coordinates.positions[i].y) > std::numeric_limits<double>::epsilon() ||
               std::fabs(lhs.coordinates.positions[i].z - rhs.coordinates.positions[i].z) > std::numeric_limits<double>::epsilon() ||
               lhs.identifier != rhs.identifier) {
                return false;
            }
        }

        return true;
    }
};
}; // namespace std
