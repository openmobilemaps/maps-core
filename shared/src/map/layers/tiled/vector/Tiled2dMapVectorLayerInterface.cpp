/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include <Tiled2dMapVectorLayerInterface.h>

#include "Tiled2dMapVectorLayer.h"

#include "Tiled2dMapVectorLayerParserHelper.h"

std::shared_ptr <Tiled2dMapVectorLayerInterface>
Tiled2dMapVectorLayerInterface::createFromStyleJson(const std::string &layerName,
                                                    const std::string &path,
                                                    const std::vector <std::shared_ptr<::LoaderInterface>> &loaders,
                                                    const std::shared_ptr<::FontLoaderInterface> &fontLoader) {
    return createExplicitly(layerName, path, std::nullopt, loaders, fontLoader, nullptr, std::nullopt, nullptr, std::nullopt);
}


std::shared_ptr<Tiled2dMapVectorLayerInterface>
Tiled2dMapVectorLayerInterface::createExplicitly(const std::string &layerName,
                                                 const std::optional<std::string> &styleJson,
                                                 std::optional<bool> localStyleJson,
                                                 const std::vector<std::shared_ptr<::LoaderInterface>> &loaders,
                                                 const std::shared_ptr<::FontLoaderInterface> &fontLoader,
                                                 const std::shared_ptr<Tiled2dMapVectorLayerLocalDataProviderInterface> &localDataProvider,
                                                 const std::optional<::Tiled2dMapZoomInfo> &customZoomInfo,
                                                 const std::shared_ptr<Tiled2dMapVectorLayerSymbolDelegateInterface> &symbolDelegate, const std::optional<std::unordered_map<std::string, std::string>> & sourceUrlParams) {

    const auto urlParams = sourceUrlParams.value_or(std::unordered_map<std::string, std::string>());

    if ((localStyleJson.has_value() && *localStyleJson) || localDataProvider) {
        std::optional<Tiled2dMapVectorLayerParserResult> parserResult = std::nullopt;
        StringInterner stringTable = ValueKeys::newStringInterner();

        if (localDataProvider) {
            auto localProvidedStyleJson = localDataProvider->getStyleJson();
            if (localProvidedStyleJson) {
                parserResult = Tiled2dMapVectorLayerParserHelper::parseStyleJsonFromString(layerName, *localProvidedStyleJson, localDataProvider, loaders, stringTable, urlParams);
            }
        }

        if (!(localStyleJson.has_value() && *localStyleJson) && !parserResult) {
            return std::make_shared<Tiled2dMapVectorLayer>(layerName, *styleJson, loaders, fontLoader, customZoomInfo, symbolDelegate, urlParams, localDataProvider);
        }

        if (!parserResult && styleJson.has_value()) {
            parserResult = Tiled2dMapVectorLayerParserHelper::parseStyleJsonFromString(layerName, *styleJson, nullptr, loaders, stringTable, urlParams);
        }

        if (parserResult.has_value() && parserResult->status == LoaderStatus::OK) {
            return std::make_shared<Tiled2dMapVectorLayer>(layerName, parserResult->mapDescription, std::move(stringTable), loaders, fontLoader, customZoomInfo, symbolDelegate, localDataProvider, urlParams);
        } else {
            return nullptr;
        }
    }

    if (styleJson.has_value()) {
        return std::make_shared<Tiled2dMapVectorLayer>(layerName, *styleJson, loaders, fontLoader, customZoomInfo, symbolDelegate, urlParams);
    }

    return nullptr;
}
