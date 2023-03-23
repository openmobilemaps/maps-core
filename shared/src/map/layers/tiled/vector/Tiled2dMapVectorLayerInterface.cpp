/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

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
Tiled2dMapVectorLayerInterface::createFromLocalStyleJson(const std::string & layerName, const std::string & styleJson, const std::vector</*not-null*/ std::shared_ptr<::LoaderInterface>> & loaders, const /*not-null*/ std::shared_ptr<::FontLoaderInterface> & fontLoader, double dpFactor) {

    auto res = Tiled2dMapVectorLayerParserHelper::parseStyleJsonFromString(layerName, styleJson, dpFactor, loaders);

    if(res.status != LoaderStatus::OK) {
        return nullptr;
    }

    return std::make_shared<Tiled2dMapVectorLayer>(layerName, res.mapDescription, loaders, fontLoader);
}
