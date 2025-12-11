#pragma once

#include "DataLoaderResult.h"
#include "FontLoader.h"
#include "FontLoaderResult.h" // Added to ensure FontLoaderResult is fully defined

struct DataLoadRequest {
    DataLoadRequest(const std::string &url, const std::optional<std::string> &etag)
        : url(url)
        , etag(etag) {}

    std::string url;
    std::optional<std::string> etag;
    djinni::Promise<DataLoaderResult> promise;
};
struct TextureLoadRequest {
    TextureLoadRequest(const std::string &url, const std::optional<std::string> &etag)
        : url(url)
        , etag(etag) {}

    std::string url;
    std::optional<std::string> etag;
    djinni::Promise<TextureLoaderResult> promise;
};