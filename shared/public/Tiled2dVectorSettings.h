// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from tiled_layer.djinni

#pragma once

#include "VectorTileOrigin.h"
#include <utility>

struct Tiled2dVectorSettings final {
    VectorTileOrigin tileOrigin;

    Tiled2dVectorSettings(VectorTileOrigin tileOrigin_)
    : tileOrigin(std::move(tileOrigin_))
    {}
};