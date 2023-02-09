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

Tiled2dMapLayer::Tiled2dMapLayer()
    : curT(0) {}

void Tiled2dMapLayer::setSourceInterface(const WeakActor<Tiled2dMapSourceInterface> &sourceInterface) {
    this->sourceInterface = sourceInterface;
    if (isHidden) {
        sourceInterface.message(&Tiled2dMapSourceInterface::pause);
    }
    auto errorManager = this->errorManager;
    if (errorManager) {
        sourceInterface.message(&Tiled2dMapSourceInterface::setErrorManager, errorManager);
    }
}

void Tiled2dMapLayer::onAdded(const std::shared_ptr<::MapInterface> &mapInterface, int32_t layerIndex) {
    this->mapInterface = mapInterface;

    sourceInterface.message(&Tiled2dMapSourceInterface::setMinZoomLevelIdentifier, minZoomLevelIdentifier);
    sourceInterface.message(&Tiled2dMapSourceInterface::setMaxZoomLevelIdentifier, maxZoomLevelIdentifier);

    auto camera = std::dynamic_pointer_cast<MapCamera2dInterface>(mapInterface->getCamera());
    if (camera) {
        camera->addListener(shared_from_this());
        onVisibleBoundsChanged(camera->getVisibleRect(), camera->getZoom());
    }
}

void Tiled2dMapLayer::onRemoved() {
    if (mapInterface) {
        auto camera = std::dynamic_pointer_cast<MapCamera2dInterface>(mapInterface->getCamera());
        if (camera) {
            camera->removeListener(shared_from_this());
        }
    }
    mapInterface = nullptr;
}

void Tiled2dMapLayer::pause() {
    sourceInterface.message(&Tiled2dMapSourceInterface::pause);
}

void Tiled2dMapLayer::resume() {
    if (!isHidden) {
        sourceInterface.message(&Tiled2dMapSourceInterface::resume);
    }
}

void Tiled2dMapLayer::hide() {
    isHidden = true;
    if (sourceInterface) {
        sourceInterface.message(&Tiled2dMapSourceInterface::pause);
    }
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void Tiled2dMapLayer::show() {
    isHidden = false;
    if (sourceInterface) {
        sourceInterface.message(&Tiled2dMapSourceInterface::resume);
    }
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void Tiled2dMapLayer::onVisibleBoundsChanged(const ::RectCoord &visibleBounds, double zoom) {
    sourceInterface.message(&Tiled2dMapSourceInterface::onVisibleBoundsChanged, visibleBounds, curT, zoom);
}

void Tiled2dMapLayer::onRotationChanged(float angle) {
    // not used
}

void Tiled2dMapLayer::onMapInteraction() {
    // not used
}

void Tiled2dMapLayer::setMaskingObject(const std::shared_ptr<::MaskingObjectInterface> &maskingObject) {}

void Tiled2dMapLayer::setMinZoomLevelIdentifier(std::optional<int32_t> value) {
    minZoomLevelIdentifier = value;
    if (sourceInterface)
        sourceInterface.message(&Tiled2dMapSourceInterface::setMinZoomLevelIdentifier, value);
}

std::optional<int32_t> Tiled2dMapLayer::getMinZoomLevelIdentifier() {
    if (sourceInterface)
        //return sourceInterface->getMinZoomLevelIdentifier();
    return std::nullopt;
}

void Tiled2dMapLayer::setMaxZoomLevelIdentifier(std::optional<int32_t> value) {
    maxZoomLevelIdentifier = value;
    if (sourceInterface)
        sourceInterface.message(&Tiled2dMapSourceInterface::setMaxZoomLevelIdentifier, value);
}

std::optional<int32_t> Tiled2dMapLayer::getMaxZoomLevelIdentifier() {
    //if (sourceInterface)
//        return sourceInterface->getMaxZoomLevelIdentifier();
    return std::nullopt;
}

LayerReadyState Tiled2dMapLayer::isReadyToRenderOffscreen() {
    //if (sourceInterface) {
      //  return sourceInterface->isReadyToRenderOffscreen();
    //}

    return LayerReadyState::READY;
}

void Tiled2dMapLayer::setErrorManager(const std::shared_ptr<::ErrorManager> &errorManager) {
    this->errorManager = errorManager;
    auto sourceInterface = this->sourceInterface;
    if (sourceInterface) {
        sourceInterface.message(&Tiled2dMapSourceInterface::setErrorManager, errorManager);
    }
}

void Tiled2dMapLayer::forceReload() {
    auto sourceInterface = this->sourceInterface;
    if (sourceInterface) {
        sourceInterface.message(&Tiled2dMapSourceInterface::forceReload);
    }
}

void Tiled2dMapLayer::setT(int t) {
    curT = t;

    auto mapInterface = this->mapInterface;
    if (mapInterface) {
        auto camera = std::dynamic_pointer_cast<MapCamera2dInterface>(mapInterface->getCamera());
        if (camera) {
            onVisibleBoundsChanged(camera->getVisibleRect(), camera->getZoom());
        }
    }

    onTilesUpdated();
}
