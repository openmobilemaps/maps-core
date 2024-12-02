/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorSymbolFontProviderManager.h"

Tiled2dMapVectorSymbolFontProviderManager::Tiled2dMapVectorSymbolFontProviderManager(
        const std::shared_ptr<FontLoaderInterface> &fontLoader) : fontLoader(fontLoader) {}

std::shared_ptr <FontLoaderResult> Tiled2dMapVectorSymbolFontProviderManager::loadFont(const std::string &fontName) {
    if (fontLoaderResults.count(fontName) > 0) {
        return fontLoaderResults.at(fontName);
    } else if (fontLoader) {
        auto fontResult = std::make_shared<FontLoaderResult>(fontLoader->loadFont(Font(fontName)));
        if (fontResult->status == LoaderStatus::OK && fontResult->fontData && fontResult->imageData) {
            fontLoaderResults.insert({fontName, fontResult});
        }
        return fontResult;
    } else {
      return nullptr;
    }
}
