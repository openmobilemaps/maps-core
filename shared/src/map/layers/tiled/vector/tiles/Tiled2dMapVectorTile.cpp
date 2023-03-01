/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorTile.h"
#include "CoordinateSystemIdentifiers.h"
#include "QuadCoord.h"
#include "RenderPass.h"
#include "RenderObject.h"

Tiled2dMapVectorTile::Tiled2dMapVectorTile(const std::weak_ptr<MapInterface> &mapInterface, const Tiled2dMapTileInfo &tileInfo,
                                           const WeakActor<Tiled2dMapVectorLayer> &vectorLayer)
        : mapInterface(mapInterface), tileInfo(tileInfo), vectorLayer(vectorLayer) {}

void Tiled2dMapVectorTile::setAlpha(float alpha) {
    if (this->alpha == alpha) {
        return;
    }

    this->alpha = alpha;
    auto mapInterface = this->mapInterface.lock();
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

float Tiled2dMapVectorTile::getAlpha() {
    return alpha;
}