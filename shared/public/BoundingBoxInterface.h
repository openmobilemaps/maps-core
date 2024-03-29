// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from coordinate_system.djinni

#pragma once

#include <cstdint>
#include <memory>

struct Coord;
struct RectCoord;

class BoundingBoxInterface {
public:
    virtual ~BoundingBoxInterface() = default;

    static /*not-null*/ std::shared_ptr<BoundingBoxInterface> create(int32_t systemIdentifier);

    virtual void addPoint(const Coord & p) = 0;

    virtual RectCoord asRectCoord() = 0;

    virtual Coord getCenter() = 0;

    virtual Coord getMin() = 0;

    virtual Coord getMax() = 0;

    virtual int32_t getSystemIdentifier() = 0;
};
