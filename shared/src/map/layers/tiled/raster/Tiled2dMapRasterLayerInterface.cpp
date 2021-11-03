/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapRasterLayerInterface.h"
#include "Tiled2dMapRasterLayer.h"

std::shared_ptr<Tiled2dMapRasterLayerInterface>
Tiled2dMapRasterLayerInterface::create(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                       const std::shared_ptr<::LoaderInterface> & tileLoader) {
    return std::make_shared<Tiled2dMapRasterLayer>(layerConfig, tileLoader);
}


std::shared_ptr<Tiled2dMapRasterLayerInterface> Tiled2dMapRasterLayerInterface::createWithMask(const std::shared_ptr<::Tiled2dMapLayerConfig> & layerConfig, const std::shared_ptr<::LoaderInterface> & tileLoader, const std::shared_ptr<::MaskingObjectInterface> & mask){
    return std::make_shared<Tiled2dMapRasterLayer>(layerConfig, tileLoader, mask);
}
