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
boundingBox(Coord("", 0.0, 0.0, 0.0), Coord("", 0.0, 0.0, 0.0))
{

    referencePoint = converter->convertToRenderSystem(coordinate);
    referenceSize = fontResult->fontData->info.size;

    float spaceAdvance = 0.0f;
    for (const auto &d : fontResult->fontData->glyphs) {
        if(d.charCode == " ") {
            spaceAdvance = d.advance.x;
            break;
        }
    }

    std::vector<BreakResult> breaks = {};
    if(textSymbolPlacement == TextSymbolPlacement::POINT) {
        std::vector<std::string> letters;

        for (const auto &entry: text) {
            for (const auto &c : TextHelper::splitWstring(entry.text)) {
                letters.push_back(c);
            }
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
        for(auto& l : *lineCoordinates) {
            renderLineCoordinates.push_back(converter->convertToRenderSystem(l));
        }
    }
}

int Tiled2dMapVectorSymbolLabelObject::getCharacterCount(){
    return characterCount;
}

void Tiled2dMapVectorSymbolLabelObject::setupProperties(std::vector<float> &textureCoordinates, std::vector<uint16_t> &styleIndices, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier) {
    for(auto &i : splittedTextInfo) {
        auto &d = fontResult->fontData->glyphs[i.glyphIndex];
        if(i.glyphIndex >= 0 && d.charCode != " ") {
            auto& d = fontResult->fontData->glyphs[i.glyphIndex];
            textureCoordinates[4 * countOffset + 0] = d.uv.topLeft.x;
            textureCoordinates[4 * countOffset + 1] = d.uv.bottomRight.y;
            textureCoordinates[4 * countOffset + 2] = d.uv.bottomRight.x - d.uv.topLeft.x;
            textureCoordinates[4 * countOffset + 3] = d.uv.topLeft.y - d.uv.bottomLeft.y;

            styleIndices[countOffset] = styleOffset;
            countOffset += 1;
        }
    }
    styleOffset += 1;
}

void Tiled2dMapVectorSymbolLabelObject::updateProperties(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor) {
    switch(textSymbolPlacement) {
        case TextSymbolPlacement::LINE_CENTER:
        case TextSymbolPlacement::POINT: {
            updatePropertiesPoint(positions, scales, rotations, styles, countOffset, styleOffset, zoomIdentifier, scaleFactor);
            break;
        }


        case TextSymbolPlacement::LINE: {

            if (rotationAlignment == SymbolAlignment::VIEWPORT) {
                updatePropertiesPoint(positions, scales, rotations, styles, countOffset, styleOffset, zoomIdentifier, scaleFactor);
            } else {
                //                auto rotatedFactor = layoutLine(scale, updateObject);
                //
                //                if(rotatedFactor > 0.5 && lineCoordinates && !rotated) {
                //                    std::reverse((*lineCoordinates).begin(), (*lineCoordinates).end());
                //                    std::reverse(renderLineCoordinates.begin(), renderLineCoordinates.end());
                //
                //                    updatePropertiesLine(positions, scales, rotations, alphas, countOffset, zoomIdentifier, scaleFactor)
                //                    rotated = true;
                //                }
            }

            break;
        }
    }

    styleOffset += 1;

}

void Tiled2dMapVectorSymbolLabelObject::updatePropertiesPoint(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor) {

    auto evalContext = EvaluationContext(zoomIdentifier, featureContext);

    float fontSize = scaleFactor * description->style.getTextSize(evalContext);


    auto pen = Vec2D(0.0, 0.0);

    std::optional<BoundingBox> box = std::nullopt;

    auto numGlyphs = splittedTextInfo.size();


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

    centerPositions.clear();
    centerPositions.reserve(numGlyphs);

    std::vector<size_t> lineEndIndices;

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
                    box = BoundingBox(Coord(referencePoint.systemIdentifier, quad.topLeft.x, quad.topLeft.y, referencePoint.z));
                }

                box->addPoint(quad.topLeft.x, quad.topLeft.y, referencePoint.z);
                box->addPoint(quad.topRight.x, quad.topRight.y, referencePoint.z);
                box->addPoint(quad.bottomLeft.x, quad.bottomLeft.y, referencePoint.z);
                box->addPoint(quad.bottomRight.x, quad.bottomRight.y, referencePoint.z);

                scales[2 * (countOffset + centerPositions.size())  + 0] = size.x;
                scales[2 * (countOffset + centerPositions.size()) + 1] = size.y;
                rotations[countOffset + (centerPositions.size() / 2)] = 0.0;

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

    if (!centerPositions.empty()) {
        Vec2D min(box->min.x, box->min.y);
        Vec2D max(box->max.x, box->max.y);
        Vec2D size((max.x - min.x), (max.y - min.y));

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
                    double delta = (size.x - lineWidth) / factor;

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

        Vec2D offset(0.0, 0.0);

        switch (textAnchor) {
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

        assert(centerPositions.size() == characterCount);

        for(auto const centerPosition: centerPositions) {
            positions[2 * countOffset + 0] = centerPosition.x + dx;
            positions[2 * countOffset + 1] = centerPosition.y + dy;

            box->addPoint(centerPosition.x, centerPosition.y, referencePoint.z);

            countOffset += 1;
        }
    }
    boundingBox = box ? RectCoord(box->min, box->max) : RectCoord(referencePoint, referencePoint);
}

void Tiled2dMapVectorSymbolLabelObject::updatePropertiesLine(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor) {

}
