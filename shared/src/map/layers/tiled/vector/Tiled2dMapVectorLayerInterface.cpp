/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorLayerInterface.h"
#include "Tiled2dMapVectorLayer.h"

std::shared_ptr<Tiled2dMapVectorLayerInterface>
Tiled2dMapVectorLayerInterface::create(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                       const std::shared_ptr<::TileLoaderInterface> &tileLoader) {
    return std::make_shared<Tiled2dMapVectorLayer>(layerConfig, tileLoader);
}
