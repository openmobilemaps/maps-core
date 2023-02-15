/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */


#include "TextLayerObject.h"

#include "GlyphDescription.h"
#include "BoundingBox.h"
#include "FormattedStringEntry.h"
#include "TextHelper.h"
#include "TextJustify.h"
#include "TextDescription.h"
#include "TextSymbolPlacement.h"
#include "Vec2DHelper.h"
#include "Logger.h"

#include <cmath>

TextLayerObject::TextLayerObject(const std::shared_ptr<TextInterface> &text, const std::shared_ptr<TextInfoInterface> &textInfo,const std::shared_ptr<TextShaderInterface> &shader, const std::shared_ptr<MapInterface> &mapInterface, const FontData& fontData, const Vec2F &offset, double lineHeight, double letterSpacing)
: text(text),
  textInfo(textInfo),
  lineCoordinates(textInfo->getLineCoordinates()),
  converter(mapInterface->getCoordinateConverterHelper()),
  camera(mapInterface->getCamera()),
  shader(shader),
  referencePoint(Coord("", 0.0, 0.0, 0.0)),
  fontData(fontData),
  offset(offset),
  lineHeight(lineHeight),
  letterSpacing(letterSpacing),
  boundingBox(Coord("", 0.0, 0.0, 0.0), Coord("", 0.0, 0.0, 0.0))
{
    if (text) {
        renderConfig = { std::make_shared<RenderConfig>(text->asGraphicsObject(), 1) };
    }

    referencePoint = converter->convertToRenderSystem(textInfo->getCoordinate());
    referenceSize = fontData.info.size;

    if (shader) {
        shader->setReferencePoint(Vec3D(referencePoint.x, referencePoint.y, referencePoint.z));
    }
}

std::vector<std::shared_ptr<RenderConfigInterface>> TextLayerObject::getRenderConfig() {
    return renderConfig;
}

void TextLayerObject::update(float scale) {
    switch(textInfo->getSymbolPlacement()) {
        case TextSymbolPlacement::POINT: {
            layoutPoint(scale);
            break;
        }

        case TextSymbolPlacement::LINE:
        case TextSymbolPlacement::LINE_CENTER: {
            layoutLine(scale);
            break;
        }
    }
}

