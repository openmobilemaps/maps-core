/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "WmtsTiled2dMapLayerConfigFactory.h"
#include "WmtsTiled2dMapLayerConfig.h"

std::shared_ptr<::Tiled2dMapLayerConfig> WmtsTiled2dMapLayerConfigFactory::create(const WmtsLayerDescription & wmtsLayerConfiguration, const std::vector<::Tiled2dMapZoomLevelInfo> & zoomLevelInfo, const ::Tiled2dMapZoomInfo & zoomInfo) {
    return std::make_shared<WmtsTiled2dMapLayerConfig>(wmtsLayerConfiguration, zoomLevelInfo, zoomInfo);
}
