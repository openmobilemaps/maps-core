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
#include "LineGeometryBuilder.h"

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
                                      const Vec3D &origin, LineCapType capType, LineJoinType joinType, bool optimizeForDots) {
    int numLines = (int)lines.size();

    std::vector<std::tuple<std::vector<Vec3D>, int>> convertedLines;
    convertedLines.reserve(numLines);

    for (int lineIndex = 0; lineIndex < numLines; lineIndex++) {
        auto const &[mapCoords, lineStyleIndex] = lines[lineIndex];

        std::vector<Vec3D> renderCoords;
        renderCoords.reserve(mapCoords.size());
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

    LineGeometryBuilder::buildLines(line, convertedLines, origin, capType, joinType, is3d, optimizeForDots);
}

void LineGroup2dLayerObject::setLines(const std::vector<std::tuple<std::vector<Coord>, int>> &lines, const Vec3D &origin,
                                      LineCapType capType, LineJoinType joinType, bool optimizeForDots) {

    int numLines = (int)lines.size();

    std::vector<std::tuple<std::vector<Vec3D>, int>> convertedLines;
    convertedLines.reserve(numLines);

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

    LineGeometryBuilder::buildLines(line, convertedLines, origin, capType, joinType, is3d, optimizeForDots);
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