void TextLayerObject::layoutPoint(float scale) {
    float fontSize = fontData.info.size * scale;
    auto pen = Vec2D(0.0, 0.0);

    std::vector<GlyphDescription> glyphs = {};

    std::optional<BoundingBox> box = std::nullopt;

    int characterCount = 0;
    std::vector<size_t> lineEndIndices;

    for (const auto &entry: textInfo->getText()) {
        for (const auto &c : TextHelper::splitWstring(entry.text)) {
            for (const auto &d : fontData.glyphs) {
                if (c == " " && characterCount < 15) {
                    pen.x += fontData.info.spaceAdvance * fontSize * entry.scale;
                    characterCount += 1;
                    break;
                } else if (c == "\n" || c == " ") {
                    lineEndIndices.push_back(glyphs.size() - 1);
                    characterCount = 0;
                    pen.x = 0.0;
                    pen.y += fontSize;
                    break;
                }

                if (c == d.charCode) {
                    auto size = Vec2D(d.boundingBoxSize.x * fontSize * entry.scale, d.boundingBoxSize.y * fontSize * entry.scale);
                    auto bearing = Vec2D(d.bearing.x * fontSize * entry.scale, d.bearing.y * fontSize * entry.scale);
                    auto advance = Vec2D(d.advance.x * fontSize * entry.scale, d.advance.y * fontSize * entry.scale);

                    auto x = pen.x + bearing.x;
                    auto y = pen.y - bearing.y;
                    auto xw = x + size.x;
                    auto yh = y + size.y;

                    Quad2dD quad = Quad2dD(Vec2D(x, yh), Vec2D(xw, yh), Vec2D(xw, y), Vec2D(x, y));

                    if (!box) {
                        box = BoundingBox(Coord(referencePoint.systemIdentifier, quad.topLeft.x, quad.topLeft.y, referencePoint.z));
                    }

                    box->addPoint(quad.topLeft.x, quad.topLeft.y, referencePoint.z);
                    box->addPoint(quad.topRight.x, quad.topRight.y, referencePoint.z);
                    box->addPoint(quad.bottomLeft.x, quad.bottomLeft.y, referencePoint.z);
                    box->addPoint(quad.bottomRight.x, quad.bottomRight.y, referencePoint.z);

                    glyphs.push_back(GlyphDescription(quad, d.uv));
                    pen.x += advance.x * (1.0 + letterSpacing);
                    characterCount += 1;
                    break;
                }
            }
        }
    }

    lineEndIndices.push_back(glyphs.size() - 1);

    if (!glyphs.empty()) {

        Vec2D min(box->min.x, box->min.y);
        Vec2D max(box->max.x, box->max.y);

        box = std::nullopt;

        Vec2D size((max.x - min.x), (max.y - min.y));

        switch (textInfo->getTextJustify()) {
            case TextJustify::LEFT:
                //Nothing to do here
                break;
            case TextJustify::CENTER: {
                size_t lineStart = 0;
                for (auto const lineEndIndex: lineEndIndices) {
                    double lineWidth = glyphs[lineEndIndex].frame.topRight.x - glyphs[lineStart].frame.topLeft.x;
                    double widthDeltaHalfed = (size.x - lineWidth) / 2.0;
                    for(size_t i = lineStart; i <= lineEndIndex; i++) {
                        glyphs[i].frame.bottomRight.x += widthDeltaHalfed;
                        glyphs[i].frame.bottomLeft.x += widthDeltaHalfed;
                        glyphs[i].frame.topRight.x += widthDeltaHalfed;
                        glyphs[i].frame.topLeft.x += widthDeltaHalfed;
                    }
                    lineStart = lineEndIndex + 1;
                }
                break;
            }

            case TextJustify::RIGHT:{
                size_t lineStart = 0;
                for (auto const lineEndIndex: lineEndIndices) {
                    double lineWidth = glyphs[lineEndIndex].frame.topRight.x - glyphs[lineStart].frame.topLeft.x;
                    double widthDelta = (size.x - lineWidth);
                    for(size_t i = lineStart; i <= lineEndIndex; i++) {
                        glyphs[i].frame.bottomRight.x += widthDelta;
                        glyphs[i].frame.bottomLeft.x += widthDelta;
                        glyphs[i].frame.topRight.x += widthDelta;
                        glyphs[i].frame.topLeft.x += widthDelta;
                    }
                    lineStart = lineEndIndex + 1;
                }
                break;
            }
        }

        double offsetMultiplier = fontSize + fontData.info.ascender + fontData.info.descender;

        Vec2D textOffset(offset.x * offsetMultiplier, offset.y * offsetMultiplier);

        Vec2D offset(0.0, 0.0);

        switch (textInfo->getTextAnchor()) {
            case Anchor::CENTER:
                offset.x -= size.x / 2.0 - textOffset.x;
                offset.y -= size.y / 2.0 - textOffset.y;
                break;
            case Anchor::LEFT:
                offset.x += textOffset.x;
                offset.y -= size.y / 2.0 - textOffset.y;
                break;
            case Anchor::RIGHT:
                offset.x -= size.x - textOffset.x;
                offset.y -= size.y / 2.0 - textOffset.y;
                break;
            case Anchor::TOP:
                offset.x -= size.x / 2.0 - textOffset.x;
                offset.y -= -textOffset.y;
                break;
            case Anchor::BOTTOM:
                offset.x -= size.x / 2.0 - textOffset.x;
                offset.y -= size.y - textOffset.y + fontSize * 0.5;
                break;
            case Anchor::TOP_LEFT:
                offset.x -= -textOffset.x;
                offset.y -= -textOffset.y;
                break;
            case Anchor::TOP_RIGHT:
                offset.x -= size.x -textOffset.x;
                offset.y -= -textOffset.y;
                break;
            case Anchor::BOTTOM_LEFT:
                offset.x -= -textOffset.x;
                offset.y -= size.y - textOffset.y;
                break;
            case Anchor::BOTTOM_RIGHT:
                offset.x -= size.x -textOffset.x;
                offset.y -= size.y - textOffset.y;
                break;
            default:
                break;
        }

        for (auto &glyph: glyphs) {
            glyph.frame.bottomRight.x += referencePoint.x + offset.x - min.x;
            glyph.frame.bottomLeft.x += referencePoint.x + offset.x - min.x;
            glyph.frame.topRight.x += referencePoint.x + offset.x - min.x;
            glyph.frame.topLeft.x += referencePoint.x + offset.x - min.x;

            glyph.frame.bottomRight.y += referencePoint.y + offset.y - min.y;
            glyph.frame.bottomLeft.y += referencePoint.y + offset.y - min.y;
            glyph.frame.topRight.y += referencePoint.y + offset.y - min.y;
            glyph.frame.topLeft.y += referencePoint.y + offset.y - min.y;

            if (!box) {
                box = BoundingBox(Coord(referencePoint.systemIdentifier, glyph.frame.topLeft.x, glyph.frame.topLeft.y, referencePoint.z));
            }

            box->addPoint(glyph.frame.topLeft.x, glyph.frame.topLeft.y, referencePoint.z);
            box->addPoint(glyph.frame.topRight.x, glyph.frame.topRight.y, referencePoint.z);
            box->addPoint(glyph.frame.bottomLeft.x, glyph.frame.bottomLeft.y, referencePoint.z);
            box->addPoint(glyph.frame.bottomRight.x, glyph.frame.bottomRight.y, referencePoint.z);

        }

        if (text) {
            text->setTexts({TextDescription(glyphs)});
        }
    }

    boundingBox = box ? RectCoord(box->min, box->max) : RectCoord(referencePoint, referencePoint);
}

