#pragma once

#include "PolygonInfo.h"

namespace std {
template <> struct hash<PolygonInfo> {
    inline size_t operator()(const PolygonInfo &obj) const { return std::hash<std::string>{}(obj.identifier); }
};
template <> struct equal_to<PolygonInfo> {
    inline bool operator()(const PolygonInfo &lhs, const PolygonInfo &rhs) const { return lhs.identifier == rhs.identifier; }
};
}; // namespace std
