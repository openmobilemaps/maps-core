// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from core.djinni

#pragma once

#include "MapCoordinateSystem.h"
#include <utility>

struct MapConfig final {
    ::MapCoordinateSystem mapCoordinateSystem;

    //NOLINTNEXTLINE(google-explicit-constructor)
    MapConfig(::MapCoordinateSystem mapCoordinateSystem_)
    : mapCoordinateSystem(std::move(mapCoordinateSystem_))
    {}
};
