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
#include "Logger.h"
#include "RenderLineDescription.h"
#include "ShaderLineStyle.h"

#include "TrigonometryLUT.h"

LineGroup2dLayerObject::LineGroup2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                               const std::shared_ptr<LineGroup2dInterface> &line,
                                               const std::shared_ptr<LineGroupShaderInterface> &shader, bool is3d)
    : conversionHelper(conversionHelper)
    , line(line)
    , shader(shader)
    , renderConfig(std::make_shared<RenderConfig>(line->asGraphicsObject(), 0))
    , is3d(is3d) {}

void LineGroup2dLayerObject::update() {}

std::vector<std::shared_ptr<RenderConfigInterface>> LineGroup2dLayerObject::getRenderConfig() { return {renderConfig}; }

void LineGroup2dLayerObject::setLines(const std::vector<std::tuple<std::vector<Vec2D>, int>> &lines, const int32_t systemIdentifier, const Vec3D &origin, LineCapType capType) {
    std::vector<uint32_t> lineIndices;
    std::vector<float> lineAttributes;

    int numLines = (int)lines.size();


    std::vector<std::tuple<std::vector<Vec3D>, int>> convertedLines;

    for (int lineIndex = 0; lineIndex < numLines; lineIndex++) {
        int lineStyleIndex = std::get<1>(lines[lineIndex]);

        std::vector<Vec3D> renderCoords;
        for (auto const &mapCoord : std::get<0>(lines[lineIndex])) {
            Coord renderCoord = conversionHelper->convertToRenderSystem(Coord(systemIdentifier, mapCoord.x, mapCoord.y, 0.0));

            double sinX, cosX, sinY, cosY;
            lut::sincos(renderCoord.y, sinY, cosY);
            lut::sincos(renderCoord.x, sinX, cosX);

            double x = is3d ? 1.0 * sinY * cosX - origin.x : renderCoord.x - origin.x;
            double y = is3d ?  1.0 * cosY - origin.y : renderCoord.y - origin.y;
            double z = is3d ? -1.0 * sinY * sinX - origin.z : 0.0;

            renderCoords.push_back(Vec3D(x, y, z));
        }

        convertedLines.push_back(std::make_tuple(renderCoords, lineStyleIndex));
        
    }

    buildLines(convertedLines, origin, capType);
}

void LineGroup2dLayerObject::setLines(const std::vector<std::tuple<std::vector<Coord>, int>> &lines, const Vec3D &origin, LineCapType capType) {

    std::vector<uint32_t> lineIndices;
    std::vector<float> lineAttributes;

    int numLines = (int)lines.size();

    std::vector<std::tuple<std::vector<Vec3D>, int>> convertedLines;

    for (int lineIndex = 0; lineIndex < numLines; lineIndex++) {
        int lineStyleIndex = std::get<1>(lines[lineIndex]);

        std::vector<Vec3D> renderCoords;
        for (auto const &mapCoord : std::get<0>(lines[lineIndex])) {
            const auto& renderCoord = conversionHelper->convertToRenderSystem(mapCoord);

            double sinX, cosX, sinY, cosY;
            lut::sincos(renderCoord.y, sinY, cosY);
            lut::sincos(renderCoord.x, sinX, cosX);

            double x = is3d ? 1.0 * sinY * cosX - origin.x : renderCoord.x - origin.x;
            double y = is3d ?  1.0 * cosY - origin.y : renderCoord.y - origin.y;
            double z = is3d ? -1.0 * sinY * sinX - origin.z : 0.0;

            renderCoords.push_back(Vec3D(x, y, z));
        }

        convertedLines.push_back(std::make_tuple(renderCoords, lineStyleIndex));

    }

    buildLines(convertedLines, origin, capType);

}

