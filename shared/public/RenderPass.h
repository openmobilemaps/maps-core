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
    RenderPass(RenderPassConfig config, const std::vector<std::shared_ptr<::RenderObjectInterface>> &renderObjects);

    RenderPass(RenderPassConfig config, const std::vector<std::shared_ptr<::RenderObjectInterface>> &renderObjects, const std::shared_ptr<MaskingObjectInterface> &maskingObject);

    virtual std::vector<std::shared_ptr<::RenderObjectInterface>> getRenderObjects() override;

    virtual void addRenderObject(const std::shared_ptr<RenderObjectInterface> & renderObject) override;

    virtual RenderPassConfig getRenderPassConfig() override;

    virtual std::shared_ptr<::MaskingObjectInterface> getMaskingObject() override;

    virtual std::optional< ::RectI> getScissoringRect() override;

    void setScissoringRect(std::optional< ::RectI> rect);

private:
    RenderPassConfig config;
    std::vector<std::shared_ptr<::RenderObjectInterface>> renderObjects;
    std::shared_ptr<MaskingObjectInterface> maskingObject;

    std::optional< ::RectI> scissoringRect = std::nullopt;
};
