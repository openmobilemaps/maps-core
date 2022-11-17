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
        return DataLoaderResult(nullptr, std::nullopt, LoaderStatus::NOOP, std::nullopt);
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
};
