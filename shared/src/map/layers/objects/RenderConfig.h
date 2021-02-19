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

#include "RenderConfigInterface.h"

class RenderConfig : public RenderConfigInterface {
  public:
    virtual ~RenderConfig(){};
    RenderConfig(std::shared_ptr<GraphicsObjectInterface> graphicsObjectInterface, int32_t renderIndex);

    virtual std::shared_ptr<::GraphicsObjectInterface> getGraphicsObject() override;

    virtual int32_t getRenderIndex() override;

  private:
    int32_t renderIndex;
    std::shared_ptr<GraphicsObjectInterface> graphicsObjectInterface;
};
