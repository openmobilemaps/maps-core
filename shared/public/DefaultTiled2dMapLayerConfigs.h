// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from tiled_layer.djinni

#pragma once

#include <memory>
#include <string>

class Tiled2dMapLayerConfig;

class DefaultTiled2dMapLayerConfigs {
public:
    virtual ~DefaultTiled2dMapLayerConfigs() = default;

    static /*not-null*/ std::shared_ptr<Tiled2dMapLayerConfig> webMercator(const std::string & layerName, const std::string & urlFormat);
};
