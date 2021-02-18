#include "Vec2FHelper.h"
#include <cmath>

double Vec2FHelper::distance(const ::Vec2F &from, const ::Vec2F &to) {
    return std::sqrt((from.x - to.x) * (from.x - to.x) + (from.y - to.y) * (from.y - to.y));
}

::Vec2F Vec2FHelper::midpoint(const ::Vec2F &from, const ::Vec2F &to) {
    return Vec2F((from.x + to.x) / 2.0, (from.y + to.y) / 2.0);
}