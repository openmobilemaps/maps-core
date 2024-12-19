/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "PolygonPatternGroup2dLayerObject.h"
#include "RenderVerticesDescription.h"
#include "Logger.h"
#include "BoundingBox.h"
#include "CoordinateSystemIdentifiers.h"
#include <cmath>

PolygonPatternGroup2dLayerObject::PolygonPatternGroup2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                                     const std::shared_ptr<PolygonPatternGroup2dInterface> &polygon,
                                                     const std::shared_ptr<PolygonPatternGroupShaderInterface> &shader)
    : conversionHelper(conversionHelper)
    , polygon(polygon)
    , shader(shader)
    , renderConfig(std::make_shared<RenderConfig>(polygon->asGraphicsObject(), 0)) {}

void PolygonPatternGroup2dLayerObject::update() {}

std::vector<std::shared_ptr<RenderConfigInterface>> PolygonPatternGroup2dLayerObject::getRenderConfig() { return {renderConfig}; }


void PolygonPatternGroup2dLayerObject::setVertices(const std::vector<std::tuple<std::vector<::Coord>, int>> & vertices, const std::vector<uint16_t> & indices, bool is3d) {
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
        double x = renderVertices[i];
        double y = renderVertices[i + 1];
        double z = renderVertices[i + 2];

        // Adjust the coordinates by subtracting the average
        renderVertices[i]     = is3d ? 1.0 * sin(y) * cos(x) - rx : x - rx;
        renderVertices[i + 1] = is3d ? 1.0 * cos(y) - ry : y - ry;
        renderVertices[i + 2] = is3d ? -1.0 * sin(y) * sin(x) - rz : 0.0;
    }

    auto i = SharedBytes((int64_t)indices.data(), (int32_t)indices.size(), (int32_t)sizeof(uint16_t));
    auto v = SharedBytes((int64_t)renderVertices.data(), (int32_t)renderVertices.size(), (int32_t)sizeof(float));

    polygon->setVertices(v, i, Vec3D(rx, ry, rz));
}

void PolygonPatternGroup2dLayerObject::setVertices(const std::vector<float> &verticesBuffer, const std::vector<uint16_t> & indices, const Vec3D & origin) {
    auto i = SharedBytes((int64_t)indices.data(), (int32_t)indices.size(), (int32_t)sizeof(uint16_t));
    auto v = SharedBytes((int64_t)verticesBuffer.data(), (int32_t)verticesBuffer.size(), (int32_t)sizeof(float));
    polygon->setVertices(v, i, origin);
}

void PolygonPatternGroup2dLayerObject::setOpacities(const std::vector<float> &opacities) {
    polygon->setOpacities(SharedBytes((int64_t)opacities.data(), (int32_t)opacities.size(), (int32_t)sizeof(float)));
}

void PolygonPatternGroup2dLayerObject::setTextureCoordinates(const std::vector<float> &coordinates) {
    polygon->setTextureCoordinates(SharedBytes((int64_t)coordinates.data(), (int32_t)coordinates.size() / 5, 5 * (int32_t)sizeof(float)));
}

void PolygonPatternGroup2dLayerObject::setScalingFactor(float factor) {
    polygon->setScalingFactor(factor);
}

void PolygonPatternGroup2dLayerObject::setScalingFactor(Vec2F factor) {
    polygon->setScalingFactors(factor);
}

void PolygonPatternGroup2dLayerObject::loadTexture(const /*not-null*/ std::shared_ptr<::RenderingContextInterface> & context, const /*not-null*/ std::shared_ptr<TextureHolderInterface> & textureHolder) {
    polygon->loadTexture(context, textureHolder);
}

std::shared_ptr<GraphicsObjectInterface> PolygonPatternGroup2dLayerObject::getPolygonObject() { return polygon->asGraphicsObject(); }

std::shared_ptr<ShaderProgramInterface> PolygonPatternGroup2dLayerObject::getShaderProgram() { return shader->asShaderProgramInterface(); }
