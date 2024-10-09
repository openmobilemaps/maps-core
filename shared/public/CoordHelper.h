//
//  CoordHelper.h
//
//
//  Created by Stefan Mitterrutzner on 12.10.22.
//

#pragma once

#include "Coord.h"
#include <cmath>

class CoordHelper {
  public:
    // assumes the coordinates are in the same coordinate system, returns the distance in system units
    static float distance(const Coord &from, const Coord &to) { return std::sqrt((from.x - to.x) * (from.x - to.x) + (from.y - to.y) * (from.y - to.y)); }
};

inline Coord operator+( const ::Coord& left, const double delta) {
    return Coord(left.systemIdentifier ,left.x + delta, left.y + delta, left.z + delta);
}

inline Coord operator-( const ::Coord& left, const double delta) {
    return Coord(left.systemIdentifier ,left.x - delta, left.y - delta, left.z - delta);
}

inline Coord operator*( const ::Coord& left, const double factor) {
    return Coord(left.systemIdentifier ,left.x * factor, left.y * factor, left.z * factor);
}
