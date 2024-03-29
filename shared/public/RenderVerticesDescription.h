// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from graphicsobjects.djinni

#pragma once

#include "Vec2D.h"
#include <cstdint>
#include <utility>
#include <vector>

struct RenderVerticesDescription final {
    std::vector<::Vec2D> vertices;
    int32_t styleIndex;

    RenderVerticesDescription(std::vector<::Vec2D> vertices_,
                              int32_t styleIndex_)
    : vertices(std::move(vertices_))
    , styleIndex(std::move(styleIndex_))
    {}
};
