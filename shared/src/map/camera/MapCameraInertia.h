/*
 * Copyright (c) 2026 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#ifndef MAPSCORE__MapCameraInertia__MapCameraInertia__
#define MAPSCORE__MapCameraInertia__MapCameraInertia__

#include <cstdint>

#include "Vec2F.h"

/// object describing parameters of inertia
/// currently only dragging inertia is implemented
/// zoom and rotation are still missing
struct MapCameraInertia {
    static constexpr int64_t minimumInertiaDeltaMicroSeconds = 8000;

    int64_t timestampStart;
    int64_t timestampUpdate;
    Vec2F velocity;
    double t1;
    double t2;

    MapCameraInertia(int64_t timestampStart, Vec2F velocity, double t1, double t2)
        : timestampStart(timestampStart)
        , timestampUpdate(timestampStart)
        , velocity(velocity)
        , t1(t1)
        , t2(t2) {}
};

#endif // MAPSCORE__MapCameraInertia__MapCameraInertia__
