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

#include "Coord.h"
#include "CoordinateConversionHelper.h"
#include "CoordinateConverterInterface.h"
#include "CoordinateSystemIdentifiers.h"
#include "MapCoordinateSystem.h"

class DefaultSystemToRenderConverter : public CoordinateConverterInterface {
  public:
    DefaultSystemToRenderConverter(const MapCoordinateSystem &mapCoordinateSystem)
        : mapCoordinateSystemIdentifier(mapCoordinateSystem.identifier) {
        boundsLeft = mapCoordinateSystem.bounds.topLeft.x;
        boundsTop = mapCoordinateSystem.bounds.topLeft.y;
        boundsFar = mapCoordinateSystem.bounds.topLeft.z;
        boundsRight = mapCoordinateSystem.bounds.bottomRight.x;
        boundsBottom = mapCoordinateSystem.bounds.bottomRight.y;
        boundsNear = mapCoordinateSystem.bounds.bottomRight.z;
        halfWidth = 0.5 * (boundsRight - boundsLeft);
        halfHeight = 0.5 * (boundsBottom - boundsTop);
        halfDepth = 0.5 * (boundsNear - boundsFar);
    }

    virtual Coord convert(const Coord &coordinate) override {
        double x = (boundsRight < boundsLeft) ? -coordinate.x + boundsRight : (coordinate.x - boundsLeft);
        double y = (boundsBottom < boundsTop) ? -coordinate.y + boundsBottom : (coordinate.y - boundsTop);
        double z = (boundsNear < boundsFar) ? -coordinate.z + boundsNear : (coordinate.y - boundsFar);
        return Coord(getTo(), x - halfWidth, y - halfHeight, z - halfDepth);
    }

    virtual std::string getFrom() override { return mapCoordinateSystemIdentifier; }

    virtual std::string getTo() override { return CoordinateSystemIdentifiers::RENDERSYSTEM(); }

  private:
    double boundsLeft;
    double boundsTop;
    double boundsRight;
    double boundsBottom;
    double boundsNear;
    double boundsFar;
    double halfWidth;
    double halfHeight;
    double halfDepth;

    std::string mapCoordinateSystemIdentifier;
};
