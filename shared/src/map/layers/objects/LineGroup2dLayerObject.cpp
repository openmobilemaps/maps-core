/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "LineGroup2dLayerObject.h"
#include "RenderLineDescription.h"

LineGroup2dLayerObject::LineGroup2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                               const std::shared_ptr<LineGroup2dInterface> &line,
                                               const std::shared_ptr<LineGroupShaderInterface> &shader)
    : conversionHelper(conversionHelper)
    , line(line)
    , shader(shader)
    , renderConfig(std::make_shared<RenderConfig>(line->asGraphicsObject(), 0)) {}

void LineGroup2dLayerObject::update() {}

std::vector<std::shared_ptr<RenderConfigInterface>> LineGroup2dLayerObject::getRenderConfig() { return {renderConfig}; }

void LineGroup2dLayerObject::setLines(const std::vector<std::tuple<std::vector<Coord>, int>> &lines) {
    std::vector<RenderLineDescription> renderCoordsGroups;
    for (auto const &line : lines) {
        std::vector<Vec2D> renderCoords;
        for (auto const &mapCoord : std::get<0>(line)) {
            Coord renderCoord = conversionHelper->convertToRenderSystem(mapCoord);
            renderCoords.push_back(Vec2D(renderCoord.x, renderCoord.y));
        }
        renderCoordsGroups.push_back(RenderLineDescription(renderCoords, std::get<1>(line)));
    }

    line->setLines(renderCoordsGroups);
}

void LineGroup2dLayerObject::setStyles(const std::vector<LineStyle> &styles) { shader->setStyles(styles); }

std::shared_ptr<GraphicsObjectInterface> LineGroup2dLayerObject::getLineObject() { return line->asGraphicsObject(); }

std::shared_ptr<ShaderProgramInterface> LineGroup2dLayerObject::getShaderProgram() { return shader->asShaderProgramInterface(); }
