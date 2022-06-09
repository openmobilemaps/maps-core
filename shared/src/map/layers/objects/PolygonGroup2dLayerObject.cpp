/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "PolygonGroup2dLayerObject.h"
#include "RenderVerticesDescription.h"

PolygonGroup2dLayerObject::PolygonGroup2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                                     const std::shared_ptr<PolygonGroup2dInterface> &polygon,
                                                     const std::shared_ptr<PolygonGroupShaderInterface> &shader)
    : conversionHelper(conversionHelper)
    , polygon(polygon)
    , shader(shader)
    , renderConfig(std::make_shared<RenderConfig>(polygon->asGraphicsObject(), 0)) {}

void PolygonGroup2dLayerObject::update() {}

std::vector<std::shared_ptr<RenderConfigInterface>> PolygonGroup2dLayerObject::getRenderConfig() { return {renderConfig}; }

void PolygonGroup2dLayerObject::setVertices(const std::vector<std::tuple<std::vector<::Coord>, int>> &vertices,
                                            const std::vector<int32_t> &indices) {
    std::vector<RenderVerticesDescription> renderVertices;
    for (auto const &vertice : vertices) {
        std::vector<Vec2D> renderCoords;
        for (auto const &mapCoord : std::get<0>(vertice)) {
            Coord renderCoord = conversionHelper->convertToRenderSystem(mapCoord);
            renderCoords.push_back(Vec2D(renderCoord.x, renderCoord.y));
        }
        renderVertices.push_back(RenderVerticesDescription(renderCoords, std::get<1>(vertice)));
    }
    polygon->setVertices(renderVertices, indices);
}

void PolygonGroup2dLayerObject::setStyles(const std::vector<PolygonStyle> &styles) { shader->setStyles(styles); }

std::shared_ptr<GraphicsObjectInterface> PolygonGroup2dLayerObject::getPolygonObject() { return polygon->asGraphicsObject(); }

std::shared_ptr<ShaderProgramInterface> PolygonGroup2dLayerObject::getShaderProgram() { return shader->asShaderProgramInterface(); }
