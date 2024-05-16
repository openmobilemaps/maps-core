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
#include "Logger.h"
#include "ShaderLineStyle.h"

#include <cmath>

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

    std::vector<uint32_t> lineIndices;
    std::vector<float> lineAttributes;

    int numLines = (int) lines.size();

    int lineIndexOffset = 0;
    for (int lineIndex = 0; lineIndex < numLines; lineIndex++) {
        int lineStyleIndex = std::get<1>(lines[lineIndex]);

        std::vector<Vec2D> renderCoords;
        for (auto const &mapCoord : std::get<0>(lines[lineIndex])) {
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
            float lineStyleInfo = lineStyleIndex + (i == 0 && i == iSecondToLast ? (3 << 8)
                    : (i == 0 ? (float) (1 << 8)
                    : (i == iSecondToLast ? (float) (2 << 8)
                    : 0.0)));

            // Vertex 4
            lineAttributes.push_back(pNext.x);
            lineAttributes.push_back(pNext.y);

            lineAttributes.push_back(widthNormalX);
            lineAttributes.push_back(widthNormalY);

            lineAttributes.push_back(p.x);
            lineAttributes.push_back(p.y);
            lineAttributes.push_back(pNext.x);
            lineAttributes.push_back(pNext.y);

            lineAttributes.push_back(3);
            lineAttributes.push_back(prefixTotalLineLength);
            lineAttributes.push_back(lineStyleInfo);

            // Vertex 3
            lineAttributes.push_back(pNext.x);
            lineAttributes.push_back(pNext.y);

            lineAttributes.push_back(widthNormalX);
            lineAttributes.push_back(widthNormalY);

            lineAttributes.push_back(p.x);
            lineAttributes.push_back(p.y);
            lineAttributes.push_back(pNext.x);
            lineAttributes.push_back(pNext.y);

            lineAttributes.push_back(2);
            lineAttributes.push_back(prefixTotalLineLength);
            lineAttributes.push_back(lineStyleInfo);

            // Vertex 2
            lineAttributes.push_back(p.x);
            lineAttributes.push_back(p.y);

            lineAttributes.push_back(widthNormalX);
            lineAttributes.push_back(widthNormalY);

            lineAttributes.push_back(p.x);
            lineAttributes.push_back(p.y);
            lineAttributes.push_back(pNext.x);
            lineAttributes.push_back(pNext.y);

            lineAttributes.push_back(1);
            lineAttributes.push_back(prefixTotalLineLength);
            lineAttributes.push_back(lineStyleInfo);

            // Vertex 1
            // Position
            lineAttributes.push_back(p.x);
            lineAttributes.push_back(p.y);

            // Width normal
            lineAttributes.push_back(widthNormalX);
            lineAttributes.push_back(widthNormalY);

            // Position pointA and pointB
            lineAttributes.push_back(p.x);
            lineAttributes.push_back(p.y);
            lineAttributes.push_back(pNext.x);
            lineAttributes.push_back(pNext.y);

            // Vertex Index
            lineAttributes.push_back(0);

            // Segment Start Length Position (length prefix sum)
            lineAttributes.push_back(prefixTotalLineLength);

            // Style Info
            lineAttributes.push_back(lineStyleInfo);


            // Vertex indices
            lineIndices.push_back(lineIndexOffset + 4 * i);
            lineIndices.push_back(lineIndexOffset + 4 * i + 1);
            lineIndices.push_back(lineIndexOffset + 4 * i + 2);

            lineIndices.push_back(lineIndexOffset + 4 * i);
            lineIndices.push_back(lineIndexOffset + 4 * i + 2);
            lineIndices.push_back(lineIndexOffset + 4 * i + 3);

            prefixTotalLineLength += lineLength;
        }
        lineIndexOffset += ((pointCount - 1) * 4);
    }

    auto attributes = SharedBytes((int64_t) lineAttributes.data(), (int32_t) lineAttributes.size(), (int32_t) sizeof(float));
    auto indices = SharedBytes((int64_t) lineIndices.data(), (int32_t) lineIndices.size(), (int32_t) sizeof(uint32_t));
    line->setLines(attributes, indices);
}

void LineGroup2dLayerObject::setStyles(const std::vector<LineStyle> &styles) {
    std::vector<ShaderLineStyle> shaderLineStyles;
    for(auto& s : styles) {
        auto cap = 1;
        switch(s.lineCap) {
            case LineCapType::BUTT: { cap = 0; break; }
            case LineCapType::ROUND: { cap = 1; break; }
            case LineCapType::SQUARE: { cap = 2; break; }
            default: { cap = 1; }
        }

        auto dn = s.dashArray.size();
        auto dValue0 = dn > 0 ? s.dashArray[0] : 0.0;
        auto dValue1 = (dn > 1 ? s.dashArray[1] : 0.0) + dValue0;
        auto dValue2 = (dn > 2 ? s.dashArray[2] : 0.0) + dValue1;
        auto dValue3 = (dn > 3 ? s.dashArray[3] : 0.0) + dValue2;

        float dotted = s.dotted ? 0 : 1;

        shaderLineStyles.emplace_back(s.width, s.color.normal.r, s.color.normal.g, s.color.normal.b, s.color.normal.a, s.gapColor.normal.r, s.gapColor.normal.g, s.gapColor.normal.b, s.gapColor.normal.a, s.widthType == SizeType::SCREEN_PIXEL ? 1.0 : 0.0, s.opacity, s.blur, cap, dn, dValue0, dValue1, dValue2, dValue3, s.offset, s.dotted, s.dottedSkew);
    }

    auto bytes = SharedBytes((int64_t)shaderLineStyles.data(), (int)shaderLineStyles.size(), 20 * sizeof(float));
    shader->setStyles(bytes);
}

void LineGroup2dLayerObject::setScalingFactor(float factor) {
    shader->setDashingScaleFactor(factor);
}

std::shared_ptr<GraphicsObjectInterface> LineGroup2dLayerObject::getLineObject() { return line->asGraphicsObject(); }

std::shared_ptr<ShaderProgramInterface> LineGroup2dLayerObject::getShaderProgram() { return shader->asShaderProgramInterface(); }
