/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Line2dLayerObject.h"

Line2dLayerObject::Line2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                     const std::shared_ptr<Line2dInterface> &line,
                                     const std::shared_ptr<ColorLineShaderInterface> &shader)
    : conversionHelper(conversionHelper)
    , line(line)
    , shader(shader) {
    renderConfig = {std::make_shared<RenderConfig>(line->asGraphicsObject(), 0)};
}

void Line2dLayerObject::update() {}

std::vector<std::shared_ptr<RenderConfigInterface>> Line2dLayerObject::getRenderConfig() { return renderConfig; }

void Line2dLayerObject::setPositions(std::vector<Coord> positions) {
    std::vector<Vec2D> renderCoords;
    for (Coord mapCoord : positions) {
        Coord renderCoord = conversionHelper->convertToRenderSystem(mapCoord);
        renderCoords.push_back(Vec2D(renderCoord.x, renderCoord.y));
    }
    line->setLinePositions(renderCoords);
}

void Line2dLayerObject::setColor(const Color &color) {
    shader->setColor(color.r, color.g, color.b, color.a);
}

void Line2dLayerObject::setMiter(const float miter) {
    shader->setMiter(miter);
}

std::shared_ptr<GraphicsObjectInterface> Line2dLayerObject::getLineObject() { return line->asGraphicsObject(); }

std::shared_ptr<LineShaderProgramInterface> Line2dLayerObject::getShaderProgram() { return shader->asLineShaderProgramInterface(); }
