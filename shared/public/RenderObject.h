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

#include <vector>
#include "RenderObjectInterface.h"
#include "GraphicsObjectInterface.h"

class RenderObject : public RenderObjectInterface {
public:
    RenderObject(const std::shared_ptr<::GraphicsObjectInterface> graphicsObject);

    RenderObject(const std::shared_ptr<::GraphicsObjectInterface> graphicsObject, bool isScreenSpace, float screenSpaceScalingFactor = 1);

    RenderObject(const std::shared_ptr<::GraphicsObjectInterface> graphicsObject, std::vector<float> modelMatrix);

    virtual std::shared_ptr<::GraphicsObjectInterface> getGraphicsObject() override;

    virtual bool hasCustomModelMatrix() override;

    virtual std::vector<float> getCustomModelMatrix() override;

    virtual bool isScreenSpaceCoords() override;

    virtual float getScreenSpaceScalingFactor() override;
    
private:
    std::shared_ptr<GraphicsObjectInterface> graphicsObject;
    bool setCustomModelMatrix = false;
    bool screenSpaceCoords = false;
    float screenSpaceScalingFactorValue = 1;
    std::vector<float> modelMatrix;
};
