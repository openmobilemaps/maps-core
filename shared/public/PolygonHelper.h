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

#include "CoordinateConversionHelperInterface.h"
#include "PolygonInfo.h"
#include "PolygonCoord.h"
#include "gpc.h"

class PolygonHelper {
private:
  public:
    // clipping
    enum ClippingOperation { Intersection, Union, Difference, XOR };
    static PolygonCoord clip(const PolygonCoord &a, const PolygonCoord &b, const ClippingOperation operation);

    // Point inside
    static bool pointInside(const PolygonCoord &polygon, const Coord &point,
                            const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper);
    static bool pointInside(const PolygonInfo &polygon, const Coord &point,
                            const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper);
    static bool pointInside(const Coord &point, const std::vector<Coord> &positions, const std::vector<std::vector<Coord>> holes,
                            const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper);
    static bool pointInside(const Coord &point, const std::vector<Coord> &positions,
                            const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper);

private:
    static gpc_op gpcOperationFrom(const ClippingOperation operation);
};
