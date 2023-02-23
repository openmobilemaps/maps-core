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

Tiled2dMapVectorTile::Tiled2dMapVectorTile(const Tiled2dMapTileInfo &tileInfo, const WeakActor<Tiled2dMapVectorLayer> &vectorLayer): tileInfo(tileInfo), vectorLayer(vectorLayer) {}
