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
#include <cmath>

Line2dLayerObject::Line2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                     const std::shared_ptr<LineGroup2dInterface> &line,
                                     const std::shared_ptr<LineGroupShaderInterface> &shader)
    : conversionHelper(conversionHelper)
    , line(line)
    , shader(shader)
    , style(ColorStateList(Color(0.0f,0.0f,0.0f,0.0f), Color(0.0f,0.0f,0.0f,0.0f)), ColorStateList(Color(0.0f,0.0f,0.0f,0.0f), Color(0.0f,0.0f,0.0f,0.0f)), 0.0, 0.0, SizeType::SCREEN_PIXEL, 0.0, std::vector<float>(), LineCapType::BUTT, 0.0, false, 1.0)
    , highlighted(false)
{
    renderConfig = {std::make_shared<RenderConfig>(line->asGraphicsObject(), 0)};
}

void Line2dLayerObject::update() {}

std::vector<std::shared_ptr<RenderConfigInterface>> Line2dLayerObject::getRenderConfig() { return renderConfig; }

void Line2dLayerObject::setPositions(const std::vector<Coord> &positions) {
    std::vector<uint32_t> lineIndices;
    std::vector<float> lineAttributes;

    std::vector<Vec3D> renderCoords;
    for (auto const &mapCoord : positions) {
        Coord renderCoord = conversionHelper->convertToRenderSystem(mapCoord);

        const double rx = 0.711650 * 1.0;
        const double ry = 0.287723 * 1.0;
        const double rz = -0.639713 * 1.0;

        double x = (1.0 * sin(renderCoord.y) * cos(renderCoord.x) - rx) * 1111.0;
        double y = (1.0 * cos(renderCoord.y) - ry) * 1111.0;
        double z = (-1.0 * sin(renderCoord.y) * sin(renderCoord.x) - rz) * 1111.0;

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
        lineAttributes.push_back(p.z);

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


        // Vertex indices
        lineIndices.push_back(4 * i);
        lineIndices.push_back(4 * i + 2);
        lineIndices.push_back(4 * i + 1);

        lineIndices.push_back(4 * i);
        lineIndices.push_back(4 * i + 3);
        lineIndices.push_back(4 * i + 2);

        prefixTotalLineLength += lineLength;
    }

    auto attributes = SharedBytes((int64_t) lineAttributes.data(), (int32_t) lineAttributes.size(), (int32_t) sizeof(float));
    auto indices = SharedBytes((int64_t) lineIndices.data(), (int32_t) lineIndices.size(), (int32_t) sizeof(uint32_t));

    line->setLines(attributes, indices);
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
    ShaderLineStyle s(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

    s.colorR = highlighted ? style.color.highlighted.r : style.color.normal.r;
    s.colorG = highlighted ? style.color.highlighted.g : style.color.normal.g;
    s.colorB =  highlighted ? style.color.highlighted.b : style.color.normal.b;
    s.colorA = highlighted ? style.color.highlighted.a : style.color.normal.a;

    s.gapColorR = highlighted ? style.gapColor.highlighted.r : style.gapColor.normal.r;
    s.gapColorG = highlighted ? style.gapColor.highlighted.g : style.gapColor.normal.g;
    s.gapColorB =  highlighted ? style.gapColor.highlighted.b : style.gapColor.normal.b;
    s.gapColorA = highlighted ? style.gapColor.highlighted.a : style.gapColor.normal.a;

    s.opacity = style.opacity;
    s.blur = style.blur;

    // width type
    auto widthType = SizeType::SCREEN_PIXEL;
    auto widthAsPixel = (style.widthType == SizeType::SCREEN_PIXEL ? 1 : 0);
    s.widthAsPixel = widthAsPixel;

    s.width = style.width;

    // dashes
    auto dashArray = style.dashArray;
    auto dn = dashArray.size();
    s.numDashValue = dn;
    s.dashValue0 = dn > 0 ? dashArray[0] : 0.0;
    s.dashValue1 = (dn > 1 ? dashArray[1] : 0.0) + s.dashValue0;
    s.dashValue2 = (dn > 2 ? dashArray[2] : 0.0) + s.dashValue1;
    s.dashValue3 = (dn > 3 ? dashArray[3] : 0.0) + s.dashValue2;

    // line caps
    auto lineCap = style.lineCap;

    auto cap = 1;
    switch(lineCap){
        case LineCapType::BUTT: { cap = 0; break; }
        case LineCapType::ROUND: { cap = 1; break; }
        case LineCapType::SQUARE: { cap = 2; break; }
        default: { cap = 1; }
    }

    s.lineCap = cap;
    s.offset = style.offset;

    s.dotted = style.dotted;
    
    s.dottedSkew = style.dottedSkew;

    auto buffer = SharedBytes((int64_t)&s, 1, 21 * sizeof(float));
    shader->setStyles(buffer);
}

std::shared_ptr<GraphicsObjectInterface> Line2dLayerObject::getLineObject() { return line->asGraphicsObject(); }

std::shared_ptr<ShaderProgramInterface> Line2dLayerObject::getShaderProgram() { return shader->asShaderProgramInterface(); }
