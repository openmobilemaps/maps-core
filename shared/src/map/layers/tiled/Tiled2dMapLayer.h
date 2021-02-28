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

#include "LayerInterface.h"
#include "MapCamera2dListenerInterface.h"
#include "MapInterface.h"
#include "RenderPassInterface.h"
#include "Tiled2dMapLayerConfig.h"
#include "Tiled2dMapSourceInterface.h"
#include "Tiled2dMapSourceListenerInterface.h"
#include "SimpleTouchInterface.h"

class Tiled2dMapLayer : public LayerInterface,
                        public Tiled2dMapSourceListenerInterface,
                        public MapCamera2dListenerInterface,
                        public SimpleTouchInterface,
                        public std::enable_shared_from_this<Tiled2dMapLayer> {
  public:
    Tiled2dMapLayer(const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig);

    void setSourceInterface(const std::shared_ptr<Tiled2dMapSourceInterface> &sourceInterface);

    virtual void update() override = 0;

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() override = 0;

    virtual void onAdded(const std::shared_ptr<::MapInterface> &mapInterface) override;

    virtual void onRemoved() override;

    virtual void pause() override = 0;

    virtual void resume() override = 0;

    virtual void hide() override;

    virtual void show() override;

    virtual void onTilesUpdated() override = 0;

    virtual void onVisibleBoundsChanged(const ::RectCoord &visibleBounds, double zoom) override;

    void onRotationChanged(float angle) override;

protected:
    std::shared_ptr<MapInterface> mapInterface;
    const std::shared_ptr<Tiled2dMapLayerConfig> layerConfig;
    std::shared_ptr<Tiled2dMapSourceInterface> sourceInterface;

    bool isHidden = false;
};
