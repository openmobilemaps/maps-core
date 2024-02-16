/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "LoaderInterface.h"
#include "TextureLoaderResult.h"
#include "DataLoaderResult.h"
#include "Future.hpp"

class LoaderHelper {
public:
    static DataLoaderResult loadData(const std::string &url, const std::optional<std::string> &etag,
                                     const std::vector<std::shared_ptr<LoaderInterface>> &loaders) {
        for (auto loaderIterator = loaders.begin(); loaderIterator != loaders.end(); loaderIterator++) {
            auto result = (*loaderIterator)->loadData(url, etag);
            if (result.status != LoaderStatus::NOOP || loaderIterator == loaders.end()) {
                return std::move(result);
            }
        }
        return DataLoaderResult(std::nullopt, std::nullopt, LoaderStatus::NOOP, std::nullopt);
    }

    static TextureLoaderResult loadTexture(const std::string &url, const std::optional<std::string> &etag,
                                           const std::vector<std::shared_ptr<LoaderInterface>> &loaders) {
        for (auto loaderIterator = loaders.begin(); loaderIterator != loaders.end(); loaderIterator++) {
            auto result = (*loaderIterator)->loadTexture(url, etag);
            if (result.status != LoaderStatus::NOOP || loaderIterator == loaders.end()) {
                return std::move(result);
            }
        }
        return TextureLoaderResult(nullptr, std::nullopt, LoaderStatus::NOOP, std::nullopt);
    }

    static ::djinni::Future<::TextureLoaderResult> loadTextureAsync(const std::string &url, const std::optional<std::string> &etag,
                                           const std::vector<std::shared_ptr<LoaderInterface>> &loaders) {
        auto promise = std::make_shared<::djinni::Promise<::TextureLoaderResult>>();
        loadTextureAsyncInternal(url, etag, loaders, 0, promise);
        return promise->getFuture();
    }

    static ::djinni::Future<::DataLoaderResult> loadDataAsync(const std::string &url, const std::optional<std::string> &etag, const std::vector<std::shared_ptr<LoaderInterface>> &loaders) {
        auto promise = std::make_shared<::djinni::Promise<::DataLoaderResult>>();
        loadDataAsyncInternal(url, etag, loaders, 0, promise);
        return promise->getFuture();
    }

private:
    static void loadTextureAsyncInternal(const std::string &url, const std::optional<std::string> &etag, const std::vector<std::shared_ptr<LoaderInterface>> &loaders, size_t loaderIndex, const std::shared_ptr<::djinni::Promise<::TextureLoaderResult>> promise) {
        if (loaderIndex >= loaders.size()) {
            promise->setValue(std::move(TextureLoaderResult(nullptr, std::nullopt, LoaderStatus::NOOP, std::nullopt)));
        } else {
            loaders[loaderIndex]->loadTextureAsnyc(url, etag).then([url, etag, &loaders, loaderIndex, promise](::djinni::Future<::TextureLoaderResult> result) {
                const auto textureResult = result.get();
                if (textureResult.status != LoaderStatus::NOOP || loaderIndex == loaders.size() - 1) {
                    promise->setValue(std::move(textureResult));
                } else {
                    loadTextureAsyncInternal(url, etag, loaders, loaderIndex + 1, promise);
                }
            });
        }
    }

    static void loadDataAsyncInternal(const std::string &url, const std::optional<std::string> &etag, const std::vector<std::shared_ptr<LoaderInterface>> &loaders, size_t loaderIndex, const std::shared_ptr<::djinni::Promise<::DataLoaderResult>> promise) {
        if (loaderIndex >= loaders.size()) {
            promise->setValue(DataLoaderResult(std::nullopt, std::nullopt, LoaderStatus::NOOP, std::nullopt));
        } else {
            loaders[loaderIndex]->loadDataAsync(url, etag).then([url, etag, &loaders, loaderIndex, promise](::djinni::Future<::DataLoaderResult> result) {
                const auto dataResult = result.get();
                if (dataResult.status != LoaderStatus::NOOP || loaderIndex == loaders.size() - 1) {
                    promise->setValue(std::move(dataResult));
                } else {
                    loadDataAsyncInternal(url, etag, loaders, loaderIndex + 1, promise);
                }
            });
        }
    }
};
