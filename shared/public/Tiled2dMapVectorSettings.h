// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from tiled_layer.djinni

#pragma once

#include "Tiled2dMapVectorTileOrigin.h"
#include <utility>

struct Tiled2dMapVectorSettings final {
    Tiled2dMapVectorTileOrigin tileOrigin;

    //NOLINTNEXTLINE(google-explicit-constructor)
    Tiled2dMapVectorSettings(Tiled2dMapVectorTileOrigin tileOrigin_)
    : tileOrigin(std::move(tileOrigin_))
    {}
};
