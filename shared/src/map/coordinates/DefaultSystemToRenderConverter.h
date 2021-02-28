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
        boundsRight = mapCoordinateSystem.bounds.bottomRight.x;
        boundsBottom = mapCoordinateSystem.bounds.bottomRight.y;
        halfWidth = 0.5 * (boundsRight - boundsLeft);
        halfHeight = 0.5 * (boundsBottom - boundsTop);
    }

    virtual Coord convert(const Coord &coordinate) override {
        double x = (boundsRight < boundsLeft) ? -coordinate.x + boundsRight : (coordinate.x - boundsLeft);
        double y = (boundsBottom < boundsTop) ? -coordinate.y + boundsBottom : (coordinate.y - boundsTop);
        return Coord(getTo(), x - halfWidth, y - halfHeight, coordinate.z);
    }

    virtual std::string getFrom() override { return mapCoordinateSystemIdentifier; }

    virtual std::string getTo() override { return CoordinateSystemIdentifiers::RENDERSYSTEM(); }

  private:
    double boundsLeft;
    double boundsTop;
    double boundsRight;
    double boundsBottom;
    double halfWidth;
    double halfHeight;

    std::string mapCoordinateSystemIdentifier;
};
