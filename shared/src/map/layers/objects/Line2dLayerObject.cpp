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
#include <cmath>

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

void Line2dLayerObject::setPositions(const std::vector<Coord> &positions) {
    std::vector<uint32_t> lineIndices;
    std::vector<float> lineAttributes;

    std::vector<Vec2D> renderCoords;
    for (auto const &mapCoord : positions) {
        Coord renderCoord = conversionHelper->convertToRenderSystem(mapCoord);
        renderCoords.push_back(Vec2D(renderCoord.x, renderCoord.y));
    }

    int pointCount = (int)renderCoords.size();

    float prefixTotalLineLength = 0.0;

    int iSecondToLast = pointCount - 2;
    for (int i = 0; i <= iSecondToLast; i++) {
        const Vec2D &p = renderCoords[i];
        const Vec2D &pNext = renderCoords[i + 1];

        float lengthNormalX = pNext.x - p.x;
        float lengthNormalY = pNext.y - p.y;
        float lineLength = std::sqrt(lengthNormalX * lengthNormalX + lengthNormalY * lengthNormalY);
        lengthNormalX = lengthNormalX / lineLength;
        lengthNormalY = lengthNormalY / lineLength;
        float widthNormalX = -lengthNormalY;
        float widthNormalY = lengthNormalX;

        // SegmentType (0 inner, 1 start, 2 end, 3 single segment) | lineStyleIndex
        // (each one Byte, i.e. up to 256 styles if supported by shader!)
        float lineStyleInfo = 0 + (i == 0 && i == iSecondToLast ? (3 << 8)
                                                : (i == 0 ? (float) (1 << 8)
                                                   : (i == iSecondToLast ? (float) (2 << 8)
                                                      : 0.0)));

        // Vertex 1
        // Position
        lineAttributes.push_back(p.x);
        lineAttributes.push_back(p.y);

        // Width normal
        lineAttributes.push_back(-widthNormalX);
        lineAttributes.push_back(-widthNormalY);

        // Length normal
        lineAttributes.push_back(-lengthNormalX);
        lineAttributes.push_back(-lengthNormalY);

        // Position pointA and pointB
        lineAttributes.push_back(p.x);
        lineAttributes.push_back(p.y);
        lineAttributes.push_back(pNext.x);
        lineAttributes.push_back(pNext.y);

        // Segment Start Length Position (length prefix sum)
        lineAttributes.push_back(prefixTotalLineLength);

        // Style Info
        lineAttributes.push_back(lineStyleInfo);

        // Vertex 2
        lineAttributes.push_back(p.x);
        lineAttributes.push_back(p.y);

        lineAttributes.push_back(widthNormalX);
        lineAttributes.push_back(widthNormalY);

        lineAttributes.push_back(-lengthNormalX);
        lineAttributes.push_back(-lengthNormalY);

        lineAttributes.push_back(p.x);
        lineAttributes.push_back(p.y);
        lineAttributes.push_back(pNext.x);
        lineAttributes.push_back(pNext.y);

        lineAttributes.push_back(prefixTotalLineLength);

        lineAttributes.push_back(lineStyleInfo);

        // Vertex 3
        lineAttributes.push_back(pNext.x);
        lineAttributes.push_back(pNext.y);

        lineAttributes.push_back(widthNormalX);
        lineAttributes.push_back(widthNormalY);

        lineAttributes.push_back(lengthNormalX);
        lineAttributes.push_back(lengthNormalY);

        lineAttributes.push_back(p.x);
        lineAttributes.push_back(p.y);
        lineAttributes.push_back(pNext.x);
        lineAttributes.push_back(pNext.y);
        lineAttributes.push_back(prefixTotalLineLength);

        lineAttributes.push_back(lineStyleInfo);

        // Vertex 4
        lineAttributes.push_back(pNext.x);
        lineAttributes.push_back(pNext.y);

        lineAttributes.push_back(-widthNormalX);
        lineAttributes.push_back(-widthNormalY);

        lineAttributes.push_back(lengthNormalX);
        lineAttributes.push_back(lengthNormalY);

        lineAttributes.push_back(p.x);
        lineAttributes.push_back(p.y);
        lineAttributes.push_back(pNext.x);
        lineAttributes.push_back(pNext.y);

        lineAttributes.push_back(prefixTotalLineLength);

        lineAttributes.push_back(lineStyleInfo);

        // Vertex indices
        lineIndices.push_back(4 * i);
        lineIndices.push_back(4 * i + 1);
        lineIndices.push_back(4 * i + 2);

        lineIndices.push_back(4 * i);
        lineIndices.push_back(4 * i + 2);
        lineIndices.push_back(4 * i + 3);

        prefixTotalLineLength += lineLength;
    }

    auto attributes = SharedBytes((int64_t) lineAttributes.data(), (int32_t) lineAttributes.size(), (int32_t) sizeof(float));
    auto indices = SharedBytes((int64_t) lineIndices.data(), (int32_t) lineIndices.size(), (int32_t) sizeof(uint32_t));
    line->setLine(attributes, indices);
}

void Line2dLayerObject::setStyle(const LineStyle &style) { shader->setStyle(style); }

void Line2dLayerObject::setHighlighted(bool highlighted) { shader->setHighlighted(highlighted); }

std::shared_ptr<GraphicsObjectInterface> Line2dLayerObject::getLineObject() { return line->asGraphicsObject(); }

std::shared_ptr<ShaderProgramInterface> Line2dLayerObject::getShaderProgram() { return shader->asShaderProgramInterface(); }
