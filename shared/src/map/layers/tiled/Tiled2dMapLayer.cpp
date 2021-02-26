/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapLayer.h"
#include "MapCamera2dInterface.h"

Tiled2dMapLayer::Tiled2dMapLayer(const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig)
    : layerConfig(layerConfig) {}

void Tiled2dMapLayer::setSourceInterface(const std::shared_ptr<Tiled2dMapSourceInterface> &sourceInterface) {
    this->sourceInterface = sourceInterface;
}

void Tiled2dMapLayer::onAdded(const std::shared_ptr<::MapInterface> &mapInterface) {
    this->mapInterface = mapInterface;
    auto camera = std::dynamic_pointer_cast<MapCamera2dInterface>(mapInterface->getCamera());
    if (camera) {
        camera->addListener(shared_from_this());
        onVisibleBoundsChanged(camera->getVisibleRect(), camera->getZoom());
    }
}

void Tiled2dMapLayer::onRemoved() {
    auto camera = std::dynamic_pointer_cast<MapCamera2dInterface>(mapInterface->getCamera());
    if (camera) {
        camera->removeListener(shared_from_this());
    }
}

void Tiled2dMapLayer::hide() { isHidden = true; }

void Tiled2dMapLayer::show() { isHidden = false; }

void Tiled2dMapLayer::onVisibleBoundsChanged(const ::RectCoord &visibleBounds, double zoom) {
    sourceInterface->onVisibleBoundsChanged(visibleBounds, zoom);
}

void Tiled2dMapLayer::onRotationChanged(float angle) {
    // not used
}
