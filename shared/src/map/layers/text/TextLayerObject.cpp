/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "TextLayerObject.h"

TextLayerObject::TextLayerObject(const std::shared_ptr<TextInterface> &text, const std::shared_ptr<TextShaderInterface> &shader, const Coord& referencePoint, float referenceSize):
text(text),
shader(shader),
referencePoint(referencePoint),
referenceSize(referenceSize)
{
    renderConfig = { std::make_shared<RenderConfig>(text->asGraphicsObject(), 0) };
    shader->setReferencePoint(Vec3D(referencePoint.x, referencePoint.y, referencePoint.z));
}

std::vector<std::shared_ptr<RenderConfigInterface>> TextLayerObject::getRenderConfig() {
    return renderConfig;
}
