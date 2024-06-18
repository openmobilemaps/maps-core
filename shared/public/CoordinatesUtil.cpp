/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "CoordinatesUtil.h"

Coord operator-(const Coord &c1, const Coord &c2) {
    assert(c1.systemIdentifier == c2.systemIdentifier);
    return Coord(c1.systemIdentifier, c1.x - c2.x,  c1.y - c2.y,  c1.z - c2.z);
}