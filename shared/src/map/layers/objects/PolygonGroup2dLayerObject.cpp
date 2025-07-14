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
#include "TrigonometryLUT.h"
#include <cmath>

PolygonGroup2dLayerObject::PolygonGroup2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                                     const std::shared_ptr<PolygonGroup2dInterface> &polygon,
                                                     const std::shared_ptr<PolygonGroupShaderInterface> &shader)
    : conversionHelper(conversionHelper)
    , polygon(polygon)
    , shader(shader)
    , renderConfig(std::make_shared<RenderConfig>(polygon->asGraphicsObject(), 0)) {}

void PolygonGroup2dLayerObject::update() {}

std::vector<std::shared_ptr<RenderConfigInterface>> PolygonGroup2dLayerObject::getRenderConfig() { return {renderConfig}; }


void PolygonGroup2dLayerObject::setVertices(const std::vector<std::tuple<std::vector<::Coord>, int>> & vertices, const std::vector<uint16_t> & indices, bool is3d) {
    double avgX = 0.0f, avgY = 0.0f;
    size_t totalPoints = 0;
    std::vector<float> renderVertices;

    // First loop: Convert to render system, accumulate averages, and store render coordinates
    for (auto const &v : vertices) {
        float s = (float)std::get<1>(v);

        for (auto const &mapCoord : std::get<0>(v)) {
            // Convert to render system once
            const auto renderCoord = conversionHelper->convertToRenderSystem(mapCoord);

            // Accumulate for averaging
            avgX += renderCoord.x;
            avgY += renderCoord.y;
            totalPoints += 1;

            // Store the render coordinates temporarily in renderVertices
            renderVertices.push_back(renderCoord.x);
            renderVertices.push_back(renderCoord.y);
            renderVertices.push_back(renderCoord.z);
            renderVertices.push_back(s); // Append style or any extra data
        }
    }

    // Calculate the average (origin)
    if (totalPoints > 0) {
        avgX /= totalPoints;
        avgY /= totalPoints;
    }

    double rx = is3d ? 1.0 * sin(avgY) * cos(avgX) : avgX;
    double ry = is3d ? 1.0 * cos(avgY) : avgY;
    double rz = is3d ? -1.0 * sin(avgY) * sin(avgX) : 0.0;

    // Adjust the stored render coordinates by subtracting the average (origin)
    for (size_t i = 0; i < renderVertices.size(); i += 4) { // 4 values: x, y, z, s
        const auto& x = renderVertices[i];
        const auto& y = renderVertices[i + 1];
        const auto& z = renderVertices[i + 2];

        // Adjust the coordinates by subtracting the average
        if(is3d) {
            double sinX, sinY, cosX, cosY;
            lut::sincos(x, sinX, cosX);
            lut::sincos(y, sinY, cosY);

            renderVertices[i]     = 1.0 * sinY * cosX - rx;
            renderVertices[i + 1] = 1.0 * cosY - ry;
            renderVertices[i + 2] = -1.0 * sinY * sinX - rz;
        } else {
            renderVertices[i]     = x - rx;
            renderVertices[i + 1] = y - ry;
            renderVertices[i + 2] = 0.0;
        }
    }

    auto i = SharedBytes((int64_t)indices.data(), (int32_t)indices.size(), (int32_t)sizeof(uint16_t));
    auto v = SharedBytes((int64_t)renderVertices.data(), (int32_t)renderVertices.size(), (int32_t)sizeof(float));
    
    polygon->setVertices(v, i, Vec3D(rx, ry, rz));
}

void PolygonGroup2dLayerObject::setVertices(const std::vector<float> &verticesBuffer,
                                            const std::vector<uint16_t> & indices,
                                            const Vec3D & origin) {
    auto i = SharedBytes((int64_t)indices.data(), (int32_t)indices.size(), (int32_t)sizeof(uint16_t));
    auto v = SharedBytes((int64_t)verticesBuffer.data(), (int32_t)verticesBuffer.size(), (int32_t)sizeof(float));
    polygon->setVertices(v, i, origin);
}

void PolygonGroup2dLayerObject::setStyles(const std::vector<PolygonStyle> &styles) {
    std::vector<float> shaderStyles;

    for(auto& s : styles) {
        shaderStyles.push_back(s.color.r);
        shaderStyles.push_back(s.color.g);
        shaderStyles.push_back(s.color.b);
        shaderStyles.push_back(s.color.a);
        shaderStyles.push_back(s.opacity);
#ifndef __APPLE__
        // Padding to have the style size as a multiple of 16 bytes (OpenGL uniform buffer padding due to std140)
        shaderStyles.push_back(0.0);
        shaderStyles.push_back(0.0);
        shaderStyles.push_back(0.0);
#endif
    }

#ifndef __APPLE__
    auto s = SharedBytes((int64_t)shaderStyles.data(), (int32_t)styles.size(), (int32_t)8 * sizeof(float));
#else
    auto s = SharedBytes((int64_t)shaderStyles.data(), (int32_t)styles.size(), (int32_t)5 * sizeof(float));
#endif
    shader->setStyles(s);
}


std::shared_ptr<GraphicsObjectInterface> PolygonGroup2dLayerObject::getPolygonObject() { return polygon->asGraphicsObject(); }

std::shared_ptr<ShaderProgramInterface> PolygonGroup2dLayerObject::getShaderProgram() { return shader->asShaderProgramInterface(); }
