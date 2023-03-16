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

#include "Tiled2dMapTileInfo.h"

class Tiled2dMapVectorLayerReadyInterface {
public:
    virtual void tileIsReady(const Tiled2dMapTileInfo &tile, const std::string &layerIdentifier,
                             const std::vector<std::shared_ptr<RenderObjectInterface>> renderObjects) = 0;
};
