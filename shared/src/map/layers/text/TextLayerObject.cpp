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

TextLayerObject::TextLayerObject(const std::shared_ptr<TextInterface> &text, const std::shared_ptr<TextInfoInterface> &textInfo,const std::shared_ptr<TextShaderInterface> &shader, const std::shared_ptr<MapInterface> &mapInterface, const FontData& fontData, const Vec2F &offset, double lineHeight, double letterSpacing, int64_t maxCharacterWidth, double maxCharacterAngle, SymbolAlignment rotationAlignment)
: text(text),
  textGraphicsObject(text ? text->asGraphicsObject() : nullptr),
  textInfo(textInfo),
  lineCoordinates(textInfo->getLineCoordinates()),
  converter(mapInterface->getCoordinateConverterHelper()),
  camera(mapInterface->getCamera()),
  shader(shader),
  referencePoint(Coord(0, 0.0, 0.0, 0.0)),
  fontData(fontData),
  offset(offset),
  lineHeight(lineHeight),
  letterSpacing(letterSpacing),
  maxCharacterAngle(maxCharacterAngle),
  boundingBox(Coord(0, 0.0, 0.0, 0.0), Coord(0, 0.0, 0.0, 0.0)), rotationAlignment(rotationAlignment)
{
    if (text) {
        renderConfig = { std::make_shared<RenderConfig>(text->asGraphicsObject(), 1) };
    }

    referencePoint = converter->convertToRenderSystem(textInfo->getCoordinate());
    referenceSize = fontData.info.size;

    for (const auto &d : fontData.glyphs) {
        if(d.charCode == " ") {
            spaceAdvance = d.advance.x;
            break;
        }
    }

    std::vector<BreakResult> breaks = {};
    if(textInfo->getSymbolPlacement() == TextSymbolPlacement::POINT) {
        std::vector<std::string> letters;

        for (const auto &entry: textInfo->getText()) {
            for (const auto &c : TextHelper::splitWstring(entry.text)) {
                letters.push_back(c);
            }
        }

        breaks = TextHelper::bestBreakIndices(letters, maxCharacterWidth);
    }

    int currentLetterIndex = 0;
    for (const auto &entry: textInfo->getText()) {
        for (const auto &c : TextHelper::splitWstring(entry.text)) {
            int index = -1;
            bool found = false;

            int i = 0;
            for (const auto &d : fontData.glyphs) {
                if(c == d.charCode) {
                    index = i;
                    found = true;
                    break;
                }

                ++i;
            }



            if(textInfo->getSymbolPlacement() == TextSymbolPlacement::POINT) {
                // check for line breaks in point texts
                auto it = std::find_if(breaks.begin(), breaks.end(), [&](const auto& v) { return v.index == currentLetterIndex; });
                if(it != breaks.end()) {
                    // add line break
                    if(it->keepLetter && found) {
                        splittedTextInfo.emplace_back(index, entry.scale);
                    }
                    // use -1 as line break
                    splittedTextInfo.emplace_back(-1, entry.scale);
                } else {
                    // just add it
                    if(found) {
                        splittedTextInfo.emplace_back(index, entry.scale);
                    }
                }
            } else {
                // non-point symbols: just add it if found, no line breaks possible
                if(found) {
                    splittedTextInfo.emplace_back(index, entry.scale);
                }
            }

            currentLetterIndex++;
        }
    }

    if(lineCoordinates) {
        for(auto& l : *lineCoordinates) {
            renderLineCoordinates.push_back(converter->convertToRenderSystem(l));
        }
    }
}

std::vector<std::shared_ptr<RenderConfigInterface>> TextLayerObject::getRenderConfig() {
    return renderConfig;
}

void TextLayerObject::update(float scale) {
    update(scale, true);
}

void TextLayerObject::layout(float scale) {
    update(scale, false);
}

void TextLayerObject::update(float scale, bool updateObject) {
    switch(textInfo->getSymbolPlacement()) {
        case TextSymbolPlacement::POINT: {
            layoutPoint(scale, updateObject);
            break;
        }

        case TextSymbolPlacement::LINE_CENTER:
        case TextSymbolPlacement::LINE: {
            auto rotatedFactor = layoutLine(scale, updateObject);

            if (rotatedFactor > 0.5 && lineCoordinates && !rotated) {
                std::reverse((*lineCoordinates).begin(), (*lineCoordinates).end());
                std::reverse(renderLineCoordinates.begin(), renderLineCoordinates.end());

                layoutLine(scale, updateObject);
                rotated = true;
            }

            break;
        }
    }
}

