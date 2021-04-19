/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Circle2dLayerObject.h"
#include "ColorCircleShaderInterface.h"
#include "Quad2dInterface.h"

Circle2dLayerObject::Circle2dLayerObject(const std::shared_ptr<MapInterface> &mapInterface)
        : conversionHelper(mapInterface->getCoordinateConverterHelper()),
          shader(),
          quad() {
    std::shared_ptr<ColorCircleShaderInterface> shader = mapInterface->getShaderFactory()->createColorCircleShader();
    quad = mapInterface->getGraphicsObjectFactory()->createQuad(shader->asShaderProgramInterface());
    this->shader = shader;
    renderConfig = std::make_shared<RenderConfig>(quad->asGraphicsObject(), 0);
}

std::vector<std::shared_ptr<RenderConfigInterface>> Circle2dLayerObject::getRenderConfig() {
    return { renderConfig };
}

void Circle2dLayerObject::setColor(Color color) {
    shader->setColor(color.r, color.g, color.b, color.a);
}

void Circle2dLayerObject::setPosition(Coord position, double radius) {
    Coord renderPos = conversionHelper->convertToRenderSystem(position);
    quad->setFrame(Quad2dD(Vec2D(renderPos.x - radius, renderPos.y - radius),
            Vec2D(renderPos.x + radius, renderPos.y - radius),
            Vec2D(renderPos.x + radius, renderPos.y + radius),
            Vec2D(renderPos.x - radius, renderPos.y + radius)), RectD(0, 0, 1, 1));
}

std::shared_ptr<Quad2dInterface> Circle2dLayerObject::getQuadObject() {
    return quad;
}
