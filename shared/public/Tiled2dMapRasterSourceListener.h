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

#include "Tiled2dMapRasterTileInfo.h"
#include "VectorSet.h"

class Tiled2dMapRasterSourceListener {
public:
    virtual void onTilesUpdated(const std::string &layerName, VectorSet<Tiled2dMapRasterTileInfo> currentTileInfos) = 0;
};
