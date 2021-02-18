#pragma once

#include "Polygon.h"

namespace std {
template<>
struct hash<Polygon> {
    inline size_t operator()(const Polygon& obj) const {
        return std::hash<std::string>{}(obj.identifier);
    }
};
template<>
struct equal_to<Polygon> {
    inline bool operator()(const Polygon &lhs, const Polygon &rhs) const
    {
        return lhs.identifier == rhs.identifier;
    }
};
};
