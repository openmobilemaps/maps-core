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
#include "MapCameraInterface.h"
#include "CoordinateSystemIdentifiers.h"

Tiled2dMapLayer::Tiled2dMapLayer()
    : curT(0) {}

void Tiled2dMapLayer::setSourceInterfaces(const std::vector<WeakActor<Tiled2dMapSourceInterface>> &sourceInterfaces) {
    std::lock_guard<std::recursive_mutex> lock(sourcesMutex);
    this->sourceInterfaces = sourceInterfaces;
    for (const auto &sourceInterface : sourceInterfaces) {
        if (isHidden) {
            sourceInterface.message(&Tiled2dMapSourceInterface::pause);
        }
        auto errorManager = this->errorManager;
        if (errorManager) {
            sourceInterface.message(&Tiled2dMapSourceInterface::setErrorManager, errorManager);
        }
    }
}

void Tiled2dMapLayer::onAdded(const std::shared_ptr<::MapInterface> &mapInterface, int32_t layerIndex) {
    this->mapInterface = mapInterface;

    {
        std::lock_guard<std::recursive_mutex> lock(sourcesMutex);
        for (const auto &sourceInterface: sourceInterfaces) {
            sourceInterface.message(&Tiled2dMapSourceInterface::setMinZoomLevelIdentifier, minZoomLevelIdentifier);
            sourceInterface.message(&Tiled2dMapSourceInterface::setMaxZoomLevelIdentifier, maxZoomLevelIdentifier);
        }
    }

    auto camera = std::dynamic_pointer_cast<MapCameraInterface>(mapInterface->getCamera());
    if (camera) {
        camera->addListener(shared_from_this());
        camera->notifyListenerBoundsChange();
    }
}

void Tiled2dMapLayer::onRemoved() {
    if (mapInterface) {
        auto camera = std::dynamic_pointer_cast<MapCameraInterface>(mapInterface->getCamera());
        if (camera) {
            camera->removeListener(shared_from_this());
        }
    }
    mapInterface = nullptr;
}

void Tiled2dMapLayer::pause() {
    std::lock_guard<std::recursive_mutex> lock(sourcesMutex);
    for (const auto &sourceInterface : sourceInterfaces) {
        sourceInterface.message(&Tiled2dMapSourceInterface::pause);
    }
}

void Tiled2dMapLayer::resume() {
    if (!isHidden) {
        std::lock_guard<std::recursive_mutex> lock(sourcesMutex);
        for (const auto &sourceInterface : sourceInterfaces) {
            sourceInterface.message(&Tiled2dMapSourceInterface::resume);
        }
    }
}

void Tiled2dMapLayer::hide() {
    if (isHidden) {
        return;
    }
    isHidden = true;
    {
        std::lock_guard<std::recursive_mutex> lock(sourcesMutex);
        for (const auto &sourceInterface: sourceInterfaces) {
            sourceInterface.message(&Tiled2dMapSourceInterface::pause);
        }
    }
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void Tiled2dMapLayer::show() {
    if (isHidden == false) {
        return;
    }
    isHidden = false;
    {
        std::lock_guard<std::recursive_mutex> lock(sourcesMutex);
        for (const auto &sourceInterface : sourceInterfaces) {
            sourceInterface.message(&Tiled2dMapSourceInterface::resume);
        }
    }
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void Tiled2dMapLayer::onVisibleBoundsChanged(const ::RectCoord &visibleBounds, double zoom) {
    std::lock_guard<std::recursive_mutex> lock(sourcesMutex);
    for (const auto &sourceInterface : sourceInterfaces) {
        sourceInterface.message(MailboxDuplicationStrategy::replaceNewest, &Tiled2dMapSourceInterface::onVisibleBoundsChanged,
                                visibleBounds, curT, zoom);
    }
}

void Tiled2dMapLayer::onCameraChange(const std::vector<float> &viewMatrix, const std::vector<float> &projectionMatrix, float verticalFov,
                                float horizontalFov, float width, float height, float focusPointAltitude, const ::Coord & focusPointPosition, float zoom) {
    std::lock_guard<std::recursive_mutex> lock(sourcesMutex);

    for (const auto &sourceInterface: sourceInterfaces) {
        sourceInterface.message(MailboxDuplicationStrategy::replaceNewest, &Tiled2dMapSourceInterface::onCameraChange,
                                viewMatrix, projectionMatrix, verticalFov, horizontalFov, width, height, focusPointAltitude, focusPointPosition, zoom);
    }
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
    std::lock_guard<std::recursive_mutex> lock(sourcesMutex);
    for (const auto &sourceInterface : sourceInterfaces) {
        sourceInterface.message(&Tiled2dMapSourceInterface::setMinZoomLevelIdentifier, value);
    }
}

std::optional<int32_t> Tiled2dMapLayer::getMinZoomLevelIdentifier() {
    // TODO: adjust for multiple sources
    /* if (sourceInterface)
           return sourceInterface->getMinZoomLevelIdentifier();*/
    return std::nullopt;
}

void Tiled2dMapLayer::setMaxZoomLevelIdentifier(std::optional<int32_t> value) {
    maxZoomLevelIdentifier = value;
    std::lock_guard<std::recursive_mutex> lock(sourcesMutex);
    for (const auto &sourceInterface : sourceInterfaces) {
        sourceInterface.message(&Tiled2dMapSourceInterface::setMaxZoomLevelIdentifier, value);
    }
}

std::optional<int32_t> Tiled2dMapLayer::getMaxZoomLevelIdentifier() {
    // TODO: adjust for multiple sources
    // if (sourceInterface)
    //        return sourceInterface->getMaxZoomLevelIdentifier();
    return std::nullopt;
}

LayerReadyState Tiled2dMapLayer::isReadyToRenderOffscreen() {
    bool isNotReady = false;
    for (const auto &source : sourceInterfaces) {
        auto state = source.syncAccess([](const auto &source) {
            if(auto strong = source.lock()) {
                return strong->isReadyToRenderOffscreen();
            }
            return LayerReadyState::ERROR;
        });
        switch (state) {
            case LayerReadyState::ERROR:
            case LayerReadyState::TIMEOUT_ERROR:
                return state;
            case LayerReadyState::NOT_READY:
                isNotReady = true;
                break;
            case LayerReadyState::READY:
                break;
        }
    }
    return isNotReady ? LayerReadyState::NOT_READY : LayerReadyState::READY;
}

void Tiled2dMapLayer::setErrorManager(const std::shared_ptr<::ErrorManager> &errorManager) {
    this->errorManager = errorManager;
    std::lock_guard<std::recursive_mutex> lock(sourcesMutex);
    for (const auto &sourceInterface : sourceInterfaces) {
        sourceInterface.message(&Tiled2dMapSourceInterface::setErrorManager, errorManager);
    }
}

void Tiled2dMapLayer::forceReload() {
    std::lock_guard<std::recursive_mutex> lock(sourcesMutex);
    for (const auto &sourceInterface : sourceInterfaces) {
        sourceInterface.message(&Tiled2dMapSourceInterface::forceReload);
    }
}

void Tiled2dMapLayer::setT(int t) {
    curT = t;

    auto mapInterface = this->mapInterface;
    if (mapInterface) {
        auto camera = std::dynamic_pointer_cast<MapCameraInterface>(mapInterface->getCamera());
        if (camera) {
            onVisibleBoundsChanged(camera->getVisibleRect(), camera->getZoom());
        }
    }

    //onRasterTilesUpdated();
}
