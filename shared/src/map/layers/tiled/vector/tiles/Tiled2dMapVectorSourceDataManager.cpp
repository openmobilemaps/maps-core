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
#include "Tiled2dMapVectorLayer.h"

Tiled2dMapVectorSourceDataManager::Tiled2dMapVectorSourceDataManager(const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                                                     const std::shared_ptr<VectorMapDescription> &mapDescription,
                                                                     const std::string &source)
        : vectorLayer(vectorLayer),
        mapDescription(mapDescription),
        source(source) {}

void Tiled2dMapVectorSourceDataManager::onAdded(const std::weak_ptr<::MapInterface> &mapInterface) {
    this->mapInterface = mapInterface;
}

void Tiled2dMapVectorSourceDataManager::onRemoved() {
    this->mapInterface = std::weak_ptr<MapInterface>();
}

void Tiled2dMapVectorSourceDataManager::setSelectionDelegate(const WeakActor<Tiled2dMapVectorLayerSelectionInterface> &selectionDelegate) {
    this->selectionDelegate = selectionDelegate;
}
