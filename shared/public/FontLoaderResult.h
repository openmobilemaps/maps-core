// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from loader.djinni

#pragma once

#include "FontData.h"
#include "LoaderStatus.h"
#include "TextureHolderInterface.h"
#include <memory>
#include <optional>
#include <utility>

struct FontLoaderResult final {
    /*nullable*/ std::shared_ptr<::TextureHolderInterface> imageData;
    std::optional<FontData> fontData;
    LoaderStatus status;

    FontLoaderResult(/*nullable*/ std::shared_ptr<::TextureHolderInterface> imageData_,
                     std::optional<FontData> fontData_,
                     LoaderStatus status_)
    : imageData(std::move(imageData_))
    , fontData(std::move(fontData_))
    , status(std::move(status_))
    {}
};
