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

#include "Tiled2dMapVectorTileInfo.h"
#include <unordered_set>

class Tiled2dMapVectorSourceListener {
public:
    virtual void onTilesUpdated(const std::string &sourceName, VectorSet<Tiled2dMapVectorTileInfo> currentTileInfos) = 0;
};
