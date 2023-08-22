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
#include "DateHelper.h"
#include "SymbolAnimationCoordinator.h"
#include "fast_atan2.h"

Tiled2dMapVectorSymbolLabelObject::Tiled2dMapVectorSymbolLabelObject(const std::shared_ptr<CoordinateConversionHelperInterface> &converter,
                                                                     const std::shared_ptr<FeatureContext> featureContext,
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
                                                                     const double radialOffset,
                                                                     const double lineHeight,
                                                                     const double letterSpacing,
                                                                     const int64_t maxCharacterWidth,
                                                                     const double maxCharacterAngle,
                                                                     const SymbolAlignment rotationAlignment,
                                                                     const TextSymbolPlacement &textSymbolPlacement,
                                                                     std::shared_ptr<SymbolAnimationCoordinator> animationCoordinator):
textSymbolPlacement(textSymbolPlacement),
rotationAlignment(rotationAlignment),
featureContext(featureContext),
description(description),
lineHeight(lineHeight),
letterSpacing(letterSpacing),
maxCharacterAngle(maxCharacterAngle),
textAnchor(textAnchor),
textJustify(textJustify),
offset(offset),
radialOffset(radialOffset),
fontResult(fontResult),
fullText(fullText),
lineCoordinates(lineCoordinates),
boundingBox(Vec2D(0, 0), Vec2D(0, 0), Vec2D(0, 0), Vec2D(0, 0)),
referencePoint(converter->convertToRenderSystem(coordinate)),
referenceSize(fontResult->fontData->info.size),
animationCoordinator(animationCoordinator)
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
                        if (!isSpace) {
                            characterCount += 1;
                        }
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

    numSymbols = (int)splittedTextInfo.size();

    if(lineCoordinates) {
        std::transform(lineCoordinates->begin(), lineCoordinates->end(), std::back_inserter(renderLineCoordinates),
                       [converter](const auto& l) {
            return converter->convertToRenderSystem(l);
        });
        renderLineCoordinatesCount = renderLineCoordinates.size();
    } else {
        renderLineCoordinatesCount = 0;
    }

    if (textJustify == TextJustify::AUTO) {
        switch (textAnchor) {
            case Anchor::TOP_LEFT:
            case Anchor::LEFT:
            case Anchor::BOTTOM_LEFT:
                this->textJustify = TextJustify::LEFT;
                break;
            case Anchor::TOP_RIGHT:
            case Anchor::RIGHT:
            case Anchor::BOTTOM_RIGHT:
                this->textJustify = TextJustify::RIGHT;
                break;
            case Anchor::CENTER:
            case Anchor::TOP:
            case Anchor::BOTTOM:
                this->textJustify = TextJustify::CENTER;
                break;
        }
    }
}

void Tiled2dMapVectorSymbolLabelObject::updateLayerDescription(const std::shared_ptr<SymbolVectorLayerDescription> layerDescription) {
    this->description = layerDescription;
    lastZoomEvaluation = -1;
}


int Tiled2dMapVectorSymbolLabelObject::getCharacterCount(){
    return characterCount;
}

