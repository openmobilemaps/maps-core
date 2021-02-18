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

#include "RendererInterface.h"
#include <queue>

class Renderer : public RendererInterface {
  public:
    void addToRenderQueue(const std::shared_ptr<RenderPassInterface> &renderPass);

    /** Ensure calling on graphics thread */
    void drawFrame(const std::shared_ptr<RenderingContextInterface> &renderingContext,
                   const std::shared_ptr<CameraInterface> &camera);

  private:
    std::queue<const std::shared_ptr<RenderPassInterface>> renderQueue;
};
