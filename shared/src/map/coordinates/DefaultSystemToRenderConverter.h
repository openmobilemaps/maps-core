//
// Created by Christoph Maurhofer on 05.02.2021.
//

#pragma once

#include "CoordinateConverterInterface.h"
#include "MapCoordinateSystem.h"
#include "Coord.h"
#include "CoordinateConversionHelper.h"

class DefaultSystemToRenderConverter : public CoordinateConverterInterface {
public:
    DefaultSystemToRenderConverter(const MapCoordinateSystem &mapCoordinateSystem) {
        boundsLeft = mapCoordinateSystem.boundsLeft;
        boundsTop = mapCoordinateSystem.boundsTop;
        boundsRight = mapCoordinateSystem.boundsRight;
        boundsBottom = mapCoordinateSystem.boundsBottom;
        halfWidth = 0.5 * (boundsRight - boundsLeft);
        halfHeight = 0.5 * (boundsBottom - boundsTop);
    }

    Coord convert(const Coord &coordinate) {
        double x = (boundsRight < boundsLeft) ? -coordinate.x + boundsRight : (coordinate.x - boundsLeft);
        double y = (boundsBottom < boundsTop) ? -coordinate.y + boundsBottom : (coordinate.y - boundsTop);
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