void Tiled2dMapVectorSymbolLabelObject::setupProperties(std::vector<float> &textureCoordinates, std::vector<uint16_t> &styleIndices, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier) {
    const auto evalContext = EvaluationContext(zoomIdentifier, featureContext);

    evaluateStyleProperties(zoomIdentifier);

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


void Tiled2dMapVectorSymbolLabelObject::evaluateStyleProperties(const double zoomIdentifier) {
    auto roundedZoom = std::round(zoomIdentifier * 100.0) / 100.0;

    if (roundedZoom == lastZoomEvaluation) {
        return;
    }

    const auto evalContext = EvaluationContext(roundedZoom, featureContext);

    textSize = description->style.getTextSize(evalContext);
    textAlignment = description->style.getTextRotationAlignment(evalContext);
    textRotate = description->style.getTextRotate(evalContext);
    textPadding = description->style.getTextPadding(evalContext);

    textOpacity = description->style.getTextOpacity(evalContext);
    textColor = description->style.getTextColor(evalContext);
    haloColor = description->style.getTextHaloColor(evalContext);
    haloWidth = description->style.getTextHaloWidth(evalContext);

    lastZoomEvaluation = roundedZoom;
}


void Tiled2dMapVectorSymbolLabelObject::updateProperties(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor, const bool collides, const double rotation, const float alpha, const bool isCoordinateOwner, long long now) {
    const auto evalContext = EvaluationContext(zoomIdentifier, featureContext);

    evaluateStyleProperties(zoomIdentifier);

    float alphaFactor;
    if (!isCoordinateOwner) {
        alphaFactor = 0.0;
    } else if (collides || !(description->minZoom <= zoomIdentifier && description->maxZoom >= zoomIdentifier)) {
        alphaFactor = animationCoordinator->getTextAlpha(0.0, now);
    } else {
        float targetAlpha = textOpacity * alpha;
        alphaFactor = animationCoordinator->getTextAlpha(targetAlpha, now);
    }

    styles[(9 * styleOffset) + 0] = textColor.r; //R
    styles[(9 * styleOffset) + 1] = textColor.g; //G
    styles[(9 * styleOffset) + 2] = textColor.b; //B
    styles[(9 * styleOffset) + 3] = textColor.a * alphaFactor; //A
    styles[(9 * styleOffset) + 4] = haloColor.r; //R
    styles[(9 * styleOffset) + 5] = haloColor.g; //G
    styles[(9 * styleOffset) + 6] = haloColor.b; //B
    styles[(9 * styleOffset) + 7] = haloColor.a * alphaFactor; //A
    styles[(9 * styleOffset) + 8] = haloWidth;

    isOpaque = styles[(9 * styleOffset) + 3] != 0.0;

    styleOffset += 1;

    switch(textSymbolPlacement) {
        case TextSymbolPlacement::POINT: {
            updatePropertiesPoint(positions, scales, rotations, styles, countOffset, styleOffset, zoomIdentifier, scaleFactor, rotation);
            break;
        }
        case TextSymbolPlacement::LINE_CENTER:
        case TextSymbolPlacement::LINE: {
            if (rotationAlignment == SymbolAlignment::VIEWPORT) {
                updatePropertiesPoint(positions, scales, rotations, styles, countOffset, styleOffset, zoomIdentifier, scaleFactor,
                                      rotation);
            } else {
                auto rotatedFactor = updatePropertiesLine(positions, scales, rotations, styles, countOffset, styleOffset,
                                                          zoomIdentifier, scaleFactor, rotation);
                if(rotatedFactor > 0.5 && lineCoordinates) {
                    std::reverse((*lineCoordinates).begin(), (*lineCoordinates).end());
                    std::reverse(renderLineCoordinates.begin(), renderLineCoordinates.end());

                    wasReversed = !wasReversed;

                    countOffset -= characterCount;

                    updatePropertiesLine(positions, scales, rotations, styles, countOffset, styleOffset, zoomIdentifier, scaleFactor, rotation);
                }
            }

            break;
        }
    }
}

void Tiled2dMapVectorSymbolLabelObject::updatePropertiesPoint(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor, const double rotation) {
    
    const auto evalContext = EvaluationContext(zoomIdentifier, featureContext);
    
    const float fontSize = scaleFactor * textSize;
    
    auto pen = Vec2D(0.0, 0.0);

    Vec2D boxMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2D boxMax(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

    Vec2D centerPosBoxMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2D centerPosBoxMax(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

    centerPositions.clear();
    centerPositions.reserve(characterCount);
    
    static std::vector<size_t> lineEndIndices;
    lineEndIndices.clear();

    float angle;
    if (textAlignment == SymbolAlignment::MAP) {
        angle = textRotate;
    } else {
        angle = textRotate - rotation;
    }

    Vec2D anchorOffset(0.0, 0.0);

    static std::vector<double> baseLines;
    baseLines.clear();

    float yOffset = 0;

    for(const auto &i : splittedTextInfo) {
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

                baseLines.push_back(yh);

                boxMin.x = std::min(boxMin.x, std::min(quad.topLeft.x, std::min(quad.topRight.x, std::min(quad.bottomLeft.x, quad.bottomRight.x))));
                boxMin.y = std::min(boxMin.y, std::min(quad.topLeft.y, std::min(quad.topRight.y, std::min(quad.bottomLeft.y, quad.bottomRight.y))));

                boxMax.x = std::max(boxMax.x, std::max(quad.topLeft.x, std::max(quad.topRight.x, std::max(quad.bottomLeft.x, quad.bottomRight.x))));
                boxMax.y = std::max(boxMax.y, std::max(quad.topLeft.y, std::max(quad.topRight.y, std::max(quad.bottomLeft.y, quad.bottomRight.y))));

                if (pen.x == 0.0 && pen.y == 0.0) {
                    // only look at first character for offset
                    // this way the left top edge of the first character is exactly in the origin.
                    if (radialOffset == 0.0) {
                        anchorOffset.x = -boxMin.x;
                        yOffset = boxMin.y;
                    }
                }

                const size_t centerPositionSize = centerPositions.size();
                scales[2 * (countOffset + centerPositionSize) + 0] = size.x;
                scales[2 * (countOffset + centerPositionSize) + 1] = size.y;
                rotations[countOffset + centerPositionSize] = -angle;

                centerPosBoxMin.x = std::min(centerPosBoxMin.x, x + size.x / 2);
                centerPosBoxMax.x = std::max(centerPosBoxMax.x, x + size.x / 2);

                centerPosBoxMin.y = std::min(centerPosBoxMin.y, y + size.y / 2);
                centerPosBoxMax.y = std::max(centerPosBoxMax.y, y + size.y / 2);

                centerPositions.push_back(Vec2D(x + size.x / 2,
                                                y + size.y / 2));
            }

            pen.x += advance.x * (1.0 + letterSpacing);
        } else if(i.glyphIndex == -1) {
            if (!centerPositions.empty()) {
                lineEndIndices.push_back(centerPositions.size() - 1);
            }
            pen.x = 0.0;
            pen.y += fontSize * lineHeight;

            baseLines.clear();
        }
    }

    // Use the median base line of the last line for size calculations
    // This way labels with decent look better placed.
    std::sort(baseLines.begin(), baseLines.end());
    double medianLastBaseLine;
    if (baseLines.size() % 2 == 0) {
        medianLastBaseLine = (baseLines[baseLines.size() / 2 - 1] + baseLines[baseLines.size() / 2]) / 2;
    } else {
        medianLastBaseLine = baseLines[baseLines.size() / 2];
    }

    if (!centerPositions.empty()) {
        lineEndIndices.push_back(centerPositions.size() - 1);
    }

    const Vec2D size((boxMax.x - boxMin.x), (medianLastBaseLine - boxMin.y));
    const Vec2D centerSize((centerPosBoxMax.x - centerPosBoxMin.x), (centerPosBoxMax.y - centerPosBoxMin.y));

    switch (textJustify) {
        case TextJustify::AUTO:
            // Should have been finalized to one of the below. Otherwise use default LEFT.
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

    // TODO: currently only shifting to top right
    Vec2D textOffset(0.0, 0.0);
    if (radialOffset != 0) {
        // Text offset is ignored when radial offset is set
        textOffset.x = radialOffset;
        textOffset.y = -radialOffset;
    } else {
        textOffset.x = offset.x;
        textOffset.y = offset.y;
    }

    switch (textAnchor) {
        case Anchor::CENTER:
        case Anchor::TOP:
        case Anchor::BOTTOM:
            anchorOffset.x -= size.x / 2.0 - textOffset.x * fontSize;
            break;
        case Anchor::LEFT:
        case Anchor::TOP_LEFT:
        case Anchor::BOTTOM_LEFT:
            anchorOffset.x += textOffset.x * fontSize;
            break;
        case Anchor::RIGHT:
        case Anchor::TOP_RIGHT:
        case Anchor::BOTTOM_RIGHT:
            anchorOffset.x -= size.x - textOffset.x * fontSize;
            break;
        default:
            break;
    }

    switch (textAnchor) {
        case Anchor::CENTER:
        case Anchor::LEFT:
        case Anchor::RIGHT:
            anchorOffset.y -= yOffset + size.y / 2.0 + textOffset.y * fontSize;
            break;
        case Anchor::TOP:
        case Anchor::TOP_LEFT:
        case Anchor::TOP_RIGHT:
            anchorOffset.y -= textOffset.y * fontSize - yOffset;
            break;
        case Anchor::BOTTOM:
        case Anchor::BOTTOM_LEFT:
        case Anchor::BOTTOM_RIGHT:
            if (radialOffset != 0.0) {
                anchorOffset.y -= (lineHeight - fontResult->fontData->info.lineHeight + 1.0 - textOffset.y) * fontSize;
            } else {
                anchorOffset.y = -size.y;
                anchorOffset.y -= ((fontResult->fontData->info.lineHeight - fontResult->fontData->info.base) * lineHeight - textOffset.y) * fontSize + yOffset;
            }
            break;
        default:
            break;
    }

    Coord boundingBoxMin(referencePoint.systemIdentifier, std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), referencePoint.z);
    Coord boundingBoxMax(referencePoint.systemIdentifier, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), referencePoint.z);

    Vec2D anchorOffsetRot = Vec2DHelper::rotate(anchorOffset, Vec2D(0, 0), angle);

    const auto dx = referencePoint.x + anchorOffset.x;
    const auto dy = referencePoint.y + anchorOffset.y;
    const auto dxRot = referencePoint.x + anchorOffsetRot.x;
    const auto dyRot = referencePoint.y + anchorOffsetRot.y;

    assert(centerPositions.size() == characterCount);

    float maxSymbolRadius = 0.0;
    for(auto const centerPosition: centerPositions) {
        auto rotated = Vec2DHelper::rotate(centerPosition, Vec2D(0, 0), angle);

        positions[2 * countOffset + 0] = rotated.x + dxRot;
        positions[2 * countOffset + 1] = rotated.y + dyRot;

        const float scaleXH = scales[2 * countOffset + 0] / 2.0;
        const float scaleYH = scales[2 * countOffset + 1] / 2.0;
        maxSymbolRadius = std::max(maxSymbolRadius, std::max(scaleXH, scaleYH));

        const double x1 = dx + centerPosition.x - scaleXH;
        const double x2 = dx + centerPosition.x + scaleXH;

        const double y1 = dy + centerPosition.y - scaleYH;
        const double y2 = dy + centerPosition.y + scaleYH;

        boundingBoxMin.x = std::min(boundingBoxMin.x, std::min(x1, x2));
        boundingBoxMin.y = std::min(boundingBoxMin.y, std::min(y1, y2));

        boundingBoxMax.x = std::max(boundingBoxMax.x, std::max(x1, x2));
        boundingBoxMax.y = std::max(boundingBoxMax.y, std::max(y1, y2));

        countOffset += 1;
    }

    auto rectBoundingBox = (!centerPositions.empty()) ? RectCoord(boundingBoxMin, boundingBoxMax) :  RectCoord(referencePoint, referencePoint);

    const float scaledTextPadding = textPadding * scaleFactor;

    rectBoundingBox.topLeft.x -= scaledTextPadding;
    rectBoundingBox.topLeft.y -= scaledTextPadding;

    rectBoundingBox.bottomRight.x += scaledTextPadding;
    rectBoundingBox.bottomRight.y += scaledTextPadding;

    dimensions.x = rectBoundingBox.bottomRight.x - rectBoundingBox.topLeft.x;
    dimensions.y = rectBoundingBox.bottomRight.y - rectBoundingBox.topLeft.y;

    boundingBox = Quad2dD(Vec2DHelper::rotate(Vec2D(rectBoundingBox.topLeft.x, rectBoundingBox.topLeft.y), Vec2D(dx, dy), angle),
                          Vec2DHelper::rotate(Vec2D(rectBoundingBox.bottomRight.x, rectBoundingBox.topLeft.y), Vec2D(dx, dy), angle),
                          Vec2DHelper::rotate(Vec2D(rectBoundingBox.bottomRight.x, rectBoundingBox.bottomRight.y), Vec2D(dx, dy), angle),
                          Vec2DHelper::rotate(Vec2D(rectBoundingBox.topLeft.x, rectBoundingBox.bottomRight.y), Vec2D(dx, dy), angle));
    if (rotationAlignment != SymbolAlignment::MAP) {
        boundingBoxViewportAligned = CollisionRectD(referencePoint.x, referencePoint.y, boundingBoxMin.x - scaledTextPadding, boundingBoxMin.y - scaledTextPadding, dimensions.x, dimensions.y);
        boundingBoxCircles = std::nullopt;
    } else {
        std::vector<CircleD> circles;
        Vec2D origin = Vec2D(dx, dy);
        Vec2D lastCirclePosition = Vec2D(0, 0);
        size_t count = centerPositions.size();
        for (int i = 0; i < count; i++) {
            Vec2D newPos = Vec2D(dx + centerPositions.at(i).x, dy + centerPositions.at(i).y);
            newPos = Vec2DHelper::rotate(newPos, origin, angle);
            if (i != count - 1 && std::sqrt((newPos.x - lastCirclePosition.x) * (newPos.x - lastCirclePosition.x) +
                                            (newPos.y - lastCirclePosition.y) * (newPos.y - lastCirclePosition.y)) <=
                                          (2.0 * maxSymbolRadius) * collisionDistanceBias) {
                continue;
            }
            circles.emplace_back(newPos.x, newPos.y, maxSymbolRadius + scaledTextPadding);
            lastCirclePosition = newPos;
        }
        boundingBoxCircles = circles;
        boundingBoxViewportAligned = std::nullopt;
    }
}

