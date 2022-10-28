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

#include "PolygonMaskObject.h"

struct Tiled2dMapLayerMaskWrapper {
    std::shared_ptr<PolygonMaskObject> maskObject;
    size_t polygonHash;

    Tiled2dMapLayerMaskWrapper(std::shared_ptr<PolygonMaskObject> maskObject, size_t polygonHash): maskObject(maskObject), polygonHash(polygonHash){}

    Tiled2dMapLayerMaskWrapper(): maskObject(nullptr), polygonHash(0){}
};
