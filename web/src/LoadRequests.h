#pragma once

#include "DataLoaderResult.h"
#include "TextureLoaderResult.h"

#include "Future.hpp"

#include <string>
#include <optional>

class WebLoader;

struct DataLoadRequest {
    DataLoadRequest(const std::string &url, const std::optional<std::string> &etag)
        : url(url)
        , etag(etag) {}

    std::string url;
    std::optional<std::string> etag;
    djinni::Promise<DataLoaderResult> promise;
    std::weak_ptr<WebLoader> loader; // link back to loader, for bookeeping in fetch callbacks
};

struct TextureLoadRequest {
    TextureLoadRequest(const std::string &url, const std::optional<std::string> &etag)
        : url(url)
        , etag(etag) {}

    std::string url;
    std::optional<std::string> etag;
    djinni::Promise<TextureLoaderResult> promise;
};