void TextLayerObject::layoutPoint(float scale, bool updateObject) {
    float fontSize = fontData.info.size * scale;
    auto pen = Vec2D(0.0, 0.0);

    if (scale != lastScale) {
        std::optional<BoundingBox> box = std::nullopt;

        auto numGlyphs = splittedTextInfo.size();

        vertices.clear();
        vertices.reserve(numGlyphs * 24);
        indices.clear();
        indices.reserve(numGlyphs * 6);

        int indicesStart = 0;

        std::vector<size_t> lineEndIndices;

        for(auto& i : splittedTextInfo) {
            if(i.glyphIndex >= 0) {
                auto &d = fontData.glyphs[i.glyphIndex];
                auto size = Vec2D(d.boundingBoxSize.x * fontSize * i.scale, d.boundingBoxSize.y * fontSize * i.scale);
                auto bearing = Vec2D(d.bearing.x * fontSize * i.scale, d.bearing.y * fontSize * i.scale);
                auto advance = Vec2D(d.advance.x * fontSize * i.scale, d.advance.y * fontSize * i.scale);

                if(d.charCode != " ") {
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

                    vertices.push_back(quad.bottomLeft.x);
                    vertices.push_back(quad.bottomLeft.y);
                    vertices.push_back(d.uv.bottomLeft.x);
                    vertices.push_back(d.uv.bottomLeft.y);
                    vertices.push_back(0.0);
                    vertices.push_back(0.0);

                    vertices.push_back(quad.topLeft.x);
                    vertices.push_back(quad.topLeft.y);
                    vertices.push_back(d.uv.topLeft.x);
                    vertices.push_back(d.uv.topLeft.y);
                    vertices.push_back(0.0);
                    vertices.push_back(0.0);

                    vertices.push_back(quad.topRight.x);
                    vertices.push_back(quad.topRight.y);
                    vertices.push_back(d.uv.topRight.x);
                    vertices.push_back(d.uv.topRight.y);
                    vertices.push_back(0.0);
                    vertices.push_back(0.0);

                    vertices.push_back(quad.bottomRight.x);
                    vertices.push_back(quad.bottomRight.y);
                    vertices.push_back(d.uv.bottomRight.x);
                    vertices.push_back(d.uv.bottomRight.y);
                    vertices.push_back(0.0);
                    vertices.push_back(0.0);

                    indices.push_back(0 + indicesStart);
                    indices.push_back(1 + indicesStart);
                    indices.push_back(2 + indicesStart);
                    indices.push_back(0 + indicesStart);
                    indices.push_back(2 + indicesStart);
                    indices.push_back(3 + indicesStart);

                    indicesStart += 4;
                }

                pen.x += advance.x * (1.0 + letterSpacing);
            } else if(i.glyphIndex == -1) {
                lineEndIndices.push_back((vertices.size() / 24) - 1);
                pen.x = 0.0;
                pen.y += fontSize;
            }
        }

        lineEndIndices.push_back((vertices.size() / 24) - 1);

        if (!vertices.empty()) {
            Vec2D min(box->min.x, box->min.y);
            Vec2D max(box->max.x, box->max.y);
            Vec2D size((max.x - min.x), (max.y - min.y));

            switch (textInfo->getTextJustify()) {
                case TextJustify::AUTO:
                case TextJustify::LEFT:
                    //Nothing to do here
                    break;
                case TextJustify::RIGHT:
                case TextJustify::CENTER: {
                    size_t lineStart = 0;

                    for (auto const lineEndIndex: lineEndIndices) {
                        double lineWidth = vertices[lineEndIndex * 24 + 2 * 6] - vertices[lineStart * 24 + 6];
                        auto factor = textInfo->getTextJustify() == TextJustify::CENTER ? 2.0 : 1.0;
                        double delta = (size.x - lineWidth) / factor;

                        for(size_t i = lineStart; i <= lineEndIndex; i++) {
                            vertices[i * 24] += delta;
                            vertices[i * 24 + 6] += delta;
                            vertices[i * 24 + 12] += delta;
                            vertices[i * 24 + 18] += delta;
                        }

                        lineStart = lineEndIndex + 1;
                    }

                    break;
                }
            }

            double offsetMultiplier = fontSize;

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

            box = BoundingBox(referencePoint.systemIdentifier);

            auto dx = referencePoint.x + offset.x - min.x;
            auto dy = referencePoint.y + offset.y - min.y;

            for(auto i=0; i<vertices.size(); i+=6) {
                vertices[i] += dx;
                vertices[i+1] += dy;

                box->addPoint(vertices[i], vertices[i+1], referencePoint.z);
            }
        }

        boundingBox = box ? RectCoord(box->min, box->max) : RectCoord(referencePoint, referencePoint);

        lastScale = scale;
    }

    if (text && updateObject) {
        text->setTextsShared(SharedBytes((int64_t) vertices.data(), (int32_t) vertices.size(), (int32_t) sizeof(float)),
                             SharedBytes((int64_t) indices.data(), (int32_t) indices.size(), (int32_t) sizeof(int16_t)));
    }
}

