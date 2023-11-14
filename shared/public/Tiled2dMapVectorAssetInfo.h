// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from tiled_vector_layer.djinni

#pragma once

#include "RectI.h"
#include "TextureHolderInterface.h"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

struct Tiled2dMapVectorAssetInfo final {
    std::unordered_map<std::string, ::RectI> featureIdentifiersUv;
    /*nullable*/ std::shared_ptr<::TextureHolderInterface> texture;

    Tiled2dMapVectorAssetInfo(std::unordered_map<std::string, ::RectI> featureIdentifiersUv_,
                              /*nullable*/ std::shared_ptr<::TextureHolderInterface> texture_)
    : featureIdentifiersUv(std::move(featureIdentifiersUv_))
    , texture(std::move(texture_))
    {}
};