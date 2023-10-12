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
                                                    const std::shared_ptr<::FontLoaderInterface> &fontLoader,
                                                    double dpFactor) {
    return std::make_shared<Tiled2dMapVectorLayer>(layerName, path, loaders, fontLoader, dpFactor);
}

std::shared_ptr <Tiled2dMapVectorLayerInterface>
Tiled2dMapVectorLayerInterface::createFromStyleJsonWithUrlParams(const std::string &layerName,
                                                    const std::string &path,
                                                    const std::vector <std::shared_ptr<::LoaderInterface>> &loaders,
                                                    const std::shared_ptr<::FontLoaderInterface> &fontLoader,
                                                    double dpFactor, const std::unordered_map<std::string, std::string> & sourceUrlParams) {
    return std::make_shared<Tiled2dMapVectorLayer>(layerName, path, loaders, fontLoader, dpFactor, std::nullopt, sourceUrlParams);
}

std::shared_ptr<Tiled2dMapVectorLayerInterface>
Tiled2dMapVectorLayerInterface::createFromStyleJsonWithZoomInfo(const std::string &layerName, const std::string &path,
                                                                const std::vector</*not-null*/ std::shared_ptr<::LoaderInterface>> &loaders,
                                                                const /*not-null*/ std::shared_ptr<::FontLoaderInterface> &fontLoader,
                                                                double dpFactor, const ::Tiled2dMapZoomInfo &zoomInfo) {
    return std::make_shared<Tiled2dMapVectorLayer>(layerName, path, loaders, fontLoader, dpFactor, zoomInfo);
}

std::shared_ptr<Tiled2dMapVectorLayerInterface>
Tiled2dMapVectorLayerInterface::createFromStyleJsonWithZoomInfoUrlParams(const std::string &layerName, const std::string &path,
                                                                const std::vector</*not-null*/ std::shared_ptr<::LoaderInterface>> &loaders,
                                                                const /*not-null*/ std::shared_ptr<::FontLoaderInterface> &fontLoader,
                                                                double dpFactor, const ::Tiled2dMapZoomInfo &zoomInfo, const std::unordered_map<std::string, std::string> & sourceUrlParams) {
    return std::make_shared<Tiled2dMapVectorLayer>(layerName, path, loaders, fontLoader, dpFactor, zoomInfo, sourceUrlParams);
}

std::shared_ptr <Tiled2dMapVectorLayerInterface>
Tiled2dMapVectorLayerInterface::createFromLocalStyleJson(const std::string & layerName, const std::string & styleJson, const std::vector</*not-null*/ std::shared_ptr<::LoaderInterface>> & loaders, const /*not-null*/ std::shared_ptr<::FontLoaderInterface> & fontLoader, double dpFactor) {
    return createFromLocalStyleJsonWithUrlParams(layerName, styleJson, loaders, fontLoader, dpFactor, {});
}

std::shared_ptr <Tiled2dMapVectorLayerInterface>
Tiled2dMapVectorLayerInterface::createFromLocalStyleJsonWithUrlParams(const std::string & layerName, const std::string & styleJson, const std::vector</*not-null*/ std::shared_ptr<::LoaderInterface>> & loaders, const /*not-null*/ std::shared_ptr<::FontLoaderInterface> & fontLoader, double dpFactor,
                                                                      const std::unordered_map<std::string, std::string> & sourceUrlParams) {

    auto res = Tiled2dMapVectorLayerParserHelper::parseStyleJsonFromString(layerName, styleJson, dpFactor, loaders, sourceUrlParams);

    if(res.status != LoaderStatus::OK) {
        return nullptr;
    }

    return std::make_shared<Tiled2dMapVectorLayer>(layerName, res.mapDescription, loaders, fontLoader);
}

std::shared_ptr<Tiled2dMapVectorLayerInterface>
Tiled2dMapVectorLayerInterface::createFromLocalStyleJsonWithZoomInfo(const std::string &layerName, const std::string &styleJson,
const std::vector<std::shared_ptr<::LoaderInterface>> &loaders,
const std::shared_ptr<::FontLoaderInterface> &fontLoader,
double dpFactor, const Tiled2dMapZoomInfo &zoomInfo) {
    return createFromLocalStyleJsonWithZoomInfoUrlParams(layerName, styleJson, loaders, fontLoader, dpFactor, zoomInfo, {});
}

std::shared_ptr<Tiled2dMapVectorLayerInterface>
Tiled2dMapVectorLayerInterface::createFromLocalStyleJsonWithZoomInfoUrlParams(const std::string &layerName, const std::string &styleJson,
                                                                     const std::vector<std::shared_ptr<::LoaderInterface>> &loaders,
                                                                     const std::shared_ptr<::FontLoaderInterface> &fontLoader,
                                                                     double dpFactor, const Tiled2dMapZoomInfo &zoomInfo,
                                                                              const std::unordered_map<std::string, std::string> & sourceUrlParams) {
    auto res = Tiled2dMapVectorLayerParserHelper::parseStyleJsonFromString(layerName, styleJson, dpFactor, loaders, sourceUrlParams);

    if(res.status != LoaderStatus::OK) {
        return nullptr;
    }

    return std::make_shared<Tiled2dMapVectorLayer>(layerName, res.mapDescription, loaders, fontLoader, zoomInfo);}