#ifdef DRAW_TEXT_LETTER_BOXES
std::vector<Quad2dD> TextLayerObject::getLetterBoxes() {
    return letterBoxes;
}
#endif

float TextLayerObject::layoutLine(float scale, bool updateObject) {
    if(lineCoordinates == std::nullopt) {
        return 0;
    }

#ifdef DRAW_TEXT_LETTER_BOXES
    letterBoxes.clear();
#endif

    float fontSize = fontData.info.size * scale;

    std::optional<BoundingBox> box = std::nullopt;

    auto numGlyphs = splittedTextInfo.size();
    std::vector<float> vertices;
    vertices.reserve(numGlyphs * 24);
    std::vector<int16_t> indices;
    indices.reserve(numGlyphs * 6);

    auto currentIndex = findReferencePointIndices();

    double size = 0;

    for(auto& i : splittedTextInfo) {
        if(i.glyphIndex < 0) {
            size += spaceAdvance * fontSize * i.scale;
        } else {
            auto &d = fontData.glyphs[i.glyphIndex];
            auto advance = Vec2D(d.advance.x * fontSize * i.scale, d.advance.y * fontSize * i.scale);
            size += advance.x * (1.0 + letterSpacing);
        }
    }

    currentIndex = indexAtDistance(currentIndex, -size * 0.5);

    int total = 0;
    int rotated = 0;

    int indicesStart = 0;
    int index = 0;
    double lastAngle = 0.0;

    double lineCenteringParameter = -fontData.info.base / fontData.info.lineHeight;

    for(auto &i : splittedTextInfo) {
        if(i.glyphIndex < 0) {
            currentIndex = indexAtDistance(currentIndex, spaceAdvance * fontSize * i.scale);
            lastAngle = 0;
            index = 0;
        } else {
            auto& d = fontData.glyphs[i.glyphIndex];
            auto size = Vec2D(d.boundingBoxSize.x * fontSize * i.scale, d.boundingBoxSize.y * fontSize * i.scale);
            auto bearing = Vec2D(d.bearing.x * fontSize * i.scale, d.bearing.y * fontSize * i.scale);
            auto advance = Vec2D(d.advance.x * fontSize * i.scale, d.advance.y * fontSize * i.scale);

            // Punkt auf Linie
            auto p = pointAtIndex(currentIndex);

            // get before and after to calculate angle
            auto before = pointAtIndex(indexAtDistance(currentIndex, -size.x * 0.5), false);
            auto after = pointAtIndex(indexAtDistance(currentIndex, size.x * 0.5), false);

            double angle = atan2((before.y - after.y), -(before.x - after.x));
            angle *= (180.0 / M_PI);

            if(index > 1) {
                auto diff = fabs(lastAngle - angle);
                auto min = std::min(360.0 - diff, diff);

                if(min > maxCharacterAngle) {
                    vertices.clear();
                    break;
                }
            }

            lastAngle = angle;

            auto x = p.x + bearing.x;
            auto y = p.y - bearing.y;

            rotated += (angle > 90 || angle < -90) ? 1 : 0;
            total++;

            auto xw = x + size.x;
            auto yh = y + size.y;

            auto lastIndex = currentIndex;
            currentIndex = indexAtDistance(currentIndex, advance.x * (1.0 + letterSpacing));

            // if we are at the end, and we were at the end (lastIndex), then clear and skip
            if(currentIndex.first == renderLineCoordinates.size() - 1 && lastIndex.first == currentIndex.first && (lastIndex.second == currentIndex.second)) {
                vertices.clear();
                break;
            }

            auto tl = Vec2D(x, yh);
            auto tr = Vec2D(xw, yh);
            auto bl = Vec2D(x, y);
            auto br = Vec2D(xw, y);

            Quad2dD quad = Quad2dD(tl, tr, br, bl);
            quad = TextHelper::rotateQuad2d(quad, Vec2D(p.x, p.y), angle);

            auto dy = Vec2DHelper::normalize(Vec2D(quad.bottomLeft.x - quad.topLeft.x, quad.bottomLeft.y - quad.topLeft.y));
            dy.x *= lineCenteringParameter * fontSize;
            dy.y *= lineCenteringParameter * fontSize;

            quad.topLeft = quad.topLeft - dy;
            quad.bottomLeft = quad.bottomLeft - dy;
            quad.topRight = quad.topRight - dy;
            quad.bottomRight = quad.bottomRight - dy;

            vertices.push_back(quad.bottomLeft.x);
            vertices.push_back(quad.bottomLeft.y);
            vertices.push_back(d.uv.bottomLeft.x);
            vertices.push_back(d.uv.bottomLeft.y);
            vertices.push_back(0.0);
            vertices.push_back(0.0);

            vertices.push_back(quad.topLeft.x);
            vertices.push_back(quad.topLeft.y);
            vertices.push_back(d.uv.topLeft.x);
            vertices.push_back(d.uv.topLeft.y);
            vertices.push_back(0.0);
            vertices.push_back(0.0);

            vertices.push_back(quad.topRight.x);
            vertices.push_back(quad.topRight.y);
            vertices.push_back(d.uv.topRight.x);
            vertices.push_back(d.uv.topRight.y);
            vertices.push_back(0.0);
            vertices.push_back(0.0);

            vertices.push_back(quad.bottomRight.x);
            vertices.push_back(quad.bottomRight.y);
            vertices.push_back(d.uv.bottomRight.x);
            vertices.push_back(d.uv.bottomRight.y);
            vertices.push_back(0.0);
            vertices.push_back(0.0);

            indices.push_back(0 + indicesStart);
            indices.push_back(1 + indicesStart);
            indices.push_back(2 + indicesStart);
            indices.push_back(0 + indicesStart);
            indices.push_back(2 + indicesStart);
            indices.push_back(3 + indicesStart);

            indicesStart += 4;
            index += 1;

#ifdef DRAW_TEXT_LETTER_BOXES
            letterBoxes.push_back(quad);
#endif
        }
    }

    if (!vertices.empty()) {
        box = std::nullopt;

        for (auto i=0; i<vertices.size(); i += 6) {
            if (!box) {
                box = BoundingBox(Coord(referencePoint.systemIdentifier, vertices[i], vertices[i+1], referencePoint.z));
            }

            box->addPoint(vertices[i], vertices[i+1], referencePoint.z);
        }

        if (text && updateObject) {
            text->setTextsShared(SharedBytes((int64_t)vertices.data(), (int32_t)vertices.size(), (int32_t)sizeof(float)),
                                 SharedBytes((int64_t)indices.data(), (int32_t)indices.size(), (int32_t)sizeof(int16_t)));
        }
    }

    boundingBox = box ? RectCoord(box->min, box->max) : RectCoord(referencePoint, referencePoint);

    return (double)rotated / (double)total;
}

