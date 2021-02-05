#include "Vec2FHelper.h"
#include <cmath>

double Vec2FHelper::distance(const ::Vec2F &from, const ::Vec2F &to) {
    return std::sqrt((from.x - to.x) * (from.x - to.x) +
                (from.y - to.y) * (from.y - to.y));
}
