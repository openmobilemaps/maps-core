/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "RenderConfig.h"

RenderConfig::RenderConfig(std::shared_ptr<GraphicsObjectInterface> graphicsObjectInterface, int32_t renderIndex)
    : graphicsObjectInterface(graphicsObjectInterface)
    , renderIndex(renderIndex) {}

RenderConfig::RenderConfig(std::shared_ptr<GraphicsObjectInterface> graphicsObjectInterface,
                           std::shared_ptr<MaskingObjectInterface> maskingObjectInterface, int32_t renderIndex)
    : graphicsObjectInterface(graphicsObjectInterface)
    , maskingObjectInterface(maskingObjectInterface)
    , renderIndex(renderIndex) {}

std::shared_ptr<::GraphicsObjectInterface> RenderConfig::getGraphicsObject() { return graphicsObjectInterface; }

std::shared_ptr<::MaskingObjectInterface> RenderConfig::getMaskingObject() { return maskingObjectInterface; }

int32_t RenderConfig::getRenderIndex() { return renderIndex; }