#ifdef DRAW_TEXT_LETTER_BOXES
std::vector<Quad2dD> TextLayerObject::getLetterBoxes() {
    return letterBoxes;
}
#endif

void TextLayerObject::layoutLine(float scale) {
    if(lineCoordinates == std::nullopt) {
        return;
    }

#ifdef DRAW_TEXT_LETTER_BOXES
    letterBoxes.clear();
#endif

    float fontSize = fontData.info.size * scale;

    std::vector<GlyphDescription> glyphs = {};
    std::optional<BoundingBox> box = std::nullopt;

    int characterCount = 0;
    std::vector<size_t> lineEndIndices;

    auto currentIndex = findReferencePointIndices();

    double size = 0;
    for (const auto &entry: textInfo->getText()) {
        for (const auto &c : TextHelper::splitWstring(entry.text)) {
            for (const auto &d : fontData.glyphs) {
                if (c == " " || c == "\n") {
                    size += fontData.info.spaceAdvance * fontSize * entry.scale;
                    break;
                } else if (c == d.charCode) {
                    auto advance = Vec2D(d.advance.x * fontSize * entry.scale, d.advance.y * fontSize * entry.scale);
                    size += advance.x * (1.0 + letterSpacing);
                    break;
                }
            }
        }
    }

    currentIndex = indexAtDistance(currentIndex, -size * 0.5);

    for (const auto &entry: textInfo->getText()) {
        for (const auto &c : TextHelper::splitWstring(entry.text)) {
            for (const auto &d : fontData.glyphs) {
                if (c == " " || c == "\n") {
                    currentIndex = indexAtDistance(currentIndex, fontData.info.spaceAdvance * fontSize * entry.scale);
                    characterCount += 1;
                    break;
                } else if (c == d.charCode) {
                    auto size = Vec2D(d.boundingBoxSize.x * fontSize * entry.scale, d.boundingBoxSize.y * fontSize * entry.scale);
                    auto bearing = Vec2D(d.bearing.x * fontSize * entry.scale, d.bearing.y * fontSize * entry.scale);
                    auto advance = Vec2D(d.advance.x * fontSize * entry.scale, d.advance.y * fontSize * entry.scale);

                    // Punkt auf Linie
                    auto p = converter->convertToRenderSystem(pointAtIndex(currentIndex));

                    // get before and after to calculate angle
                    auto before = pointAtIndex(indexAtDistance(currentIndex, -size.x * 0.5));
                    auto after = pointAtIndex(indexAtDistance(currentIndex, size.x * 0.5));

                    double angle = atan2((before.y - after.y), -(before.x - after.x));
                    angle *= (180.0 / M_PI);

                    auto x = p.x + bearing.x;
                    auto y = p.y - bearing.y;

                    auto xw = x + size.x;
                    auto yh = y + size.y;

                    currentIndex = indexAtDistance(currentIndex, advance.x * (1.0 + letterSpacing));

                    auto tl = Vec2D(x, yh);
                    auto tr = Vec2D(xw, yh);
                    auto bl = Vec2D(x, y);
                    auto br = Vec2D(xw, y);

                    Quad2dD quad = Quad2dD(tl, tr, br, bl);
                    quad = TextHelper::rotateQuad2d(quad, Vec2D(p.x, p.y), angle);

                    auto dy = Vec2DHelper::normalize(Vec2D(quad.bottomLeft.x - quad.topLeft.x, quad.bottomLeft.y - quad.topLeft.y));
                    // TODO: 0.3 looks good, is there a better value?
                    dy.x *= 0.3 * fontSize;
                    dy.y *= 0.3 * fontSize;

                    quad.topLeft = quad.topLeft - dy;
                    quad.bottomLeft = quad.bottomLeft - dy;
                    quad.topRight = quad.topRight - dy;
                    quad.bottomRight = quad.bottomRight - dy;

                    if (!box) {
                        box = BoundingBox(Coord(referencePoint.systemIdentifier, quad.topLeft.x, quad.topLeft.y, referencePoint.z));
                    }

                    box->addPoint(quad.topLeft.x, quad.topLeft.y, referencePoint.z);
                    box->addPoint(quad.topRight.x, quad.topRight.y, referencePoint.z);
                    box->addPoint(quad.bottomLeft.x, quad.bottomLeft.y, referencePoint.z);
                    box->addPoint(quad.bottomRight.x, quad.bottomRight.y, referencePoint.z);

                    glyphs.push_back(GlyphDescription(quad, d.uv));
                    characterCount += 1;

#ifdef DRAW_TEXT_LETTER_BOXES
                    letterBoxes.push_back(quad);
#endif
                    break;
                }
            }
        }
    }

    lineEndIndices.push_back(glyphs.size() - 1);

    if (!glyphs.empty()) {
        Vec2D min(box->min.x, box->min.y);
        Vec2D max(box->max.x, box->max.y);

        box = std::nullopt;

        for (auto &glyph: glyphs) {
            if (!box) {
                box = BoundingBox(Coord(referencePoint.systemIdentifier, glyph.frame.topLeft.x, glyph.frame.topLeft.y, referencePoint.z));
            }

            box->addPoint(glyph.frame.topLeft.x, glyph.frame.topLeft.y, referencePoint.z);
            box->addPoint(glyph.frame.topRight.x, glyph.frame.topRight.y, referencePoint.z);
            box->addPoint(glyph.frame.bottomLeft.x, glyph.frame.bottomLeft.y, referencePoint.z);
            box->addPoint(glyph.frame.bottomRight.x, glyph.frame.bottomRight.y, referencePoint.z);
        }

        if (text) {
            text->setTexts({TextDescription(glyphs)});
        }
    }

    boundingBox = box ? RectCoord(box->min, box->max) : RectCoord(referencePoint, referencePoint);
}