double Tiled2dMapVectorSymbolLabelObject::updatePropertiesLine(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor, const double rotation) {
    if(lineCoordinates == std::nullopt) {
        countOffset += characterCount;
        return 0;
    }

    auto evalContext = EvaluationContext(zoomIdentifier, featureContext);

    const float fontSize = scaleFactor * textSize;

    Vec2D boxMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2D boxMax(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

    centerPositions.clear();
    centerPositions.reserve(characterCount);

    auto currentIndex = findReferencePointIndices();

    double size = 0;

    for(const auto &i : splittedTextInfo) {
        if(i.glyphIndex < 0) {
            size += spaceAdvance * fontSize * i.scale;
        } else {
            auto &d = fontResult->fontData->glyphs[i.glyphIndex];
            auto advance = Vec2D(d.advance.x * fontSize * i.scale, d.advance.y * fontSize * i.scale);
            size += advance.x * (1.0 + letterSpacing);
        }
    }

    currentIndex = indexAtDistance(currentIndex, -size * 0.5);
    auto yOffset = Vec2D(0.0, offset.y * fontSize);

    if (wasReversed) {
        yOffset.y *= -1;
    }

    double averageAngleS = 0.0;
    double averageAngleC = 0.0;

    int index = 0;
    double lastAngle = 0.0;
    double lastAngleDiffs = 0.0;

    double lineCenteringParameter = -fontResult->fontData->info.base / fontResult->fontData->info.lineHeight;

    double maxSymbolRadius = 0.0;

    for(auto &i : splittedTextInfo) {
        if(i.glyphIndex < 0) {
            currentIndex = indexAtDistance(currentIndex, spaceAdvance * fontSize * i.scale);
            index = 0;
        } else {
            auto& d = fontResult->fontData->glyphs[i.glyphIndex];
            auto charSize = Vec2D(d.boundingBoxSize.x * fontSize * i.scale, d.boundingBoxSize.y * fontSize * i.scale);
            auto bearing = Vec2D(d.bearing.x * fontSize * i.scale, d.bearing.y * fontSize * i.scale);
            auto advance = Vec2D(d.advance.x * fontSize * i.scale, d.advance.y * fontSize * i.scale);

            auto halfSpace = std::max(charSize.x, advance.x) * 0.5;

            // Punkt auf Linie
            const auto &p = pointAtIndex(currentIndex, true);

            // get before and after to calculate angle
            const auto &before = pointAtIndex(indexAtDistance(currentIndex, -halfSpace), false);
            const auto &after = pointAtIndex(indexAtDistance(currentIndex, halfSpace), false);

            double angleRad = atan2_approximation((before.y - after.y), -(before.x - after.x));
            double angleDeg = angleRad * (180.0 / M_PI);

            if(index > 0) {
                auto diff = fabs(lastAngle - angleDeg);
                auto min = std::min(360.0 - diff, diff);
                if(min + lastAngleDiffs > maxCharacterAngle) {
                    centerPositions.clear();
                    break;
                }
                lastAngleDiffs = 0.5 * lastAngleDiffs + min;
            }

            lastAngle = angleDeg;

            auto x = p.x + bearing.x + yOffset.x;
            auto y = p.y - bearing.y + yOffset.y;

            averageAngleS += sin(angleRad) / numSymbols;
            averageAngleC += cos(angleRad) / numSymbols;

            auto xw = x + charSize.x;
            auto yh = y + charSize.y;

            auto lastIndex = currentIndex;
            currentIndex = indexAtDistance(currentIndex, advance.x * (1.0 + letterSpacing));

            // if we are at the end, and we were at the end (lastIndex), then clear and skip
            if(currentIndex.first == renderLineCoordinatesCount - 1 && lastIndex.first == currentIndex.first && (lastIndex.second == currentIndex.second)) {
                centerPositions.clear();
                break;
            }

            auto tl = Vec2D(x, yh);
            auto tr = Vec2D(xw, yh);
            auto bl = Vec2D(x, y);
            auto br = Vec2D(xw, y);

            Quad2dD quad = Quad2dD(tl, tr, br, bl);
            quad = TextHelper::rotateQuad2d(quad, p, angleDeg);

            auto dy = Vec2DHelper::normalize(Vec2D(quad.bottomLeft.x - quad.topLeft.x, quad.bottomLeft.y - quad.topLeft.y));
            dy.x *= lineCenteringParameter * fontSize;
            dy.y *= lineCenteringParameter * fontSize;

            quad.topLeft = quad.topLeft - dy;
            quad.bottomLeft = quad.bottomLeft - dy;
            quad.topRight = quad.topRight - dy;
            quad.bottomRight = quad.bottomRight - dy;

            if (d.charCode != " ") {
                const size_t centerPositionSize = centerPositions.size();
                scales[2 * (countOffset + centerPositionSize) + 0] = charSize.x;
                scales[2 * (countOffset + centerPositionSize) + 1] = charSize.y;
                maxSymbolRadius = std::max(maxSymbolRadius, std::max(charSize.x * 0.5, charSize.y * 0.5));
                rotations[countOffset + centerPositionSize] = -angleDeg;

                centerPositions.push_back(Vec2DHelper::midpoint(Vec2DHelper::midpoint(quad.bottomLeft, quad.bottomRight),
                                                                Vec2DHelper::midpoint(quad.topLeft, quad.topRight)));
            }


            boxMin.x = std::min(boxMin.x, std::min(quad.topLeft.x, std::min(quad.topRight.x, std::min(quad.bottomLeft.x, quad.bottomRight.x))));
            boxMin.y = std::min(boxMin.y, std::min(quad.topLeft.y, std::min(quad.topRight.y, std::min(quad.bottomLeft.y, quad.bottomRight.y))));

            boxMax.x = std::max(boxMax.x, std::max(quad.topLeft.x, std::max(quad.topRight.x, std::max(quad.bottomLeft.x, quad.bottomRight.x))));
            boxMax.y = std::max(boxMax.y, std::max(quad.topLeft.y, std::max(quad.topRight.y, std::max(quad.bottomLeft.y, quad.bottomRight.y))));

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
        boxMin.x = std::numeric_limits<float>::max();
    }

    assert(countOffset == countBefore + characterCount);

    if (boxMin.x != std::numeric_limits<float>::max()) {
        const float padding = textPadding * scaleFactor;
        boundingBox.topLeft = Vec2D(boxMin.x - padding, boxMin.y - padding);
        boundingBox.topRight = Vec2D(boxMax.x + padding, boxMin.y - padding);
        boundingBox.bottomRight = Vec2D(boxMax.x + padding, boxMax.y + padding);
        boundingBox.bottomLeft = Vec2D(boxMin.x + padding, boxMax.y + padding);

        std::vector<CircleD> circles;
        Vec2D lastCirclePosition = Vec2D(0, 0);
        size_t count = centerPositions.size();
        for (int i = 0; i < count; i++) {
            double newX = centerPositions.at(i).x;
            double newY = centerPositions.at(i).y;
            if (i != count - 1 && std::sqrt((newX - lastCirclePosition.x) * (newX - lastCirclePosition.x) +
                                            (newY - lastCirclePosition.y) * (newY - lastCirclePosition.y))
                                  <= (maxSymbolRadius * 2.0) * collisionDistanceBias) {
                continue;
            }
            circles.emplace_back(newX, newY, maxSymbolRadius + padding);
            lastCirclePosition.x = newX;
            lastCirclePosition.y = newY;
        }
        boundingBoxCircles = circles;
    } else {
        boundingBox.topLeft = Vec2D(0.0, 0.0);
        boundingBox.topRight = Vec2D(0.0, 0.0);
        boundingBox.bottomRight = Vec2D(0.0, 0.0);
        boundingBox.bottomLeft = Vec2D(0.0, 0.0);
    }
    boundingBoxViewportAligned = std::nullopt;

    double diff = 0;
    if (!centerPositions.empty()) {
        double recompRotation = fmod(-rotation + 360.0, 360.0);
        double averageAngle = fmod(atan2_approximation(averageAngleS, averageAngleC) * 180.0 / M_PI + 360.0, 360.0);
        diff = std::min(std::min(std::abs(averageAngle - recompRotation),
                                 std::abs(averageAngle + 360.0 - recompRotation)),
                        std::abs(averageAngle - (recompRotation + 360.0)));
    }
    return diff > 95.0 ? 1.0 : 0.0; // flip with margin to prevent rapid flips
}

std::pair<int, double> Tiled2dMapVectorSymbolLabelObject::findReferencePointIndices() {
    auto point = referencePoint;
    auto distance = std::numeric_limits<double>::max();

    double tMin = 0.0f;
    int iMin = 0;

    for(int i=1; i < renderLineCoordinatesCount; ++i) {
        auto start = renderLineCoordinates.at(i-1);
        auto end = renderLineCoordinates.at(i);

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
