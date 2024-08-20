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
#include "RenderPassInterface.h"
#include "ComputePassInterface.h"
#include <map>
#include <queue>
#include <vector>

struct RenderPassInterfaceCompare {
    bool operator()(std::shared_ptr<RenderPassInterface> &a, std::shared_ptr<RenderPassInterface> &b) {
        return a->getRenderPassConfig().renderPassIndex > b->getRenderPassConfig().renderPassIndex;
    }
};

class Renderer : public RendererInterface {
  public:
    void addToRenderQueue(const std::shared_ptr<RenderPassInterface> &renderPass) override;
    void addToComputeQueue(const std::shared_ptr<ComputePassInterface> &computePass) override;

    /** Ensure calling on graphics thread */
    void drawFrame(const std::shared_ptr<RenderingContextInterface> &renderingContext,
                   const std::shared_ptr<CameraInterface> &camera) override;

    /** Ensure calling on graphics thread */
    void compute(const /*not-null*/ std::shared_ptr<RenderingContextInterface> &renderingContext,
                         const std::shared_ptr<CameraInterface> &camera) override;


private:
    std::map<int32_t, std::vector<std::shared_ptr<RenderPassInterface>>> renderQueue;
    std::vector<std::shared_ptr<ComputePassInterface>> computeQueue;

    std::vector<float> tempMvpMatrix = std::vector<float>(16, 0.0);

    std::vector<float> identityMatrix = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
};
