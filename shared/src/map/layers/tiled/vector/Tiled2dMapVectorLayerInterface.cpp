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

std::shared_ptr <Tiled2dMapVectorLayerInterface>
Tiled2dMapVectorLayerInterface::createAnimatableFromStyleJson(const std::string &layerName,
                                                    const std::string &path,
                                                    const std::vector <std::shared_ptr<::LoaderInterface>> &loaders,
                                                    const std::shared_ptr<::FontLoaderInterface> &fontLoader,
                                                    double dpFactor,
                                                    int64_t numT) {
    return std::make_shared<Tiled2dMapVectorLayer>(layerName, path, loaders, fontLoader, dpFactor, numT);
}

std::shared_ptr <Tiled2dMapVectorLayerInterface>
Tiled2dMapVectorLayerInterface::createFromStyleJson(const std::string &layerName,
                                                    const std::string &path,
                                                    const std::vector <std::shared_ptr<::LoaderInterface>> &loaders,
                                                    const std::shared_ptr<::FontLoaderInterface> &fontLoader,
                                                    double dpFactor) {
    return createAnimatableFromStyleJson(layerName, path, loaders, fontLoader, dpFactor, 1);
}
