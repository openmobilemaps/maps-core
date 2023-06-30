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

#include <optional>
#include "RectCoord.h"
#include "BoundingBoxInterface.h"

class BoundingBox: public BoundingBoxInterface, public std::enable_shared_from_this<BoundingBox>
{
  public:
    BoundingBox();

    BoundingBox(const int32_t systemIdentifier);
    BoundingBox(const Coord& p);

    void addPoint(const double x, const double y, const double z);
    void addPoint(const Coord& p);
    void addBox(const std::optional<BoundingBox>& box);

    RectCoord asRectCoord();

    Coord getCenter();
    Coord getMin();
    Coord getMax();
    int32_t getSystemIdentifier();

    Coord center() const;
    Coord min;
    Coord max;
    int32_t systemIdentifier;
};