std::pair<int, double> TextLayerObject::findReferencePointIndices() {
    auto point = referencePoint;
    auto distance = std::numeric_limits<double>::max();

    double tMin = 0.0f;
    int iMin = 0;

    for(int i=1; i<lineCoordinates->size(); ++i) {
        auto start = converter->convertToRenderSystem(lineCoordinates->at(i-1));
        auto end = converter->convertToRenderSystem(lineCoordinates->at(i));

        auto length = Vec2DHelper::distance(Vec2D(start.x, start.y), Vec2D(end.x, end.y));

        double t = 0.0;
        if(length > 0) {
            auto dot = Vec2D(point.x - start.x, point.y - start.y) * Vec2D(end.x - start.x, end.y - start.y);
            t = dot / (length * length);
        }

        auto proj = Vec2D(start.x + t * (end.x - start.x), start.y + t * (end.y - start.y));
        auto dist = Vec2DHelper::distance(proj, Vec2D(point.x, point.y));

        if(dist < distance && t >= 0.0 && t <= 1.0) {
            tMin = t;
            iMin = i-1;
            distance = dist;
        }
    }

    return std::make_pair(iMin, tMin);
}

Coord TextLayerObject::pointAtIndex(const std::pair<int, double> &index) {
    auto s = (*lineCoordinates)[index.first];
    auto e = (*lineCoordinates)[index.first + 1 < lineCoordinates->size() ? (index.first + 1) : index.first];
    return Coord(s.systemIdentifier, s.x + (e.x - s.x) * index.second, s.y + (e.y - s.y) * index.second, s.z + (e.z - s.z) * index.second);
}

std::pair<int, double> TextLayerObject::indexAtDistance(const std::pair<int, double> &index, double distance) {
    auto current = converter->convertToRenderSystem(pointAtIndex(index));
    auto currentIndex = index;
    auto dist = std::abs(distance);

    if(distance >= 0) {
        auto start = std::min(index.first + 1, (int)lineCoordinates->size() - 1);

        double d = 0.0;
        for(int i=start; i<lineCoordinates->size(); i++) {
            auto next = converter->convertToRenderSystem((*lineCoordinates)[i]);

            d = Vec2DHelper::distance(Vec2D(current.x, current.y), Vec2D(next.x, next.y));

            if(dist > d) {
                dist -= d;
                current = next;
                currentIndex = std::make_pair(i, 0.0);
            } else {
                float dFactor = dist / d;
                return std::make_pair(currentIndex.first, currentIndex.second + dFactor * (1.0 - currentIndex.second));
            }
        }
    } else {
        auto start = index.first;

        for(int i=start; i>=0; i--) {
            auto next = converter->convertToRenderSystem((*lineCoordinates)[i]);

            auto d = Vec2DHelper::distance(Vec2D(current.x, current.y), Vec2D(next.x, next.y));

            if(dist > d) {
                dist -= d;
                current = next;
                currentIndex = std::make_pair(i, 0.0);
            } else {
                float dFactor = dist / d;

                if(i == currentIndex.first) {
                    return std::make_pair(i, currentIndex.second - currentIndex.second * dFactor);
                } else {
                    return std::make_pair(i, 1.0 - dFactor);
                }
            }
        }

    }

    return currentIndex;
}
