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

#include "Actor.h"
#include "Tiled2dMapVectorFontProvider.h"
#include "FontLoaderResult.h"
#include "FontLoaderInterface.h"
#include <unordered_map>

class Tiled2dMapVectorSymbolFontProviderManager : public ActorObject, public Tiled2dMapVectorFontProvider {
public:
    Tiled2dMapVectorSymbolFontProviderManager(const std::shared_ptr<FontLoaderInterface> &fontLoader);

    virtual std::shared_ptr<FontLoaderResult> loadFont(const std::string &fontName);

private:
    std::shared_ptr<FontLoaderInterface> fontLoader;
    std::unordered_map<std::string, std::shared_ptr<FontLoaderResult>> fontLoaderResults;
};
