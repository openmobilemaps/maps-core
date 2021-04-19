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

#include <unordered_map>
#include "RenderObjectInterface.h"
#include "RenderPassInterface.h"

class RenderPass : public RenderPassInterface {
public:
    RenderPass(RenderPassConfig config, std::vector<std::shared_ptr<::RenderObjectInterface>> renderObjects);

    virtual std::vector<std::shared_ptr<::RenderObjectInterface>> getRenderObjects() override;

    virtual void addRenderObject(const std::shared_ptr<RenderObjectInterface> & renderObject) override;

    virtual RenderPassConfig getRenderPassConfig() override;

private:
    RenderPassConfig config;
    std::vector<std::shared_ptr<::RenderObjectInterface>> renderObjects;
};
