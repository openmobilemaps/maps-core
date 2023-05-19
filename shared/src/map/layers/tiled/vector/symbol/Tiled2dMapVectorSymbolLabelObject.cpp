/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorSymbolLabelObject.h"
#include "TextHelper.h"

Tiled2dMapVectorSymbolLabelObject::Tiled2dMapVectorSymbolLabelObject(const std::shared_ptr<CoordinateConversionHelperInterface> &converter,
                                                                     const FeatureContext featureContext,
                                                                     const std::shared_ptr<SymbolVectorLayerDescription> &description,
                                                                     const std::vector<FormattedStringEntry> &text,
                                                                     const std::string &fullText,
                                                                     const ::Coord &coordinate,
                                                                     const std::optional<std::vector<Coord>> &lineCoordinates,
                                                                     const Anchor &textAnchor,
                                                                     const std::optional<double> &angle,
                                                                     const TextJustify &textJustify,
                                                                     const std::shared_ptr<FontLoaderResult> fontResult,
                                                                     const Vec2F &offset,
                                                                     const double lineHeight,
                                                                     const double letterSpacing,
                                                                     const int64_t maxCharacterWidth,
                                                                     const double maxCharacterAngle,
                                                                     const SymbolAlignment rotationAlignment,
                                                                     const TextSymbolPlacement &textSymbolPlacement):
textSymbolPlacement(textSymbolPlacement),
rotationAlignment(rotationAlignment),
featureContext(featureContext),
description(description),
lineHeight(lineHeight),
letterSpacing(letterSpacing),
maxCharacterAngle(maxCharacterAngle),
textJustify(textJustify),
textAnchor(textAnchor),
offset(offset),
fontResult(fontResult),
fullText(fullText),
lineCoordinates(lineCoordinates),
boundingBox(Coord("", 0.0, 0.0, 0.0), Coord("", 0.0, 0.0, 0.0)),
referencePoint(converter->convertToRenderSystem(coordinate)),
referenceSize(fontResult->fontData->info.size)
{
    auto spaceIt = std::find_if(fontResult->fontData->glyphs.begin(), fontResult->fontData->glyphs.end(), [](const auto& d) {
        return d.charCode == " ";
    });

    if (spaceIt != fontResult->fontData->glyphs.end()) {
        spaceAdvance = spaceIt->advance.x;
    }

    std::vector<BreakResult> breaks = {};
    if(textSymbolPlacement == TextSymbolPlacement::POINT) {
        std::vector<std::string> letters;

        for (const auto &entry: text) {
            const auto splitText = TextHelper::splitWstring(entry.text);
            std::copy(splitText.begin(), splitText.end(), std::back_inserter(letters));
        }

        breaks = TextHelper::bestBreakIndices(letters, maxCharacterWidth);
    }

    int currentLetterIndex = 0;
    for (const auto &entry: text) {
        for (const auto &c : TextHelper::splitWstring(entry.text)) {
            int index = -1;
            bool found = false;
            bool isSpace = false;

            int i = 0;
            for (const auto &d : fontResult->fontData->glyphs) {
                if(c == d.charCode) {
                    index = i;
                    found = true;
                    isSpace = d.charCode == " ";
                    break;
                }

                ++i;
            }

            if(textSymbolPlacement == TextSymbolPlacement::POINT) {
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
                        if (!isSpace) {
                            characterCount += 1;
                        }
                        splittedTextInfo.emplace_back(index, entry.scale);
                    }
                }
            } else {
                // non-point symbols: just add it if found, no line breaks possible
                if(found) {
                    if (!isSpace) {
                        characterCount += 1;
                    }
                    splittedTextInfo.emplace_back(index, entry.scale);
                }
            }

            currentLetterIndex++;
        }
    }

    if(lineCoordinates) {
        std::transform(lineCoordinates->begin(), lineCoordinates->end(), std::back_inserter(renderLineCoordinates),
                       [converter](const auto& l) {
            return converter->convertToRenderSystem(l);
        });
    }
}

int Tiled2dMapVectorSymbolLabelObject::getCharacterCount(){
    return characterCount;
}

