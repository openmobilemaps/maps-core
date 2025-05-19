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
#include "Vec3DHelper.h"

#include "TrigonometryLUT.h"
#include <cmath>
#include <utility>

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

void LineGroup2dLayerObject::setLines(const std::vector<std::tuple<std::vector<Vec2D>, int>> &lines, const int32_t systemIdentifier,
                                      const Vec3D &origin, LineCapType capType, LineJoinType joinType) {
    std::vector<uint32_t> lineIndices;
    std::vector<float> lineAttributes;

    int numLines = (int)lines.size();

    std::vector<std::tuple<std::vector<Vec3D>, int>> convertedLines;

    for (int lineIndex = 0; lineIndex < numLines; lineIndex++) {
        auto const &[mapCoords, lineStyleIndex] = lines[lineIndex];

        std::vector<Vec3D> renderCoords;
        for (auto const &mapCoord : mapCoords) {
            const auto& renderCoord = conversionHelper->convertToRenderSystem(Coord(systemIdentifier, mapCoord.x, mapCoord.y, 0.0));

            double sinX, cosX, sinY, cosY;
            lut::sincos(renderCoord.y, sinY, cosY);
            lut::sincos(renderCoord.x, sinX, cosX);

            double x = is3d ? 1.0 * sinY * cosX - origin.x : renderCoord.x - origin.x;
            double y = is3d ?  1.0 * cosY - origin.y : renderCoord.y - origin.y;
            double z = is3d ? -1.0 * sinY * sinX - origin.z : 0.0;

            renderCoords.push_back(Vec3D(x, y, z));
        }

        convertedLines.emplace_back(std::move(renderCoords), lineStyleIndex);
    }

    buildLines(convertedLines, origin, capType, joinType);
}

void LineGroup2dLayerObject::setLines(const std::vector<std::tuple<std::vector<Coord>, int>> &lines, const Vec3D &origin,
                                      LineCapType capType, LineJoinType joinType) {

    std::vector<uint32_t> lineIndices;
    std::vector<float> lineAttributes;

    int numLines = (int)lines.size();

    std::vector<std::tuple<std::vector<Vec3D>, int>> convertedLines;

    for (int lineIndex = 0; lineIndex < numLines; lineIndex++) {
        auto const &[mapCoords, lineStyleIndex] = lines[lineIndex];

        std::vector<Vec3D> renderCoords;
        for (auto const &mapCoord : mapCoords) {
            const auto& renderCoord = conversionHelper->convertToRenderSystem(mapCoord);

            double sinX, cosX, sinY, cosY;
            lut::sincos(renderCoord.y, sinY, cosY);
            lut::sincos(renderCoord.x, sinX, cosX);

            double x = is3d ? 1.0 * sinY * cosX - origin.x : renderCoord.x - origin.x;
            double y = is3d ?  1.0 * cosY - origin.y : renderCoord.y - origin.y;
            double z = is3d ? -1.0 * sinY * sinX - origin.z : 0.0;

            renderCoords.push_back(Vec3D(x, y, z));
        }

        convertedLines.emplace_back(std::move(renderCoords), lineStyleIndex);
    }

    buildLines(convertedLines, origin, capType, joinType);
}

