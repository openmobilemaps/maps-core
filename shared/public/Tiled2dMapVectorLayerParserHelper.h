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
#include "StringInterner.h"
#include "Tiled2dMapVectorLayerParserResult.h"
#include "Tiled2dMapVectorLayerLocalDataProviderInterface.h"

#include <string>

class Tiled2dMapVectorLayerParserHelper {
public:
    static Tiled2dMapVectorLayerParserResult parseStyleJsonFromUrl(const std::string &layerName,
                                                                   const std::string &styleJsonUrl,
                                                                   const std::shared_ptr<Tiled2dMapVectorLayerLocalDataProviderInterface> &localDataProvider,
                                                                   const std::vector<std::shared_ptr<::LoaderInterface>> &loaders, 
                                                                   StringInterner &stringTable,
                                                                   const std::unordered_map<std::string, std::string> & sourceUrlParams);

    static Tiled2dMapVectorLayerParserResult parseStyleJsonFromString(const std::string &layerName,
                                                                      const std::string &styleJsonString,
                                                                      const std::shared_ptr<Tiled2dMapVectorLayerLocalDataProviderInterface> &localDataProvider,
                                                                      const std::vector<std::shared_ptr<::LoaderInterface>> &loaders, 
                                                                      StringInterner &stringTable,
                                                                      const std::unordered_map<std::string, std::string> & sourceUrlParams);

    static std::string replaceUrlParams(const std::string & url, const std::unordered_map<std::string, std::string> & sourceUrlParams);
};
