//
//  CoordHelper.h
//
//
//  Created by Stefan Mitterrutzner on 12.10.22.
//

#pragma once

#include "Coord.h"
#include <cassert>
#include <cmath>

class CoordHelper {
  public:
    // assumes the coordinates are in the same coordinate system, returns the distance in system units
    static double distance(const Coord &from, const Coord &to) {
        return std::sqrt(distanceSquared(from, to));
    }
    
    // assumes the coordinates are in the same coordinate system, returns the distance in system units
    static double distanceSquared(const Coord &from, const Coord &to) { 
        assert(from.systemIdentifier == to.systemIdentifier);
        return (from.x - to.x) * (from.x - to.x) + (from.y - to.y) * (from.y - to.y); 
    }
    
    static Coord interpolate(const Coord& start, const Coord& end, double t) {
        assert(start.systemIdentifier == start.systemIdentifier);
        return Coord(start.systemIdentifier,
                     start.x + (end.x - start.x) * t,
                     start.y + (end.y - start.y) * t,
                     start.z + (end.z - start.z) * t);
    }
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
