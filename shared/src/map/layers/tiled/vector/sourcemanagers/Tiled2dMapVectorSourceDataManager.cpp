/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorSourceDataManager.h"
#include "Tiled2dMapVectorRasterTile.h"
#include "Tiled2dMapVectorPolygonTile.h"
#include "Tiled2dMapVectorLineTile.h"
#include "Tiled2dMapVectorLayer.h"
#include "RenderPass.h"

Tiled2dMapVectorSourceDataManager::Tiled2dMapVectorSourceDataManager(const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                                                     const std::shared_ptr<VectorMapDescription> &mapDescription,
                                                                     const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                                                     const std::string &source,
                                                                     const Actor<Tiled2dMapVectorReadyManager> &readyManager,
                                                                     const std::shared_ptr<Tiled2dMapVectorFeatureStateManager> &featureStateManager)
        : vectorLayer(vectorLayer),
          mapDescription(mapDescription),
          layerConfig(layerConfig),
          source(source),
          readyManager(readyManager),
          featureStateManager(featureStateManager) {
    for (int32_t index = 0; index < mapDescription->layers.size(); index++) {
        const auto &layerDescription = mapDescription->layers.at(index);
        if (layerDescription->source == source) {
            layerNameIndexMap[layerDescription->identifier] = index;
            if (layerDescription->getType() == VectorLayerType::line) {
                modifyingMaskLayers.insert(index);
            }
        }
    }

    readyManagerIndex = readyManager.converse(&Tiled2dMapVectorReadyManager::registerManager).get();
}

void Tiled2dMapVectorSourceDataManager::onAdded(const std::weak_ptr<::MapInterface> &mapInterface) {
    this->mapInterface = mapInterface;
}

void Tiled2dMapVectorSourceDataManager::onRemoved() {
    this->mapInterface = std::weak_ptr<MapInterface>();
}

void Tiled2dMapVectorSourceDataManager::setSelectionDelegate(const std::weak_ptr<Tiled2dMapVectorLayerSelectionCallbackInterface> &selectionDelegate) {
    this->selectionDelegate = selectionDelegate;
}
