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

#include "SkyboxLayerInterface.h"
#include "MapInterface.h"
#include "SimpleLayerInterface.h"
#include "SimpleTouchInterface.h"
#include "Textured2dLayerObject.h"
#include <map>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

class SkyboxLayer : public SkyboxLayerInterface,
                  public SimpleLayerInterface,
                  public std::enable_shared_from_this<SkyboxLayer> {
  public:
    SkyboxLayer();

    ~SkyboxLayer(){};

    virtual std::shared_ptr<::LayerInterface> asLayerInterface() override;

    // LayerInterface

    virtual void setMaskingObject(const std::shared_ptr<::MaskingObjectInterface> &maskingObject) override;

    virtual void update() override;

    virtual std::vector<::RenderTask> getRenderTasks() override;

    virtual void onAdded(const std::shared_ptr<MapInterface> &mapInterface, int32_t layerIndex) override;

    virtual void onRemoved() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void hide() override;

    virtual void show() override;


  private:

    std::shared_ptr<MapInterface> mapInterface;

    std::shared_ptr<MaskingObjectInterface> mask = nullptr;

    void preGenerateRenderPasses();
    std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> renderPassObjectMap;


   std::atomic<bool> isHidden;

                      std::vector<::RenderTask> renderTasks;


};