void Tiled2dMapVectorSymbolLabelObject::setupProperties(std::vector<float> &textureCoordinates, std::vector<uint16_t> &styleIndices, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier) {
    for(auto &i : splittedTextInfo) {
        auto& d = fontResult->fontData->glyphs[i.glyphIndex];
        if(i.glyphIndex >= 0 && d.charCode != " ") {
            textureCoordinates[(4 * countOffset) + 0] = d.uv.topLeft.x;
            textureCoordinates[(4 * countOffset) + 1] = d.uv.bottomRight.y;
            textureCoordinates[(4 * countOffset) + 2] = d.uv.bottomRight.x - d.uv.topLeft.x;
            textureCoordinates[(4 * countOffset) + 3] = d.uv.topLeft.y - d.uv.bottomLeft.y;

            styleIndices[countOffset] = (uint16_t)styleOffset;
            countOffset += 1;
        }
    }
    styleOffset += 1;
}

void Tiled2dMapVectorSymbolLabelObject::updateProperties(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor, const bool collides, const double rotation) {
    auto evalContext = EvaluationContext(zoomIdentifier, featureContext);

    if (collides || !(description->minZoom <= zoomIdentifier && description->maxZoom >= zoomIdentifier)) {
        styles[(9 * styleOffset) + 3] = 0;
        styles[(9 * styleOffset) + 7] = 0;
        for (int i = 0; i != characterCount; i++) {
            positions[(2 * countOffset) + 0] = 0;
            positions[(2 * countOffset) + 1] = 0;
            scales[2 * (countOffset) + 0] = 0;
            scales[2 * (countOffset) + 1] = 0;
            countOffset += 1;
        }
        styleOffset += 1;
        return;
    }

    auto opacity = description->style.getTextOpacity(evalContext);
    auto textColor = description->style.getTextColor(evalContext);
    auto haloColor = description->style.getTextHaloColor(evalContext);
    auto haloWidth = description->style.getTextHaloWidth(evalContext);

    styles[(9 * styleOffset) + 0] = textColor.r; //R
    styles[(9 * styleOffset) + 1] = textColor.g; //G
    styles[(9 * styleOffset) + 2] = textColor.b; //B
    styles[(9 * styleOffset) + 3] = textColor.a * opacity; //A
    styles[(9 * styleOffset) + 4] = haloColor.r; //R
    styles[(9 * styleOffset) + 5] = haloColor.g; //G
    styles[(9 * styleOffset) + 6] = haloColor.b; //B
    styles[(9 * styleOffset) + 7] = haloColor.a * opacity; //A
    styles[(9 * styleOffset) + 8] = haloWidth;

    styleOffset += 1;

    if (opacity == 0.0) {
        countOffset += characterCount;
        return;
    }

    switch(textSymbolPlacement) {
        case TextSymbolPlacement::LINE_CENTER:
        case TextSymbolPlacement::POINT: {
            updatePropertiesPoint(positions, scales, rotations, styles, countOffset, styleOffset, zoomIdentifier, scaleFactor, rotation);
            break;
        }

        case TextSymbolPlacement::LINE: {

            if (rotationAlignment == SymbolAlignment::VIEWPORT) {
                updatePropertiesPoint(positions, scales, rotations, styles, countOffset, styleOffset, zoomIdentifier, scaleFactor, rotation);
            } else {
                auto rotatedFactor = updatePropertiesLine(positions, scales, rotations, styles, countOffset, styleOffset, zoomIdentifier, scaleFactor, rotation);

                if(rotatedFactor > 0.5 && lineCoordinates && !wasRotated) {
                    std::reverse((*lineCoordinates).begin(), (*lineCoordinates).end());
                    std::reverse(renderLineCoordinates.begin(), renderLineCoordinates.end());

                    countOffset -= characterCount;

                    updatePropertiesLine(positions, scales, rotations, styles, countOffset, styleOffset, zoomIdentifier, scaleFactor, rotation);
                    wasRotated = true;
                }
            }

            break;
        }
    }
}

