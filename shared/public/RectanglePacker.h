// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from packer.djinni

#pragma once

#include "Vec2I.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct RectanglePackerPage;

class RectanglePacker {
public:
    virtual ~RectanglePacker() = default;

    static std::vector<RectanglePackerPage> pack(const std::unordered_map<std::string, ::Vec2I> & rectangles, const ::Vec2I & maxPageSize, int32_t spacing);
};
