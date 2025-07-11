// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from loader.djinni

#pragma once

#include "Vec2D.h"
#include <string>
#include <utility>

struct FontWrapper final {
    std::string name;
    double lineHeight;
    double base;
    ::Vec2D bitmapSize;
    /** font size rendered in distance field bitmap */
    double size;
    /** pixel distance corresponding to max value in signed distance field */
    double distanceRange;

    FontWrapper(std::string name_,
                double lineHeight_,
                double base_,
                ::Vec2D bitmapSize_,
                double size_,
                double distanceRange_)
    : name(std::move(name_))
    , lineHeight(std::move(lineHeight_))
    , base(std::move(base_))
    , bitmapSize(std::move(bitmapSize_))
    , size(std::move(size_))
    , distanceRange(std::move(distanceRange_))
    {}
};
