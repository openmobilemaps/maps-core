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
#include "Vec3D.h"
#include "LineHelper.h"
#include <cmath>

Line2dLayerObject::Line2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                     const std::shared_ptr<LineGroup2dInterface> &line,
                                     const std::shared_ptr<LineGroupShaderInterface> &shader,
                                     bool is3d)
    : conversionHelper(conversionHelper)
    , line(line)
    , shader(shader)
    , style(ColorStateList(Color(0.0f,0.0f,0.0f,0.0f), Color(0.0f,0.0f,0.0f,0.0f)), ColorStateList(Color(0.0f,0.0f,0.0f,0.0f), Color(0.0f,0.0f,0.0f,0.0f)), 0.0, 0.0, SizeType::SCREEN_PIXEL, 0.0, std::vector<float>(), 0, 0, LineCapType::BUTT, 0.0, false, 1.0)
    , highlighted(false)
    , is3d(is3d)
{
    renderConfig = {std::make_shared<RenderConfig>(line->asGraphicsObject(), 0)};
}

void Line2dLayerObject::update() {}

std::vector<std::shared_ptr<RenderConfigInterface>> Line2dLayerObject::getRenderConfig() { return renderConfig; }

void Line2dLayerObject::setPositions(const std::vector<Coord> &positions, const Vec3D & origin) {
    std::vector<uint32_t> lineIndices;
    std::vector<float> lineAttributes;

    std::vector<Vec3D> renderCoords;
    for (auto const &mapCoord : positions) {
        Coord renderCoord = conversionHelper->convertToRenderSystem(mapCoord);

        double x = is3d ? renderCoord.z * sin(renderCoord.y) * cos(renderCoord.x) - origin.x : renderCoord.x - origin.x;
        double y = is3d ?  renderCoord.z * cos(renderCoord.y) - origin.y : renderCoord.y - origin.y;
        double z = is3d ? -renderCoord.z * sin(renderCoord.y) * sin(renderCoord.x) - origin.z : 0.0;

        renderCoords.push_back(Vec3D(x, y, z));
    }

    int pointCount = (int)renderCoords.size();

    float prefixTotalLineLength = 0.0;

    int iSecondToLast = pointCount - 2;
    for (int i = 0; i <= iSecondToLast; i++) {
        const Vec3D &p = renderCoords[i];
        const Vec3D &pNext = renderCoords[i + 1];

        double lengthNormalX = pNext.x - p.x;
        double lengthNormalY = pNext.y - p.y;
        double lengthNormalZ = pNext.z - p.z;
        float lineLength = std::sqrt(lengthNormalX * lengthNormalX + lengthNormalY * lengthNormalY + lengthNormalZ * lengthNormalZ);

        // Pack the values into a float-compatible representation
        uint8_t segmentType = (i == 0 && i == iSecondToLast) ? 3 :
                              (i == 0) ? 1 :
                              (i == iSecondToLast) ? 2 : 0;

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

            lineAttributes.push_back(LineHelper::packLineStyleMetadata(vertexIndex, 0, segmentType, prefixTotalLineLength));
        }

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

    line->setLines(attributes, indices, origin, is3d);
}

void Line2dLayerObject::setStyle(const LineStyle &style_) {
    style = style_;
    setStyle(style, highlighted);
}

void Line2dLayerObject::setHighlighted(bool highlighted_) {
    highlighted = highlighted_;
    setStyle(style, highlighted);
}

void Line2dLayerObject::setStyle(const LineStyle &style, bool highlighted) {
    ShaderLineStyle s = {0};

    s.colorR = toHalfFloat(highlighted ? style.color.highlighted.r : style.color.normal.r);
    s.colorG = toHalfFloat(highlighted ? style.color.highlighted.g : style.color.normal.g);
    s.colorB =  toHalfFloat(highlighted ? style.color.highlighted.b : style.color.normal.b);
    s.colorA = toHalfFloat(highlighted ? style.color.highlighted.a : style.color.normal.a);

    s.gapColorR = toHalfFloat(highlighted ? style.gapColor.highlighted.r : style.gapColor.normal.r);
    s.gapColorG = toHalfFloat(highlighted ? style.gapColor.highlighted.g : style.gapColor.normal.g);
    s.gapColorB =  toHalfFloat(highlighted ? style.gapColor.highlighted.b : style.gapColor.normal.b);
    s.gapColorA = toHalfFloat(highlighted ? style.gapColor.highlighted.a : style.gapColor.normal.a);

    s.opacity = toHalfFloat(style.opacity);
    s.blur = toHalfFloat(style.blur);

    // width type
    auto widthType = SizeType::SCREEN_PIXEL;
    auto widthAsPixel = (style.widthType == SizeType::SCREEN_PIXEL ? 1 : 0);
    s.widthAsPixel = toHalfFloat(widthAsPixel);

    s.width = toHalfFloat(style.width);

    // line caps
    auto lineCap = style.lineCap;

    auto cap = 1;
    switch(lineCap){
        case LineCapType::BUTT: { cap = 0; break; }
        case LineCapType::ROUND: { cap = 1; break; }
        case LineCapType::SQUARE: { cap = 2; break; }
        default: { cap = 1; }
    }

    s.lineCap = toHalfFloat(cap);

    // dashes
    auto dashArray = style.dashArray;
    auto dn = dashArray.size();
    s.numDashValue = toHalfFloat(dn);
    s.dashValue0 = toHalfFloat(dn > 0 ? dashArray[0] : 0.0);
    s.dashValue1 = toHalfFloat((dn > 1 ? dashArray[1] : 0.0) + s.dashValue0);
    s.dashValue2 = toHalfFloat((dn > 2 ? dashArray[2] : 0.0) + s.dashValue1);
    s.dashValue3 = toHalfFloat((dn > 3 ? dashArray[3] : 0.0) + s.dashValue2);
    s.dashFade = toHalfFloat(style.dashFade);
    s.dashAnimationSpeed = toHalfFloat(style.dashAnimationSpeed);

    s.offset = toHalfFloat(style.offset);

    s.dotted = toHalfFloat(style.dotted);

    s.dottedSkew = toHalfFloat(style.dottedSkew);

    auto buffer = SharedBytes((int64_t)&s, 1, sizeof(ShaderLineStyle));
    shader->setStyles(buffer);
}

std::shared_ptr<GraphicsObjectInterface> Line2dLayerObject::getLineObject() { return line->asGraphicsObject(); }

std::shared_ptr<ShaderProgramInterface> Line2dLayerObject::getShaderProgram() { return shader->asShaderProgramInterface(); }
