// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from tiled_layer.djinni

#pragma once

#include "RectCoord.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct Tiled2dMapVectorSettings;
struct Tiled2dMapZoomInfo;
struct Tiled2dMapZoomLevelInfo;

class Tiled2dMapLayerConfig {
public:
    virtual ~Tiled2dMapLayerConfig() = default;

    virtual int32_t getCoordinateSystemIdentifier() = 0;

    virtual std::string getTileUrl(int32_t x, int32_t y, int32_t t, int32_t zoom) = 0;

    virtual std::vector<Tiled2dMapZoomLevelInfo> getZoomLevelInfos() = 0;

    virtual std::vector<Tiled2dMapZoomLevelInfo> getVirtualZoomLevelInfos() = 0;

    virtual Tiled2dMapZoomInfo getZoomInfo() = 0;

    virtual std::string getLayerName() = 0;

    virtual std::optional<Tiled2dMapVectorSettings> getVectorSettings() = 0;

    virtual std::optional<::RectCoord> getBounds() = 0;
};
