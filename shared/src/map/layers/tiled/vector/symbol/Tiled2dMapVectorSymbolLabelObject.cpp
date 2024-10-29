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
#include "Tiled2dMapVectorStyleParser.h"
#include "fast_atan2.h"
#include "MapCamera3d.h"

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
                                                                     std::shared_ptr<SymbolAnimationCoordinator> animationCoordinator,
                                                                     const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager,
                                                                     double dpFactor,
                                                                     bool is3d,
                                                                     const Vec3D &tileOrigin)
        : textSymbolPlacement(textSymbolPlacement),
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
          referencePoint(converter->convertToRenderSystem(coordinate)),
          referenceSize(fontResult->fontData->info.size),
          animationCoordinator(animationCoordinator),
          stateManager(featureStateManager),
          dpFactor(dpFactor),
          is3d(is3d),
          tileOrigin(tileOrigin),
          positionSize(is3d ? 3 : 2) {

    for(auto i=0; i<fontResult->fontData->glyphs.size(); ++i) {
        auto& letter = fontResult->fontData->glyphs[i];
        if(letter.charCode == " ") {
            spaceAdvance = letter.advance.x;
            spaceIndex = i;
            break;
        }
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

    lineEndIndices.push_back(0); // one is always needed

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
                    lineEndIndices.push_back(0.0);
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

        if(is3d) {
            for(auto& c : renderLineCoordinates) { 
                double x = c.z * sin(c.y) * cos(c.x);
                double y = c.z * cos(c.y);
                double z = -c.z * sin(c.y) * sin(c.x);
                cartesianRenderLineCoordinates.emplace_back(x, y, z);
            }

            auto &c = referencePoint;
            cartesianReferencePoint.x = c.z * sin(c.y) * cos(c.x);
            cartesianReferencePoint.y = c.z * cos(c.y);
            cartesianReferencePoint.z = -c.z * sin(c.y) * sin(c.x);
        }
        
        screenLineCoordinates = renderLineCoordinates;
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

    const auto &usedKeys = description->getUsedKeys();
    isStyleStateDependant = usedKeys.isStateDependant();
}

void Tiled2dMapVectorSymbolLabelObject::updateLayerDescription(const std::shared_ptr<SymbolVectorLayerDescription> layerDescription) {
    this->description = layerDescription;
    const auto &usedKeys = description->getUsedKeys();
    isStyleStateDependant = usedKeys.isStateDependant();
    lastZoomEvaluation = -1;
}


int Tiled2dMapVectorSymbolLabelObject::getCharacterCount(){
    return characterCount;
}

void Tiled2dMapVectorSymbolLabelObject::setupProperties(std::vector<float> &textureCoordinates, std::vector<uint16_t> &styleIndices, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier) {
    const auto evalContext = EvaluationContext(zoomIdentifier, dpFactor, featureContext, stateManager);

    evaluateStyleProperties(zoomIdentifier);

    for(auto &i : splittedTextInfo) {
        if (i.glyphIndex < 0) continue;
        auto& d = fontResult->fontData->glyphs[i.glyphIndex];
        if(d.charCode != " ") {
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

    if (roundedZoom == lastZoomEvaluation && !isStyleStateDependant) {
        return;
    }

    const auto evalContext = EvaluationContext(roundedZoom, dpFactor, featureContext, stateManager);

    textOpacity = description->style.getTextOpacity(evalContext);
    if (textOpacity == 0.0) {
        lastZoomEvaluation = roundedZoom;
        return;
    }

    textSize = description->style.getTextSize(evalContext);
    textAlignment = description->style.getTextRotationAlignment(evalContext);
    textRotate = description->style.getTextRotate(evalContext);
    textPadding = description->style.getTextPadding(evalContext);

    textColor = description->style.getTextColor(evalContext);
    haloColor = description->style.getTextHaloColor(evalContext);
    haloWidth = description->style.getTextHaloWidth(evalContext, textSize);
    haloBlur = description->style.getTextHaloBlur(evalContext);

    lastZoomEvaluation = roundedZoom;
}


void Tiled2dMapVectorSymbolLabelObject::updateProperties(std::vector<float> &positions, std::vector<float> &referencePositions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor, const bool collides, const double rotation, const float alpha, const bool isCoordinateOwner, long long now, const Vec2I &viewportSize, const std::shared_ptr<MapCameraInterface>& camera) {
    const auto evalContext = EvaluationContext(zoomIdentifier, dpFactor, featureContext, stateManager);

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

    styles[(10 * styleOffset) + 0] = textColor.r; //R
    styles[(10 * styleOffset) + 1] = textColor.g; //G
    styles[(10 * styleOffset) + 2] = textColor.b; //B
    styles[(10 * styleOffset) + 3] = textColor.a * alphaFactor; //A
    styles[(10 * styleOffset) + 4] = haloColor.r; //R
    styles[(10 * styleOffset) + 5] = haloColor.g; //G
    styles[(10 * styleOffset) + 6] = haloColor.b; //B
    styles[(10 * styleOffset) + 7] = haloColor.a * alphaFactor; //A
    styles[(10 * styleOffset) + 8] = haloWidth;
    styles[(10 * styleOffset) + 9] = haloBlur;

    isOpaque = styles[(10 * styleOffset) + 3] != 0.0;

    styleOffset += 1;

    switch(textSymbolPlacement) {
        case TextSymbolPlacement::POINT: {
            updatePropertiesPoint(positions, referencePositions, scales, rotations, styles, countOffset, styleOffset, zoomIdentifier, scaleFactor, rotation, viewportSize);
            break;
        }
        case TextSymbolPlacement::LINE_CENTER:
        case TextSymbolPlacement::LINE: {
            if (rotationAlignment == SymbolAlignment::VIEWPORT) {
                updatePropertiesPoint(positions, referencePositions, scales, rotations, styles, countOffset, styleOffset, zoomIdentifier, scaleFactor, rotation, viewportSize);
            } else {
                setupCamera(camera, viewportSize);
                auto rotatedFactor = updatePropertiesLine(positions, referencePositions, scales, rotations, styles, countOffset, styleOffset, zoomIdentifier, scaleFactor, rotation, viewportSize, camera);

                if(rotatedFactor > 0.5 && lineCoordinates) {
                    std::reverse((*lineCoordinates).begin(), (*lineCoordinates).end());
                    std::reverse(renderLineCoordinates.begin(), renderLineCoordinates.end());
                    std::reverse(screenLineCoordinates.begin(), screenLineCoordinates.end());
                    std::reverse(cartesianRenderLineCoordinates.begin(), cartesianRenderLineCoordinates.end());

                    wasReversed = !wasReversed;

                    countOffset -= characterCount;

                    setupCamera(camera, viewportSize);
                    updatePropertiesLine(positions, referencePositions, scales, rotations, styles, countOffset, styleOffset, zoomIdentifier, scaleFactor, rotation, viewportSize, camera);
                }
            }

            break;
        }
    }
}

void Tiled2dMapVectorSymbolLabelObject::updatePropertiesPoint(std::vector<float> &positions, std::vector<float> &referencePositions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor, const double rotation, const Vec2I &viewportSize) {

    const auto evalContext = EvaluationContext(zoomIdentifier, dpFactor, featureContext, stateManager);
    const float fontSize = is3d ? textSize : (scaleFactor * textSize);

    Vec2D boxMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2D boxMax(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

    Vec2D centerPosBoxMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2D centerPosBoxMax(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

    auto zero = Vec2D(0.0, 0.0);
    while(centerPositions.size() < characterCount) {
        centerPositions.push_back(zero);
    }

    auto pen = zero;

    float angle;
    if (textAlignment == SymbolAlignment::MAP) {
        angle = textRotate;
    } else {
        angle = textRotate - rotation;
    }

    Vec2D anchorOffset(0.0, 0.0);

    static std::vector<double> baseLines;
    while(baseLines.size() < characterCount) {
        baseLines.push_back(0.0);
    }

    float yOffset = 0;


    pen.y -= fontSize * lineHeight * 0.25;

    int numberOfCharacters = 0;
    int baseLineStartIndex = 0;
    int lineEndIndicesIndex = 0;
    const auto &glyphs = fontResult->fontData->glyphs;

    for(const auto &i : splittedTextInfo) {
        if(i.glyphIndex >= 0) {
            assert(i.glyphIndex < fontResult->fontData->glyphs.size());
            const auto &d = glyphs[i.glyphIndex];

            auto scale = fontSize * i.scale;
            auto size = Vec2D(d.boundingBoxSize.x * scale, d.boundingBoxSize.y * scale);
            auto bearing = Vec2D(d.bearing.x * scale, d.bearing.y * scale);
            auto advance = Vec2D(d.advance.x * scale, d.advance.y * scale);

            if(i.glyphIndex != spaceIndex) {
                auto x = pen.x + bearing.x;
                auto y = pen.y - bearing.y;
                auto xw = x + size.x;
                auto yh = y + size.y;

                baseLines[numberOfCharacters] = yh;

                boxMin.x = boxMin.x < x ? boxMin.x : x;
                boxMax.x = boxMax.x > xw ? boxMax.x : xw;
                boxMin.y = boxMin.y < y ? boxMin.y : y;
                boxMax.y = boxMax.y > yh ? boxMax.y : yh;

                if (pen.x == 0.0 && pen.y == 0.0) {
                    // only look at first character for offset
                    // this way the left top edge of the first character is exactly in the origin.
                    if (radialOffset == 0.0) {
                        anchorOffset.x = -boxMin.x;
                        yOffset = boxMin.y;
                    }
                }

                if (is3d) {
                    scales[2 * (countOffset + numberOfCharacters) + 0] = size.x / viewportSize.x * 2.0;
                    scales[2 * (countOffset + numberOfCharacters) + 1] = size.y / viewportSize.x * 2.0;
                } else {
                    scales[2 * (countOffset + numberOfCharacters) + 0] = size.x;
                    scales[2 * (countOffset + numberOfCharacters) + 1] = size.y;
                }

                rotations[countOffset + numberOfCharacters] = -angle;
                centerPositions[numberOfCharacters].x = (x + xw) * 0.5;
                centerPositions[numberOfCharacters].y = (y + yh) * 0.5;
                ++numberOfCharacters;
            }

            pen.x += advance.x * (1.0 + letterSpacing);

        } else if(i.glyphIndex == -1) {
            if (numberOfCharacters > 0) {
                lineEndIndices[lineEndIndicesIndex] = numberOfCharacters - 1;
                lineEndIndicesIndex++;
                assert((countOffset + numberOfCharacters-1)*2 < scales.size());
            }
            pen.x = 0.0;
            pen.y += fontSize * lineHeight;

            baseLineStartIndex = numberOfCharacters;
        }
        else {
            assert(false);
        }
    }

    // Use the median base line of the last line for size calculations
    // This way labels look better placed.
    std::sort(baseLines.begin() + baseLineStartIndex, baseLines.begin() + numberOfCharacters);

    int l = numberOfCharacters - baseLineStartIndex;
    int medianIndex = baseLineStartIndex + l/2;
    double medianLastBaseLine;
    if (l % 2 == 0) {
        medianLastBaseLine = (baseLines[medianIndex - 1] + baseLines[std::min(medianIndex + 1, (int)baseLines.size() - 1)]) / 2;
    } else {
        medianLastBaseLine = baseLines[medianIndex];
    }

    if (numberOfCharacters > 0) {
        lineEndIndices[lineEndIndicesIndex] = numberOfCharacters - 1;
        lineEndIndicesIndex++;
        assert((countOffset + numberOfCharacters-1)*2 < scales.size());
    }

    const Vec2D size((boxMax.x - boxMin.x), (medianLastBaseLine - boxMin.y));

    switch (textJustify) {
        case TextJustify::AUTO:
            // Should have been finalized to one of the below. Otherwise use default LEFT.
        case TextJustify::LEFT:
            //Nothing to do here
            break;
        case TextJustify::RIGHT:
        case TextJustify::CENTER: {
            size_t lineStart = 0;
            size_t centerPositionsSize = centerPositions.size();
            for (size_t i = 0; i < lineEndIndicesIndex; ++i) {
                auto lineEndIndex = lineEndIndices[i];
                if (lineStart >= centerPositionsSize || lineEndIndex >= centerPositionsSize) {
                    continue;
                }
                auto factor = textJustify == TextJustify::CENTER ? 2.0 : 1.0;
                const auto idx = 2 * (countOffset + lineEndIndex);
                assert(idx >= 0 && idx < scales.size());
                assert(lineStart < centerPositions.size());
                assert(lineEndIndex < centerPositions.size());
                double startFirst = centerPositions[lineStart].x - scales[idx] * 0.5;
                double endLast = centerPositions[lineEndIndex].x + scales[idx] * 0.5;
                double lineWidth = endLast - startFirst;
                double delta = (size.x - lineWidth) / factor;

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
            anchorOffset.y = -size.y * 0.5;
            anchorOffset.y += textOffset.y * fontSize + yOffset;
            break;
        case Anchor::TOP:
        case Anchor::TOP_LEFT:
        case Anchor::TOP_RIGHT:
            anchorOffset.y += textOffset.y * fontSize + yOffset;
            break;
        case Anchor::BOTTOM:
        case Anchor::BOTTOM_LEFT:
        case Anchor::BOTTOM_RIGHT:
            if (radialOffset != 0.0) {
                anchorOffset.y -= (lineHeight - fontResult->fontData->info.lineHeight + 1.0 - textOffset.y) * fontSize;
            } else {
                anchorOffset.y = -1 * size.y;
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

    const double sinAngle = sin(angle * M_PI / 180.0);
    const double cosAngle = cos(angle * M_PI / 180.0);

    assert(numberOfCharacters == characterCount);

    float maxSymbolRadius = 0.0;
    for(int i=0; i<numberOfCharacters; ++i) {
        auto& cp = centerPositions[i];

        const double rX = cp.x * cosAngle - cp.y * sinAngle;
        const double rY = cp.x * sinAngle + cp.y * cosAngle;

        const float scaleXH = (scales[2 * countOffset + 0] / 2.0) * (is3d ? 0.5 * viewportSize.x : 1.0);
        const float scaleYH = (scales[2 * countOffset + 1] / 2.0) * (is3d ? 0.5 * viewportSize.y : 1.0);

        if (is3d) {
            positions[2 * countOffset + 0] = (rX + anchorOffsetRot.x) / viewportSize.x * 2;
            positions[2 * countOffset + 1] = -(rY + anchorOffsetRot.y) / viewportSize.y * 2;

            writePosition(referencePoint.x, referencePoint.y, countOffset, referencePositions);
        } else {
            writePosition(rX + dxRot, rY + dyRot, countOffset, positions);
        }

        auto maxScale = ((scales[2 * countOffset + 0] / 2.0) > (scales[2 * countOffset + 1] / 2.0)) ? (scales[2 * countOffset + 0] / 2.0) : (scales[2 * countOffset + 1] / 2.0);
        maxSymbolRadius = (maxSymbolRadius > maxScale) ? maxSymbolRadius : maxScale;

        const double x1 = (is3d ? anchorOffsetRot.x : dxRot) + rX - scaleXH;
        const double x2 = (is3d ? anchorOffsetRot.x : dxRot) + rX + scaleXH;

        const double y1 = (is3d ? anchorOffsetRot.y : dyRot) + rY - scaleYH;
        const double y2 = (is3d ? anchorOffsetRot.y : dyRot) + rY + scaleYH;

        if(scaleXH > 0) {
            boundingBoxMin.x = boundingBoxMin.x < x1 ? boundingBoxMin.x : x1;
            boundingBoxMax.x = boundingBoxMax.x > x2 ? boundingBoxMax.x : x2;
        } else {
            boundingBoxMin.x = boundingBoxMin.x < x2 ? boundingBoxMin.x : x2;
            boundingBoxMax.x = boundingBoxMax.x > x1 ? boundingBoxMax.x : x1;
        }

        if(scaleYH > 0) {
            boundingBoxMin.y = boundingBoxMin.y < y1 ? boundingBoxMin.y : y1;
            boundingBoxMax.y = boundingBoxMax.y > y2 ? boundingBoxMax.y : y2;
        } else {
            boundingBoxMin.y = boundingBoxMin.y < y2 ? boundingBoxMin.y : y2;
            boundingBoxMax.y = boundingBoxMax.y > y1 ? boundingBoxMax.y : y1;
        }

        countOffset += 1;
    }

    auto rectBoundingBox = (numberOfCharacters > 0) ? RectCoord(boundingBoxMin, boundingBoxMax) :  RectCoord(referencePoint, referencePoint);

    const float scaledTextPadding = is3d ? textPadding : scaleFactor * textPadding;

    rectBoundingBox.topLeft.x -= scaledTextPadding;
    rectBoundingBox.topLeft.y -= scaledTextPadding;

    rectBoundingBox.bottomRight.x += scaledTextPadding;
    rectBoundingBox.bottomRight.y += scaledTextPadding;

    dimensions.x = rectBoundingBox.bottomRight.x - rectBoundingBox.topLeft.x;
    dimensions.y = rectBoundingBox.bottomRight.y - rectBoundingBox.topLeft.y;

    if (rotationAlignment != SymbolAlignment::MAP) {
        if (is3d) {
            boundingBoxViewportAligned = CollisionRectD(referencePoint.x, referencePoint.y, boundingBoxMin.x - scaledTextPadding, boundingBoxMin.y - scaledTextPadding, dimensions.x, dimensions.y);
        } else {
            boundingBoxViewportAligned = CollisionRectD(referencePoint.x, referencePoint.y, boundingBoxMin.x - scaledTextPadding, boundingBoxMin.y - scaledTextPadding, dimensions.x, dimensions.y);
        }
        boundingBoxCircles = std::nullopt;
    } else {
        std::vector<CircleD> circles;
        Vec2D origin = Vec2D(dx, dy);
        Vec2D lastCirclePosition = Vec2D(0, 0);
        size_t count = numberOfCharacters;
        for (int i = 0; i < count; i++) {
            Vec2D newPos = centerPositions[i];
            newPos = Vec2DHelper::rotate(newPos, Vec2D(0, 0), angle);
            newPos.x += dxRot;
            newPos.y += dyRot;
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

double Tiled2dMapVectorSymbolLabelObject::updatePropertiesLine(std::vector<float> &positions, std::vector<float> &referencePositions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor, const double rotation, const Vec2I &viewportSize, const std::shared_ptr<MapCameraInterface>& camera) {
    if(lineCoordinates == std::nullopt) {
        countOffset += characterCount;
        return 0;
    }

    auto evalContext = EvaluationContext(zoomIdentifier, dpFactor, featureContext, stateManager);

    const double fontSize = scaleFactor * textSize;
    const double scaleCorrection = is3d ? (1.0 / scaleFactor) : 1.0;

    Vec2D boxMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2D boxMax(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

    centerPositions.clear();
    centerPositions.reserve(characterCount);

    auto currentIndex = findReferencePointIndices();

    double size = 0;
    const auto &glyphs = fontResult->fontData->glyphs;

    for(const auto &i : splittedTextInfo) {
        if(i.glyphIndex < 0) {
            size += spaceAdvance * fontSize * i.scale;
        } else {
            const auto &d = glyphs[i.glyphIndex];
            auto advance = Vec2D(d.advance.x * fontSize * i.scale, d.advance.y * fontSize * i.scale);
            size += advance.x * (1.0 + letterSpacing);
        }
    }

    // updates currentIndex
    indexAtDistance(currentIndex, -size * 0.5 * scaleCorrection, std::nullopt, currentIndex);
    
    auto yOffset = offset.y * fontSize;

    if (wasReversed) {
        yOffset *= -1;
    }

    double averageAngleS = 0.0;
    double averageAngleC = 0.0;

    int index = 0;
    double lastAngle = 0.0;
    double preLastAngle = 0.0;
    double lastAngleDiffs = 0.0;

    double lineCenteringParameter = -fontResult->fontData->info.base / fontResult->fontData->info.lineHeight;

    double maxSymbolRadius = 0.0;

    auto indexBefore = DistanceIndex(0, 0.0);
    auto indexAfter = DistanceIndex(0, 0.0);

    for(auto &i : splittedTextInfo) {
        if(i.glyphIndex < 0) {
            // updates current index
            indexAtDistance(currentIndex, spaceAdvance * fontSize * i.scale * scaleCorrection, std::nullopt, currentIndex);
            index = 0;
        } else {
            auto& d = glyphs[i.glyphIndex];
            auto charSize = Vec2D(d.boundingBoxSize.x * fontSize * i.scale, d.boundingBoxSize.y * fontSize * i.scale);
            auto bearing = Vec2D(d.bearing.x * fontSize * i.scale, d.bearing.y * fontSize * i.scale);
            auto advance = Vec2D(d.advance.x * fontSize * i.scale, d.advance.y * fontSize * i.scale);

            auto halfSpace = std::max(charSize.x, advance.x) * 0.5;

            // Punkt auf Linie
            const auto &p = pointAtIndex(currentIndex, true);

            // get before and after to calculate angle
            indexAtDistance(currentIndex, -halfSpace * scaleCorrection, p, indexBefore);
            indexAtDistance(currentIndex, halfSpace * scaleCorrection, p, indexAfter);

            const auto &before = is3d ? screenPointAtIndex(indexBefore) : pointAtIndex(indexBefore, false);
            const auto &after = is3d ? screenPointAtIndex(indexAfter) : pointAtIndex(indexAfter, false);

            double yAngle = (before.y - after.y);
            double xAngle = -(before.x - after.x);
            double sAngle = std::sqrt(xAngle*xAngle + yAngle*yAngle);

            double angleRad = atan2_approximation(yAngle, xAngle);
            double angleDeg = angleRad * (180.0 / M_PI);

            double sinAngle = yAngle / sAngle;
            double cosAngle = xAngle / sAngle;

            if(index > 0) {
                auto diff = fabs(lastAngle - angleDeg);
                auto min = std::min(360.0 - diff, diff);
                if(min + lastAngleDiffs > maxCharacterAngle) {
                    centerPositions.clear();
                    break;
                }
                lastAngleDiffs = 0.5 * lastAngleDiffs + min;
            }

            if(index > 1) {
                auto diff = fabs(preLastAngle - angleDeg);
                auto min = std::min(360.0 - diff, diff);
                if(min + lastAngleDiffs > maxCharacterAngle) {
                    centerPositions.clear();
                    break;
                }
            }

            preLastAngle = lastAngle;
            lastAngle = angleDeg;

            auto x = p.x + bearing.x;
            auto y = (is3d ? p.y + bearing.y : p.y - bearing.y) + yOffset;
            auto xw = x + charSize.x;
            auto yh = is3d ? y - charSize.y : y + charSize.y;

            averageAngleS += sinAngle / numSymbols;
            averageAngleC += cosAngle / numSymbols;

            auto lastIndex = currentIndex;
            // update currentIndex
            indexAtDistance(currentIndex, advance.x * (1.0 + letterSpacing) * scaleCorrection, p, currentIndex);

            // if we are at the end, and we were at the end (lastIndex), then clear and skip
            if(currentIndex.index == renderLineCoordinatesCount - 1 && lastIndex.index == currentIndex.index && (lastIndex.percentage == currentIndex.percentage)) {
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

                if (is3d) {
                    auto charScaled = Vec2D(d.boundingBoxSize.x * textSize * i.scale, d.boundingBoxSize.y * textSize * i.scale);
                    scales[2 * (countOffset + centerPositionSize) + 0] = (charScaled.x / viewportSize.x) * 2.0;
                    scales[2 * (countOffset + centerPositionSize) + 1] = (charScaled.y / viewportSize.x) * 2.0;

                    maxSymbolRadius = std::max(maxSymbolRadius, std::max(charScaled.x * 0.5, charScaled.y * 0.5));

                    rotations[countOffset + centerPositionSize] = angleDeg;
                } else {
                    scales[2 * (countOffset + centerPositionSize) + 0] = charSize.x;
                    scales[2 * (countOffset + centerPositionSize) + 1] = charSize.y;

                    maxSymbolRadius = std::max(maxSymbolRadius, std::max(charSize.x * 0.5, charSize.y * 0.5));

                    rotations[countOffset + centerPositionSize] = -angleDeg;
                }


                centerPositions.push_back(Vec2DHelper::midpoint(Vec2DHelper::midpoint(quad.bottomLeft, quad.bottomRight), Vec2DHelper::midpoint(quad.topLeft, quad.topRight)));
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
            if (is3d) {
                positions[(2 * countOffset) + 0] = 0;
                positions[(2 * countOffset) + 1] = 0;
                writePosition(centerPosition.x, centerPosition.y, countOffset, referencePositions);
            } else {
                writePosition(centerPosition.x, centerPosition.y, countOffset, positions);
            }

            countOffset += 1;
        }
    } else {
        for (int i = 0; i != characterCount; i++) {
            positions[(2 * countOffset) + 0] = 0;
            positions[(2 * countOffset) + 1] = 0;
            if (is3d) {
                referencePositions[positionSize * countOffset] = 0;
                referencePositions[positionSize * countOffset + 1] = 0;
                referencePositions[positionSize * countOffset + 2] = 0;
            }
            scales[2 * (countOffset) + 0] = 0;
            scales[2 * (countOffset) + 1] = 0;
            countOffset += 1;
        }
        boxMin.x = std::numeric_limits<float>::max();
    }

    assert(countOffset == countBefore + characterCount);

    if (boxMin.x != std::numeric_limits<float>::max()) {
        const float scaledTextPadding = is3d ? textPadding : scaleFactor * textPadding;

        std::vector<CircleD> circles;
        Vec2D lastCirclePosition = Vec2D(0, 0);
        size_t count = centerPositions.size();
        for (int i = 0; i < count; i++) {
            const auto &cp = centerPositions[i];
            double newX = cp.x;
            double newY = cp.y;
            if (i != count - 1 && !is3d && (std::sqrt((newX - lastCirclePosition.x) * (newX - lastCirclePosition.x) +
                                            (newY - lastCirclePosition.y) * (newY - lastCirclePosition.y))
                                  <= (maxSymbolRadius * 2.0) * collisionDistanceBias)) {
                continue;
            }
            circles.emplace_back(newX, newY, maxSymbolRadius + scaledTextPadding);
            lastCirclePosition.x = newX;
            lastCirclePosition.y = newY;
        }
        boundingBoxCircles = circles;
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

void Tiled2dMapVectorSymbolLabelObject::setupCamera(const std::shared_ptr<MapCameraInterface> &camera, const Vec2I& viewportSize) {

    // only needed for 3d text on line rendering
    if(!is3d || (renderLineCoordinatesCount == 0)) { return; }

    // check if camera is 3d camera
    auto casted = std::dynamic_pointer_cast<MapCamera3d>(camera);
    if(!casted) { return; }

    size_t i = 0;
    for(const auto& ls : cartesianRenderLineCoordinates) {
        const auto &vec2d = casted->screenPosFromCartesianCoord(ls, viewportSize);
        // don't care about systemIdentifier, only used for indexAtPoint
        screenLineCoordinates[i].x = vec2d.x;
        screenLineCoordinates[i].y = vec2d.y;
        ++i;
    }

    auto p = casted->screenPosFromCartesianCoord(cartesianReferencePoint, viewportSize);
    // don't care about systemIdentifier, only used for findReferencePointIndices
    referencePointScreen = Coord(0, (double)p.x, (double)p.y, (double)0.0);
}

DistanceIndex Tiled2dMapVectorSymbolLabelObject::findReferencePointIndices() {
    auto point = is3d ? referencePointScreen : referencePoint;
    auto distance = std::numeric_limits<double>::max();

    double tMin = 0.0f;
    int iMin = 0;

    for(int i=1; i < renderLineCoordinatesCount; ++i) {
        auto start = screenLineCoordinates[i-1];
        auto end = screenLineCoordinates[i];

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

    return DistanceIndex(iMin, tMin);
}


void Tiled2dMapVectorSymbolLabelObject::writePosition(const double x_, const double y_, const size_t offset, std::vector<float> &buffer) {
    const size_t baseIndex = positionSize * offset;
    if (is3d) {
        const double sinY = sin(y_);
        const double cosY = cos(y_);
        const double sinX = sin(x_);
        const double cosX = cos(x_);

        buffer[baseIndex]     = sinY * cosX - tileOrigin.x;
        buffer[baseIndex + 1] = cosY - tileOrigin.y;
        buffer[baseIndex + 2] = -sinY * sinX - tileOrigin.z;
    } else {
        buffer[baseIndex]     = x_ - tileOrigin.x;
        buffer[baseIndex + 1] = y_ - tileOrigin.y;
    }
}
