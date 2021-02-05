//
// Created by Christoph Maurhofer on 05.02.2021.
//

#pragma once

#include "CoordinateConverterInterface.h"
#include "MapCoordinateSystem.h"
#include "Coord.h"
#include "CoordinateConversionHelper.h"
#include <cmath>

class DefaultSystemToRenderConverter : public CoordinateConverterInterface {
public:
    DefaultSystemToRenderConverter(const MapCoordinateSystem &mapCoordinateSystem) {
        boundsLeft = mapCoordinateSystem.boundsLeft;
        boundsTop = mapCoordinateSystem.boundsTop;
        boundsRight = mapCoordinateSystem.boundsRight;
        boundsBottom = mapCoordinateSystem.boundsBottom;
        halfWidth = 0.5 * std::abs(boundsRight - boundsLeft);
        halfHeight = 0.5 * std::abs(boundsBottom - boundsTop);
    }

    Coord convert(const Coord &coordinate) {
        double x = (boundsRight < boundsLeft) ? boundsLeft - coordinate.x :  x;
        double y = (boundsBottom < boundsTop) ? boundsTop - coordinate.y : y;
        return Coord(CoordinateConversionHelper::RENDER_SYSTEM_ID, x - halfWidth, y - halfHeight, coordinate.z);
    }

private:
    double boundsLeft;
    double boundsTop;
    double boundsRight;
    double boundsBottom;
    double halfWidth;
    double halfHeight;
};