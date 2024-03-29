// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from tiled_vector_layer.djinni

#pragma once

#include "DataLoaderResult.h"
#include "Future.hpp"
#include "TextureLoaderResult.h"
#include <cstdint>
#include <optional>
#include <string>

class Tiled2dMapVectorLayerLocalDataProviderInterface {
public:
    virtual ~Tiled2dMapVectorLayerLocalDataProviderInterface() = default;

    virtual std::optional<std::string> getStyleJson() = 0;

    virtual ::djinni::Future<::TextureLoaderResult> loadSpriteAsync(int32_t scale) = 0;

    virtual ::djinni::Future<::DataLoaderResult> loadSpriteJsonAsync(int32_t scale) = 0;

    virtual ::djinni::Future<::DataLoaderResult> loadGeojson(const std::string & sourceName, const std::string & url) = 0;
};