void LineGroup2dLayerObject::buildLines(const std::vector<std::tuple<std::vector<Vec3D>, int>> &lines, const Vec3D &origin,
                                        LineCapType capType, LineJoinType joinType) {
    std::vector<uint32_t> lineIndices;
    std::vector<float> lineAttributes;
    uint32_t vertexCount = 0;

    int numLines = (int)lines.size();

    for (int lineIndex = numLines - 1; lineIndex >= 0; lineIndex--) {
        auto const &[renderCoords, lineStyleIndex] = lines[lineIndex];

        int pointCount = (int)renderCoords.size();

        if(pointCount < 2) {
            continue;
        }

        float prefixTotalLineLength = 0.0;
        float lineLength = 0;
        float cosAngle = 0, cosHalfAngle = 0;

        Vec3D pLast(0,0,0), normal(0,0,0), lastNormal(0,0,0), lineVec(0,0,0), lastLineVec(0,0,0);

        int32_t preIndex = -1, prePreIndex = -1;

        for (int i = 0; i < pointCount; i++) {
            const Vec3D &p = renderCoords[i];

            prefixTotalLineLength += lineLength;

            float extrudeScale = 1.0;
            LineJoinType vertexJoinType = joinType;

            Vec3D extrude(0, 0, 0), extrudeLineVec(0, 0, 0);

            if (i > 0) {
                lastNormal = normal;
                lastLineVec = lineVec;
            }

            if (i < pointCount - 1) {
                const Vec3D &pNext = renderCoords[i + 1];

                lineVec = pNext - p;
                lineLength = Vec3DHelper::length(lineVec);
                if (lineLength == 0) {
                    continue;
                }
                lineVec /= lineLength;

                normal = Vec3DHelper::normalize(Vec3D(-lineVec.y, lineVec.x, 0.0));

                if (i > 0) {
                    extrude = (normal + lastNormal) / 2.0;
                    double extrudeLength = Vec3DHelper::length(extrude);
                    if (extrudeLength > 0) {
                        extrude /= extrudeLength;
                    }

                    cosAngle = normal.x * lastNormal.x - normal.y * lastNormal.x;
                    cosHalfAngle = extrude.x * normal.x + extrude.y * normal.y;
                    extrudeScale = cosHalfAngle != 0 ? abs(1.0 / cosHalfAngle) : 1.0;
                    if (extrudeScale > 2.0) {
                        vertexJoinType = LineJoinType::BEVEL;
                        extrudeScale = 2.0;
                    }
                } else {
                    extrude = normal;
                    extrudeLineVec = lineVec;
                }
            } else if (i > 0) {
                lineLength = 0;
                extrude = lastNormal;
                extrudeLineVec = -lastLineVec;
            } else {
                lineLength = 0;
                continue;
            }

            float endSide = 0;
            if (i == 0) {
                endSide = -1;
            } else if (i == pointCount - 1) {
                endSide = 1;
            }

            if (endSide != 0 && capType == LineCapType::ROUND) {
                auto originalPrePreIndex = prePreIndex;
                auto originalPreIndex = preIndex;
                pushLineVertex(p, Vec3D(0, 0, 0), 1.0, 0, prefixTotalLineLength, lineStyleIndex, true, false, vertexCount,
                               prePreIndex, preIndex, lineAttributes, lineIndices);
                int32_t centerIndex = preIndex, firstIndex = -1, lastIndex = -1;
                for (float r = -1; r <= 1; r += 0.2) {
                    Vec3D roundExtrude = Vec3DHelper::normalize(extrude * r - extrudeLineVec * (1.0 - abs(r)));
                    pushLineVertex(p, roundExtrude, 1.0, r, prefixTotalLineLength, lineStyleIndex, true, false, vertexCount,
                                   prePreIndex, preIndex, lineAttributes, lineIndices);
                    if (r == 0) {
                        firstIndex = preIndex;
                    } else {
                        lastIndex = preIndex;
                    }
                    prePreIndex = centerIndex;
                }
                prePreIndex = originalPrePreIndex;
                preIndex = originalPreIndex;
            }


            for (int8_t side = -1; side <= 1; side += 2) {
                Vec3D pointExtrude = extrude * (double)side;
                if (capType == LineCapType::SQUARE && endSide != 0) {
                    pointExtrude = pointExtrude + extrudeLineVec * endSide;
                }
                if (side * cosAngle < 0 && endSide == 0 && extrudeScale > 0.1 && joinType != LineJoinType::MITER) {
                    const double approxAngle = 2 * std::sqrt(2 - 2 * cosHalfAngle);
                    // 2.86 ~= 180/pi / 20 -> approximately one slice per 20 degrees
                    double stepSize = (joinType == LineJoinType::ROUND) ? 1.0 / round(approxAngle * 2.86) : 1.0;
                    for (float r = 0; r <= 1; r += stepSize) {
                        pointExtrude = Vec3DHelper::normalize(lastNormal * (1.0 - r) + normal * r) * (double)side;
                        pushLineVertex(p, pointExtrude, 1.0, side, prefixTotalLineLength, lineStyleIndex, true, false, vertexCount,
                                       prePreIndex, preIndex, lineAttributes, lineIndices);
                        std::swap(prePreIndex, preIndex);
                    }
                    std::swap(prePreIndex, preIndex);
                }
                else {
                    pushLineVertex(p, pointExtrude, extrudeScale, side, prefixTotalLineLength, lineStyleIndex, true, side == -1, vertexCount,
                                   prePreIndex, preIndex, lineAttributes, lineIndices);
                }
            }
        }
    }

    auto attributes = SharedBytes((int64_t)lineAttributes.data(), (int32_t)lineAttributes.size(), (int32_t)sizeof(float));
    auto indices = SharedBytes((int64_t)lineIndices.data(), (int32_t)lineIndices.size(), (int32_t)sizeof(uint32_t));
    line->setLines(attributes, indices, origin, is3d);
}

void LineGroup2dLayerObject::pushLineVertex(const Vec3D &p, const Vec3D &extrude, const float extrudeScale, const float side,
                                            const float prefixTotalLineLength, const int lineStyleIndex, const bool addTriangle,
                                            const bool reverse, uint32_t &vertexCount, int32_t &prePreIndex, int32_t &preIndex,
                                            std::vector<float> &lineAttributes, std::vector<uint32_t> &lineIndices) {
    lineAttributes.push_back(p.x);
    lineAttributes.push_back(p.y);
    if (is3d) {
        lineAttributes.push_back(p.z);
    }
    lineAttributes.push_back(extrude.x * extrudeScale);
    lineAttributes.push_back(extrude.y * extrudeScale);
    if (is3d) {
        lineAttributes.push_back(extrude.z * extrudeScale);
    }

    // Line Side
    lineAttributes.push_back((float)side);

    // Segment Start Length Position (length prefix sum)
    lineAttributes.push_back(prefixTotalLineLength);

    // Style Info
    lineAttributes.push_back(lineStyleIndex);

    uint32_t newIndex = vertexCount++;

    if (addTriangle) {
        if (prePreIndex != -1 && preIndex != -1) {
            if (reverse) {
                lineIndices.push_back(newIndex);
                lineIndices.push_back(preIndex);
                lineIndices.push_back(prePreIndex);
            } else {
                lineIndices.push_back(prePreIndex);
                lineIndices.push_back(preIndex);
                lineIndices.push_back(newIndex);
            }
        }
    }
    prePreIndex = preIndex;
    preIndex = newIndex;
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
