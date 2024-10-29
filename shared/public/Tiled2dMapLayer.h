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

#include "SimpleLayerInterface.h"
#include "MapCameraListenerInterface.h"
#include "MapInterface.h"
#include "RenderPassInterface.h"
#include "Tiled2dMapLayerConfig.h"
#include "Tiled2dMapSourceInterface.h"
#include "SimpleTouchInterface.h"
#include "Actor.h"
#include <mutex>

class Tiled2dMapLayer : public SimpleLayerInterface,
                        public MapCameraListenerInterface,
                        public std::enable_shared_from_this<Tiled2dMapLayer> {
  public:
    Tiled2dMapLayer();

    void setSourceInterfaces(const std::vector<WeakActor<Tiled2dMapSourceInterface>> &sourceInterfaces);

    virtual void update() override = 0;

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() override = 0;

    virtual void onAdded(const std::shared_ptr<::MapInterface> &mapInterface, int32_t layerIndex) override;

    virtual void onRemoved() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void hide() override;

    virtual void show() override;

    virtual void onVisibleBoundsChanged(const ::RectCoord &visibleBounds, double zoom) override;

    virtual void setMaskingObject(const std::shared_ptr<::MaskingObjectInterface> & maskingObject) override;

    virtual void setErrorManager(const std::shared_ptr< ::ErrorManager> &errorManager) override;

    virtual void forceReload() override;

    void onRotationChanged(float angle) override;

    void onMapInteraction() override;

    void setMinZoomLevelIdentifier(std::optional<int32_t> value);

    std::optional<int32_t> getMinZoomLevelIdentifier();

    void setMaxZoomLevelIdentifier(std::optional<int32_t> value);

    std::optional<int32_t> getMaxZoomLevelIdentifier();

    virtual LayerReadyState isReadyToRenderOffscreen() override;

    virtual void setT(int32_t t);

    void onCameraChange(const std::vector<float> &viewMatrix, const std::vector<float> &projectionMatrix, const ::Vec3D & origin, float verticalFov,
                        float horizontalFov, float width, float height, float focusPointAltitude, const ::Coord & focusPointPosition, float zoom) override;

protected:
    std::shared_ptr<MapInterface> mapInterface;
    std::shared_ptr< ::ErrorManager> errorManager;
    std::recursive_mutex sourcesMutex;
    std::vector<WeakActor<Tiled2dMapSourceInterface>> sourceInterfaces;

    bool isHidden = false;

    std::optional<int32_t> minZoomLevelIdentifier = std::nullopt;
    std::optional<int32_t> maxZoomLevelIdentifier = std::nullopt;

    int curT;
};
