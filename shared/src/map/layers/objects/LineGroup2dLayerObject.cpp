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
                                               const std::shared_ptr<LineGroupShaderInterface> &shader,
                                               bool is3d)
    : conversionHelper(conversionHelper)
    , line(line)
    , shader(shader)
    , renderConfig(std::make_shared<RenderConfig>(line->asGraphicsObject(), 0))
    , is3d(is3d) {}

void LineGroup2dLayerObject::update() {}

std::vector<std::shared_ptr<RenderConfigInterface>> LineGroup2dLayerObject::getRenderConfig() { return {renderConfig}; }

void LineGroup2dLayerObject::setLines(const std::vector<std::tuple<std::vector<Coord>, int>> &lines, const Vec3D & origin) {

    std::vector<uint32_t> lineIndices;
    std::vector<float> lineAttributes;

    int numLines = (int) lines.size();

    int lineIndexOffset = 0;
    for (int lineIndex = 0; lineIndex < numLines; lineIndex++) {
        int lineStyleIndex = std::get<1>(lines[lineIndex]);

        std::vector<Vec3D> renderCoords;
        for (auto const &mapCoord : std::get<0>(lines[lineIndex])) {
            Coord renderCoord = conversionHelper->convertToRenderSystem(mapCoord);
            
            double x = is3d ? 1.0 * sin(renderCoord.y) * cos(renderCoord.x) - origin.x : renderCoord.x - origin.x;
            double y = is3d ?  1.0 * cos(renderCoord.y) - origin.y : renderCoord.y - origin.y;
            double z = is3d ? -1.0 * sin(renderCoord.y) * sin(renderCoord.x) - origin.z : 0.0;

            renderCoords.push_back(Vec3D(x, y, z));
        }

        int pointCount = (int)renderCoords.size();

        float prefixTotalLineLength = 0.0;

        int iSecondToLast = pointCount - 2;
        for (int i = 0; i <= iSecondToLast; i++) {
            const Vec3D &p = renderCoords[i];
            const Vec3D &pNext = renderCoords[i + 1];

            float lengthNormalX = pNext.x - p.x;
            float lengthNormalY = pNext.y - p.y;
            float lineLength = std::sqrt(lengthNormalX * lengthNormalX + lengthNormalY * lengthNormalY);

            // SegmentType (0 inner, 1 start, 2 end, 3 single segment) | lineStyleIndex
            // (each one Byte, i.e. up to 256 styles if supported by shader!)
            float lineStyleInfo = lineStyleIndex + (i == 0 && i == iSecondToLast ? (3 << 8)
                    : (i == 0 ? (float) (1 << 8)
                    : (i == iSecondToLast ? (float) (2 << 8)
                    : 0.0)));

            for (uint8_t vertexIndex = 4; vertexIndex > 0; --vertexIndex) {
                // Vertex
                // Position pointA and pointB
                lineAttributes.push_back(p.x);
                lineAttributes.push_back(p.y);
                if (is3d) {
                    lineAttributes.push_back(p.z);
                }
                lineAttributes.push_back(pNext.x);
                lineAttributes.push_back(pNext.y);
                if (is3d) {
                    lineAttributes.push_back(pNext.z);
                }

                // Vertex Index
                lineAttributes.push_back((float) (vertexIndex - 1));

                // Segment Start Length Position (length prefix sum)
                lineAttributes.push_back(prefixTotalLineLength);

                // Style Info
                lineAttributes.push_back(lineStyleInfo);
            }

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
    line->setLines(attributes, indices, origin, is3d);
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

        printf("shaderLineStyles.push(%f, rgba[%f, %f, %f, %f])\n", s.width, s.color.normal.r, s.color.normal.g, s.color.normal.b, s.color.normal.a);
        shaderLineStyles.emplace_back(s.width, s.color.normal.r, s.color.normal.g, s.color.normal.b, s.color.normal.a, s.gapColor.normal.r, s.gapColor.normal.g, s.gapColor.normal.b, s.gapColor.normal.a, s.widthType == SizeType::SCREEN_PIXEL ? 1.0 : 0.0, s.opacity, s.blur, cap, dn, dValue0, dValue1, dValue2, dValue3, s.offset, s.dotted, s.dottedSkew);
    }

    auto bytes = SharedBytes((int64_t)shaderLineStyles.data(), (int)shaderLineStyles.size(), 21 * sizeof(float));
    shader->setStyles(bytes);
}

void LineGroup2dLayerObject::setScalingFactor(float factor) {
    shader->setDashingScaleFactor(factor);
}

std::shared_ptr<GraphicsObjectInterface> LineGroup2dLayerObject::getLineObject() { return line->asGraphicsObject(); }

std::shared_ptr<ShaderProgramInterface> LineGroup2dLayerObject::getShaderProgram() { return shader->asShaderProgramInterface(); }
