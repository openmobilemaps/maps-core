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
#include "RenderObject.h"
#include "MapCamera2dInterface.h"

Tiled2dMapVectorTile::Tiled2dMapVectorTile(const std::weak_ptr<MapInterface> &mapInterface,
                                           const Tiled2dMapTileInfo &tileInfo,
                                           const std::shared_ptr<VectorLayerDescription> &description,
                                           const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                           const WeakActor<Tiled2dMapVectorLayerTileCallbackInterface> &tileReadyInterface,
                                           const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager)
        : mapInterface(mapInterface), tileInfo(tileInfo), tileCallbackInterface(tileReadyInterface), description(description),
          layerConfig(layerConfig), featureStateManager(featureStateManager) {

    if (auto strongMapInterface = mapInterface.lock()) {
        dpFactor = strongMapInterface->getCamera()->getScreenDensityPpi() / 160.0;
    }
}

void Tiled2dMapVectorTile::updateVectorLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                                  const Tiled2dMapVectorTileDataVector &layerData) {
    this->description = description;
}

void Tiled2dMapVectorTile::updateRasterLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                                  const Tiled2dMapVectorTileDataRaster &layerData) {
    this->description = description;
}

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

void Tiled2dMapVectorTile::setSelectionDelegate(const std::weak_ptr<Tiled2dMapVectorLayerSelectionCallbackInterface> &selectionDelegate) {
    this->selectionDelegate = selectionDelegate;
}
