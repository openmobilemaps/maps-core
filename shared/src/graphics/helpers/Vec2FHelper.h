#pragma once
#include "Vec2F.h"

class Vec2FHelper {
public:
    static double distance(const ::Vec2F &from, const ::Vec2F &to);
    
    static ::Vec2F midpoint(const ::Vec2F &from, const ::Vec2F &to);
};