void Tiled2dMapVectorSymbolLabelObject::updatePropertiesPoint(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor, const double rotation) {
    
    auto evalContext = EvaluationContext(zoomIdentifier, featureContext);
    
    float fontSize = scaleFactor * description->style.getTextSize(evalContext);
    
    auto pen = Vec2D(0.0, 0.0);
    
    std::optional<BoundingBox> box = std::nullopt;
    std::optional<BoundingBox> centerPosBox = std::nullopt;

    centerPositions.clear();
    centerPositions.reserve(characterCount);
    
    std::vector<size_t> lineEndIndices;

    const auto textAlignment = description->style.getTextRotationAlignment(evalContext);

    float angle;
    if (textAlignment == SymbolAlignment::MAP) {
        angle = description->style.getTextRotate(evalContext);
    } else {
        angle = -rotation;
    }
    
    for(auto& i : splittedTextInfo) {
        if(i.glyphIndex >= 0) {
            auto &d = fontResult->fontData->glyphs[i.glyphIndex];
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
                    box = BoundingBox(referencePoint.systemIdentifier);
                    centerPosBox = BoundingBox(referencePoint.systemIdentifier);
                }
                
                box->addPoint(quad.topLeft.x, quad.topLeft.y, referencePoint.z);
                box->addPoint(quad.topRight.x, quad.topRight.y, referencePoint.z);
                box->addPoint(quad.bottomLeft.x, quad.bottomLeft.y, referencePoint.z);
                box->addPoint(quad.bottomRight.x, quad.bottomRight.y, referencePoint.z);
                
                scales[2 * (countOffset + centerPositions.size()) + 0] = size.x;
                scales[2 * (countOffset + centerPositions.size()) + 1] = size.y;
                rotations[countOffset + centerPositions.size()] = -angle;

                centerPosBox->addPoint(x + size.x / 2, y + size.y / 2, referencePoint.z);
                centerPositions.push_back(Vec2D(x + size.x / 2,
                                                y + size.y / 2));


            }

            pen.x += advance.x * (1.0 + letterSpacing);
        } else if(i.glyphIndex == -1) {
            lineEndIndices.push_back(centerPositions.size() - 1);
            pen.x = 0.0;
            pen.y += fontSize;
        }
    }
    
    lineEndIndices.push_back(centerPositions.size() - 1);

    Vec2D min(box->min.x, box->min.y);
    Vec2D max(box->max.x, box->max.y);
    Vec2D size((max.x - min.x), (max.y - min.y));

    Vec2D centerMin(centerPosBox->min.x, centerPosBox->min.y);
    Vec2D centerMax(centerPosBox->max.x, centerPosBox->max.y);
    Vec2D centerSize((centerMax.x - centerMin.x), (centerMax.y - centerMin.y));

    switch (textJustify) {
        case TextJustify::LEFT:
            //Nothing to do here
            break;
        case TextJustify::RIGHT:
        case TextJustify::CENTER: {
            size_t lineStart = 0;

            for (auto const lineEndIndex: lineEndIndices) {
                double lineWidth = centerPositions[lineEndIndex].x - centerPositions[lineStart].x;
                auto factor = textJustify == TextJustify::CENTER ? 2.0 : 1.0;
                double delta = (centerSize.x - lineWidth) / factor;

                for(size_t i = lineStart; i <= lineEndIndex; i++) {
                    centerPositions[i].x += delta;
                }

                lineStart = lineEndIndex + 1;
            }

            break;
        }
    }

    double offsetMultiplier = fontSize;

    Vec2D textOffset(offset.x * offsetMultiplier, offset.y * offsetMultiplier);

    Vec2D anchorOffset(0.0, 0.0);

    switch (textAnchor) {
        case Anchor::CENTER:
            anchorOffset.x -= size.x / 2.0 - textOffset.x;
            anchorOffset.y -= size.y / 2.0 + textOffset.y;
            break;
        case Anchor::LEFT:
            anchorOffset.x += textOffset.x;
            anchorOffset.y -= size.y / 2.0 + textOffset.y * 1000;
            break;
        case Anchor::RIGHT:
            anchorOffset.x -= size.x - textOffset.x;
            anchorOffset.y -= size.y / 2.0 + textOffset.y;
            break;
        case Anchor::TOP:
            anchorOffset.x -= size.x / 2.0 - textOffset.x;
            anchorOffset.y -= textOffset.y;
            break;
        case Anchor::BOTTOM:
            anchorOffset.x -= size.x / 2.0 - textOffset.x;
            anchorOffset.y -= size.y + textOffset.y + fontSize * 0.5;
            break;
        case Anchor::TOP_LEFT:
            anchorOffset.x -= -textOffset.x;
            anchorOffset.y -= textOffset.y;
            break;
        case Anchor::TOP_RIGHT:
            anchorOffset.x -= size.x -textOffset.x;
            anchorOffset.y -= textOffset.y;
            break;
        case Anchor::BOTTOM_LEFT:
            anchorOffset.x -= -textOffset.x;
            anchorOffset.y -= size.y + textOffset.y;
            break;
        case Anchor::BOTTOM_RIGHT:
            anchorOffset.x -= size.x -textOffset.x;
            anchorOffset.y -= size.y + textOffset.y;
            break;
        default:
            break;
    }

    box = BoundingBox(referencePoint.systemIdentifier);

    anchorOffset = Vec2DHelper::rotate(anchorOffset, Vec2D(0, 0), angle);

    auto dx = referencePoint.x + anchorOffset.x - min.x;
    auto dy = referencePoint.y + anchorOffset.y - min.y;

    assert(centerPositions.size() == characterCount);

    for(auto const centerPosition: centerPositions) {
        auto rotated = Vec2DHelper::rotate(centerPosition, Vec2D(0, 0), angle);

        positions[2 * countOffset + 0] = rotated.x + dx;
        positions[2 * countOffset + 1] = rotated.y + dy;

        const float scaleXH = scales[2 * countOffset + 0] / 2.0;
        const float scaleYH = scales[2 * countOffset + 1] / 2.0;

        box->addPoint(dx + rotated.x - scaleXH, dy + rotated.y - scaleYH, referencePoint.z);
        box->addPoint(dx + rotated.x + scaleXH, dy + rotated.y - scaleYH, referencePoint.z);
        box->addPoint(dx + rotated.x - scaleXH, dy + rotated.y + scaleYH, referencePoint.z);
        box->addPoint(dx + rotated.x + scaleXH, dy + rotated.y + scaleYH, referencePoint.z);

        countOffset += 1;
    }

    boundingBox = box ? RectCoord(box->min, box->max) : RectCoord(referencePoint, referencePoint);
    
    const float padding = description->style.getTextPadding(evalContext) * scaleFactor;

    boundingBox.topLeft.x -= padding;
    boundingBox.topLeft.y -= padding;

    boundingBox.bottomRight.x += padding;
    boundingBox.bottomRight.y += padding;
}