std::pair<int, double> TextLayerObject::findReferencePointIndices() {
    auto point = referencePoint;
    auto distance = std::numeric_limits<double>::max();

    double tMin = 0.0f;
    int iMin = 0;

    for(int i=1; i<renderLineCoordinates.size(); ++i) {
        auto start = renderLineCoordinates[i-1];
        auto end = renderLineCoordinates[i];

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

Coord TextLayerObject::pointAtIndex(const std::pair<int, double> &index, bool useRender) {
    auto s = useRender ? renderLineCoordinates[index.first] : (*lineCoordinates)[index.first];
    auto e = useRender ?  renderLineCoordinates[index.first + 1 < renderLineCoordinates.size() ? (index.first + 1) : index.first] : (*lineCoordinates)[index.first + 1 < renderLineCoordinates.size() ? (index.first + 1) : index.first];
    return Coord(s.systemIdentifier, s.x + (e.x - s.x) * index.second, s.y + (e.y - s.y) * index.second, s.z + (e.z - s.z) * index.second);
}

std::pair<int, double> TextLayerObject::indexAtDistance(const std::pair<int, double> &index, double distance) {
    auto current = pointAtIndex(index);
    auto currentIndex = index;
    auto dist = std::abs(distance);

    if(distance >= 0) {
        auto start = std::min(index.first + 1, (int)renderLineCoordinates.size() - 1);

        for(int i=start; i<renderLineCoordinates.size(); i++) {
            auto &next = renderLineCoordinates[i];

            double d = Vec2DHelper::distance(Vec2D(current.x, current.y), Vec2D(next.x, next.y));

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
            auto &next = renderLineCoordinates[i];

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