void LineGroup2dLayerObject::buildLines(const std::vector<std::tuple<std::vector<Vec3D>, int>> &lines, const Vec3D & origin, LineCapType capType) {
    std::vector<uint32_t> lineIndices;
    std::vector<float> lineAttributes;
    uint32_t vertexCount = 0;

    int numLines = (int)lines.size();

    for (int lineIndex = numLines-1; lineIndex >= 0; lineIndex--) {
        int lineStyleIndex = std::get<1>(lines[lineIndex]);

        std::vector<Vec3D> renderCoords = std::get<0>(lines[lineIndex]);

        int pointCount = (int)renderCoords.size();

        if(pointCount < 2) {
            continue;
        }

        float prefixTotalLineLength = 0.0;
        float lineLength = 0;

        std::optional<Vec3D> pLast, lastNormal = std::nullopt, lastLineVec = std::nullopt;

        int32_t preIndex = -1, prePreIndex = -1;

        for (int i = 0; i < pointCount; i++) {
            const Vec3D &p = renderCoords[i];

            prefixTotalLineLength += lineLength;

            Vec3D extrude(0, 0, 0), extrudeLineVec(0, 0, 0);

            if (i < pointCount - 1) {
                const Vec3D &pNext = renderCoords[i + 1];

                Vec3D lineVec(pNext.x - p.x, pNext.y - p.y, pNext.z - p.z);
                lineLength = std::sqrt(lineVec.x * lineVec.x + lineVec.y * lineVec.y + lineVec.z * lineVec.z);
                lineVec.x /= lineLength;
                lineVec.y /= lineLength;
                lineVec.z /= lineLength;

                Vec3D normal(-lineVec.y, lineVec.x, 0.0);
                double normalLength = sqrt(normal.x * normal.x + normal.y * normal.y);
                normal.x /= normalLength;
                normal.y /= normalLength;

                if (lastNormal) {
                    extrude.x = (normal.x + lastNormal->x) / 2.0;
                    extrude.y = (normal.y + lastNormal->y) / 2.0;
                }
                else {
                    extrude = normal;
                    extrudeLineVec = lineVec;
                }
                lastNormal = normal;
                lastLineVec = lineVec;
            } else if (lastNormal && lastLineVec) {
                lineLength = 0;
                extrude = *lastNormal;
                extrudeLineVec.x = -lastLineVec->x;
                extrudeLineVec.y = -lastLineVec->y;
                extrudeLineVec.z = -lastLineVec->z;
            } else {
                lineLength = 0;
                continue;
            }

            for (int8_t side = -1; side <= 1; side += 2) {
                lineAttributes.push_back(p.x);
                lineAttributes.push_back(p.y);
                if (is3d) {
                    lineAttributes.push_back(p.z);
                }
                if (i == 0 && capType == LineCapType::SQUARE) {
                    lineAttributes.push_back(extrude.x * (float)side - extrudeLineVec.x);
                    lineAttributes.push_back(extrude.y * (float)side - extrudeLineVec.y);
                    if (is3d) {
                        lineAttributes.push_back(extrude.z * (float)side - extrudeLineVec.z);
                    }
                }
                else if (i == pointCount-1 && capType == LineCapType::SQUARE) {
                    lineAttributes.push_back(extrude.x * (float)side + extrudeLineVec.x);
                    lineAttributes.push_back(extrude.y * (float)side + extrudeLineVec.y);
                    if (is3d) {
                        lineAttributes.push_back(extrude.z * (float)side + extrudeLineVec.z);
                    }
                }
                else {
                    lineAttributes.push_back(extrude.x * (float)side);
                    lineAttributes.push_back(extrude.y * (float)side);
                    if (is3d) {
                        lineAttributes.push_back(extrude.z * (float)side);
                    }
                }


                // Line Side
                lineAttributes.push_back((float)side);

                // Segment Start Length Position (length prefix sum)
                lineAttributes.push_back(prefixTotalLineLength);

                // Style Info
                lineAttributes.push_back(lineStyleIndex);

                uint32_t newIndex = vertexCount++;
                if (prePreIndex != -1 && preIndex != -1) {
                    if (side == -1) {
                        lineIndices.push_back(prePreIndex);
                        lineIndices.push_back(preIndex);
                        lineIndices.push_back(newIndex);
                    }
                    else {
                        lineIndices.push_back(newIndex);
                        lineIndices.push_back(preIndex);
                        lineIndices.push_back(prePreIndex);
                    }
                }
                prePreIndex = preIndex;
                preIndex = newIndex;
            }
        }
    }

    auto attributes = SharedBytes((int64_t)lineAttributes.data(), (int32_t)lineAttributes.size(), (int32_t)sizeof(float));
    auto indices = SharedBytes((int64_t)lineIndices.data(), (int32_t)lineIndices.size(), (int32_t)sizeof(uint32_t));
    line->setLines(attributes, indices, origin, is3d);
}

void LineGroup2dLayerObject::setStyles(const std::vector<LineStyle> &styles) {
    std::vector<ShaderLineStyle> shaderLineStyles;
    for (auto &s : styles) {
        auto cap = 1;
        switch (s.lineCap) {
        case LineCapType::BUTT: {
            cap = 0;
            break;
        }
        case LineCapType::ROUND: {
            cap = 1;
            break;
        }
        case LineCapType::SQUARE: {
            cap = 2;
            break;
        }
        default: {
            cap = 1;
        }
        }

        auto dn = s.dashArray.size();
        auto dValue0 = dn > 0 ? s.dashArray[0] : 0.0;
        auto dValue1 = (dn > 1 ? s.dashArray[1] : 0.0) + dValue0;
        auto dValue2 = (dn > 2 ? s.dashArray[2] : 0.0) + dValue1;
        auto dValue3 = (dn > 3 ? s.dashArray[3] : 0.0) + dValue2;

        float dotted = s.dotted ? 0 : 1;

        shaderLineStyles.emplace_back(ShaderLineStyle{toHalfFloat(s.width),
                                                      toHalfFloat(s.color.normal.r),
                                                      toHalfFloat(s.color.normal.g),
                                                      toHalfFloat(s.color.normal.b),
                                                      toHalfFloat(s.color.normal.a),
                                                      toHalfFloat(s.gapColor.normal.r),
                                                      toHalfFloat(s.gapColor.normal.g),
                                                      toHalfFloat(s.gapColor.normal.b),
                                                      toHalfFloat(s.gapColor.normal.a),
                                                      toHalfFloat(s.widthType == SizeType::SCREEN_PIXEL ? 1.0 : 0.0),
                                                      toHalfFloat(s.opacity),
                                                      toHalfFloat(s.blur),
                                                      toHalfFloat(cap),
                                                      toHalfFloat(dn),
                                                      toHalfFloat(dValue0),
                                                      toHalfFloat(dValue1),
                                                      toHalfFloat(dValue2),
                                                      toHalfFloat(dValue3),
                                                      toHalfFloat(s.dashFade),
                                                      toHalfFloat(s.dashAnimationSpeed),
                                                      toHalfFloat(s.offset),
                                                      toHalfFloat(s.dotted),
                                                      toHalfFloat(s.dottedSkew)});
    }

    auto bytes = SharedBytes((int64_t)shaderLineStyles.data(), (int)shaderLineStyles.size(), sizeof(ShaderLineStyle));
    shader->setStyles(bytes);
}

void LineGroup2dLayerObject::setScalingFactor(float factor) { shader->setDashingScaleFactor(factor); }

std::shared_ptr<GraphicsObjectInterface> LineGroup2dLayerObject::getLineObject() { return line->asGraphicsObject(); }

std::shared_ptr<ShaderProgramInterface> LineGroup2dLayerObject::getShaderProgram() { return shader->asShaderProgramInterface(); }