double Tiled2dMapVectorSymbolLabelObject::updatePropertiesLine(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor, const double rotation) {
    if(lineCoordinates == std::nullopt) {
        countOffset += characterCount;
        return 0;
    }

    auto evalContext = EvaluationContext(zoomIdentifier, featureContext);

    float fontSize = scaleFactor * description->style.getTextSize(evalContext);

    std::optional<BoundingBox> box = std::nullopt;

    centerPositions.clear();
    centerPositions.reserve(characterCount);

    auto currentIndex = findReferencePointIndices();

    double size = 0;

    for(auto& i : splittedTextInfo) {
        if(i.glyphIndex < 0) {
            size += spaceAdvance * fontSize * i.scale;
        } else {
            auto &d = fontResult->fontData->glyphs[i.glyphIndex];
            auto advance = Vec2D(d.advance.x * fontSize * i.scale, d.advance.y * fontSize * i.scale);
            size += advance.x * (1.0 + letterSpacing);
        }
    }

    currentIndex = indexAtDistance(currentIndex, -size * 0.5);

    int total = 0;
    int rotated = 0;

    int index = 0;
    double lastAngle = 0.0;

    double lineCenteringParameter = -fontResult->fontData->info.base / fontResult->fontData->info.lineHeight;

    for(auto &i : splittedTextInfo) {
        if(i.glyphIndex < 0) {
            currentIndex = indexAtDistance(currentIndex, spaceAdvance * fontSize * i.scale);
            lastAngle = 0;
            index = 0;
        } else {
            auto& d = fontResult->fontData->glyphs[i.glyphIndex];
            auto charSize = Vec2D(d.boundingBoxSize.x * fontSize * i.scale, d.boundingBoxSize.y * fontSize * i.scale);
            auto bearing = Vec2D(d.bearing.x * fontSize * i.scale, d.bearing.y * fontSize * i.scale);
            auto advance = Vec2D(d.advance.x * fontSize * i.scale, d.advance.y * fontSize * i.scale);

            // Punkt auf Linie
            auto p = pointAtIndex(currentIndex, true);

            // get before and after to calculate angle
            auto before = pointAtIndex(indexAtDistance(currentIndex, -charSize.x * 0.5), false);
            auto after = pointAtIndex(indexAtDistance(currentIndex, charSize.x * 0.5), false);

            double angle = atan2((before.y - after.y), -(before.x - after.x));
            angle *= (180.0 / M_PI);

            if(index > 1) {
                auto diff = fabs(lastAngle - angle);
                auto min = std::min(360.0 - diff, diff);

                if(min > maxCharacterAngle) {
                    centerPositions.clear();
                    break;
                }
            }

            lastAngle = angle;

            auto x = p.x + bearing.x;
            auto y = p.y - bearing.y;

            rotated += (angle > 90 || angle < -90) ? 1 : 0;
            total++;

            auto xw = x + charSize.x;
            auto yh = y + charSize.y;

            auto lastIndex = currentIndex;
            currentIndex = indexAtDistance(currentIndex, advance.x * (1.0 + letterSpacing));

            // if we are at the end, and we were at the end (lastIndex), then clear and skip
            if(currentIndex.first == renderLineCoordinates.size() - 1 && lastIndex.first == currentIndex.first && (lastIndex.second == currentIndex.second)) {
                centerPositions.clear();
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

            if (d.charCode != " ") {
                scales[2 * (countOffset + centerPositions.size()) + 0] = charSize.x;
                scales[2 * (countOffset + centerPositions.size()) + 1] = charSize.y;
                rotations[countOffset + centerPositions.size()] = -angle;

                centerPositions.push_back(Vec2D((quad.topLeft.x + quad.topRight.x) / 2,
                                                 (quad.bottomLeft.y + quad.topLeft.y ) / 2));
            }


            if (!box) {
                box = BoundingBox(Coord(referencePoint.systemIdentifier, quad.topLeft.x, quad.topRight.y, referencePoint.z));
            }

            box->addPoint(quad.topLeft.x, quad.topLeft.y, referencePoint.z);
            box->addPoint(quad.topRight.x, quad.topRight.y, referencePoint.z);
            box->addPoint(quad.bottomLeft.x, quad.bottomLeft.y, referencePoint.z);
            box->addPoint(quad.bottomRight.x, quad.bottomRight.y, referencePoint.z);

            index += 1;
        }
    }

    int countBefore = countOffset;
    if (!centerPositions.empty()) {
        assert(centerPositions.size() == characterCount);

        for (auto const &centerPosition: centerPositions) {
            positions[(2 * countOffset) + 0] = centerPosition.x;
            positions[(2 * countOffset) + 1] = centerPosition.y;

            countOffset += 1;
        }
    } else {
        for (int i = 0; i != characterCount; i++) {
            positions[(2 * countOffset) + 0] = 0;
            positions[(2 * countOffset) + 1] = 0;
            scales[2 * (countOffset) + 0] = 0;
            scales[2 * (countOffset) + 1] = 0;
            countOffset += 1;
        }
        box = std::nullopt;
    }

    assert(countOffset == countBefore + characterCount);

    if (box) {
        const float padding = description->style.getTextPadding(evalContext) * scaleFactor;
        const auto min = box->min;
        const auto max = box->max;
        boundingBox = RectCoord(Coord(min.systemIdentifier, min.x - padding, min.y - padding, min.z),
                                Coord(min.systemIdentifier, max.x + padding, max.y + padding, min.z));
    } else {
        boundingBox = RectCoord(referencePoint, referencePoint);
    }

    return (double)rotated / (double)total;
}

std::pair<int, double> Tiled2dMapVectorSymbolLabelObject::findReferencePointIndices() {
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


Coord Tiled2dMapVectorSymbolLabelObject::pointAtIndex(const std::pair<int, double> &index, bool useRender) {
    auto s = useRender ? renderLineCoordinates[index.first] : (*lineCoordinates)[index.first];
    auto e = useRender ?  renderLineCoordinates[index.first + 1 < renderLineCoordinates.size() ? (index.first + 1) : index.first] : (*lineCoordinates)[index.first + 1 < renderLineCoordinates.size() ? (index.first + 1) : index.first];
    return Coord(s.systemIdentifier, s.x + (e.x - s.x) * index.second, s.y + (e.y - s.y) * index.second, s.z + (e.z - s.z) * index.second);
}

std::pair<int, double> Tiled2dMapVectorSymbolLabelObject::indexAtDistance(const std::pair<int, double> &index, double distance) {
    auto current = pointAtIndex(index, true);
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
