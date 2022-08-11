/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "RenderObject.h"

RenderObject::RenderObject(std::shared_ptr<::GraphicsObjectInterface> graphicsObject)
    : graphicsObject(graphicsObject) {}

RenderObject::RenderObject(std::shared_ptr<::GraphicsObjectInterface> graphicsObject, std::vector<float> modelMatrix)
    : graphicsObject(graphicsObject)
    , setCustomModelMatrix(true)
    , modelMatrix(modelMatrix) {}

std::shared_ptr<::GraphicsObjectInterface> RenderObject::getGraphicsObject() { return graphicsObject; }

bool RenderObject::hasCustomModelMatrix() { return setCustomModelMatrix; }

std::vector<float> RenderObject::getCustomModelMatrix() { return modelMatrix; }
