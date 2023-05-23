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
#include "Logger.h"

PolygonGroup2dLayerObject::PolygonGroup2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                                     const std::shared_ptr<PolygonGroup2dInterface> &polygon,
                                                     const std::shared_ptr<PolygonGroupShaderInterface> &shader)
    : conversionHelper(conversionHelper)
    , polygon(polygon)
    , shader(shader)
    , renderConfig(std::make_shared<RenderConfig>(polygon->asGraphicsObject(), 0)) {}

void PolygonGroup2dLayerObject::update() {}

std::vector<std::shared_ptr<RenderConfigInterface>> PolygonGroup2dLayerObject::getRenderConfig() { return {renderConfig}; }


void PolygonGroup2dLayerObject::setVertices(const std::vector<std::tuple<std::vector<::Coord>, int>> & vertices, const std::vector<uint16_t> & indices) {

    std::vector<float> renderVertices;

    for (auto const &v: vertices) {
        float s = (float)std::get<1>(v);

        for (auto const &mapCoord: std::get<0>(v)) {
            auto renderCoord = conversionHelper->convertToRenderSystem(mapCoord);
            renderVertices.push_back(renderCoord.x);
            renderVertices.push_back(renderCoord.y);
            renderVertices.push_back(s);
        }
    }

    auto i = SharedBytes((int64_t)indices.data(), (int32_t)indices.size(), (int32_t)sizeof(uint16_t));
    auto v = SharedBytes((int64_t)renderVertices.data(), (int32_t)renderVertices.size(), (int32_t)sizeof(float));
    
    polygon->setVertices(v, i);
}

void PolygonGroup2dLayerObject::setStyles(const std::vector<PolygonStyle> &styles) {
    std::vector<float> shaderStyles;

    for(auto& s : styles) {
        shaderStyles.push_back(s.color.r);
        shaderStyles.push_back(s.color.g);
        shaderStyles.push_back(s.color.b);
        shaderStyles.push_back(s.color.a);
        shaderStyles.push_back(s.opacity);
    }

    auto s = SharedBytes((int64_t)shaderStyles.data(), (int32_t)styles.size(), (int32_t)5 * sizeof(float));
    shader->setStyles(s);
}


std::shared_ptr<GraphicsObjectInterface> PolygonGroup2dLayerObject::getPolygonObject() { return polygon->asGraphicsObject(); }

std::shared_ptr<ShaderProgramInterface> PolygonGroup2dLayerObject::getShaderProgram() { return shader->asShaderProgramInterface(); }
