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

#include "GraphicsObjectInterface.h"
#include "RenderPassInterface.h"

class RenderPass : public RenderPassInterface {
  public:
    RenderPass(RenderPassConfig config, std::vector<std::shared_ptr<::GraphicsObjectInterface>> graphicsObjects);
    virtual std::vector<std::shared_ptr<::GraphicsObjectInterface>> getGraphicsObjects() override;
    virtual RenderPassConfig getRenderPassConfig() override;

  private:
    RenderPassConfig config;
    std::vector<std::shared_ptr<::GraphicsObjectInterface>> graphicsObjects;
};
