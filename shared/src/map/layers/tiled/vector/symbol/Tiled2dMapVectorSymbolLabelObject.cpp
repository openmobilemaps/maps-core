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
#include "Vec3DHelper.h"
#include "SymbolAnimationCoordinator.h"
#include "Tiled2dMapVectorStyleParser.h"
#include "fast_atan2.h"
#include "Matrix.h"
#include "MapCamera3d.h"
#include "CoordinateSystemIdentifiers.h"

#include "TrigonometryLUT.h"

Tiled2dMapVectorSymbolLabelObject::Tiled2dMapVectorSymbolLabelObject(const std::shared_ptr<CoordinateConversionHelperInterface> &converter,
                                                                     const std::shared_ptr<FeatureContext> featureContext,
                                                                     const std::shared_ptr<SymbolVectorLayerDescription> &description,
                                                                     const std::vector<FormattedStringEntry> &text,
                                                                     const std::string &fullText,
                                                                     const ::Vec2D &coordinate,
                                                                     const std::optional<std::vector<Vec2D>> &lineCoordinates,
                                                                     const Anchor &textAnchor,
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
                                                                     const Vec3D &tileOrigin,
                                                                     const uint16_t styleIndex,
                                                                     const int32_t systemIdentifier)
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
          referencePoint(Vec3DHelper::toVec(converter->convertToRenderSystem(Vec2DHelper::toCoord(coordinate, systemIdentifier)))),
          animationCoordinator(animationCoordinator),
          stateManager(featureStateManager),
          dpFactor(dpFactor),
          is3d(is3d),
          tileOrigin(tileOrigin),
          positionSize(is3d ? 3 : 2),
          styleIndex(styleIndex) {

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
                       [converter, systemIdentifier](const auto& l) {
            return Vec3DHelper::toVec(converter->convertToRenderSystem(Vec2DHelper::toCoord(l, systemIdentifier)));
        });

        double sinX, cosX, sinY, cosY;

        if(is3d) {
            for(auto& c : renderLineCoordinates) {
                lut::sincos(c.y, sinY, cosY);
                lut::sincos(c.x, sinX, cosX);

                double x = c.z * sinY * cosX;
                double y = c.z * cosY;
                double z = -c.z * sinY * sinX;
                cartesianRenderLineCoordinates.emplace_back(x, y, z);
            }

            auto &c = referencePoint;
            lut::sincos(c.y, sinY, cosY);
            lut::sincos(c.x, sinX, cosX);

            cartesianReferencePoint.x = c.z * sinY * cosX;
            cartesianReferencePoint.y = c.z * cosY;
            cartesianReferencePoint.z = -c.z * sinY * sinX;
        }

        screenLineCoordinates = renderLineCoordinates;
        renderLineCoordinatesCount = renderLineCoordinates.size();

        if(!is3d) {
            currentReferencePointIndex = findReferencePointIndices();
        }
    } else {
        renderLineCoordinatesCount = 0;
    }

    precomputeMedianIfNeeded();

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

void Tiled2dMapVectorSymbolLabelObject::precomputeMedianIfNeeded() {
    if(lineCoordinates && (rotationAlignment != SymbolAlignment::VIEWPORT)) {
        return;
    }

    // calculate medians if not on line
    std::vector<double> heights;

    const auto &glyphs = fontResult->fontData->glyphs;
    int baseLineStartIndex = 0;

    auto penY = -lineHeight * 0.25;

    int c = 0;
    for(const auto &i : splittedTextInfo) {
        if(i.glyphIndex >= 0) {
            assert(i.glyphIndex < glyphs.size());
            const auto &d = glyphs[i.glyphIndex];

            auto scale = i.scale;
            auto size = Vec2D(d.boundingBoxSize.x * scale, d.boundingBoxSize.y * scale);
            auto bearing = Vec2D(d.bearing.x * scale, d.bearing.y * scale);
            auto advance = Vec2D(d.advance.x * scale, d.advance.y * scale);

            if(i.glyphIndex != spaceIndex) {
                auto y = penY - bearing.y;
                auto yh = y + size.y;
                heights.emplace_back(yh);
                c++;
            }
        } else if(i.glyphIndex == -1) {
            baseLineStartIndex = c;
            penY += lineHeight;
        } else {
            assert(false);
        }
    }

    if (heights.empty()) {
        return;
    }

    std::sort(
        heights.begin() + baseLineStartIndex,
        heights.begin() + c
    );

    // Compute the median indices
    int lineLength = c - baseLineStartIndex;
    int medianOffset = lineLength / 2;
    int median = baseLineStartIndex + medianOffset;

    if (lineLength % 2 == 0) {
        double medianBaseLineLow = heights[median - 1];
        double medianBaseLineHigh = heights[median];
        medianLastBaseLine = 0.5 * (medianBaseLineHigh + medianBaseLineLow);
    } else {
        medianLastBaseLine = heights[median];
    }
}

void Tiled2dMapVectorSymbolLabelObject::updateLayerDescription(const std::shared_ptr<SymbolVectorLayerDescription> layerDescription) {
    this->description = layerDescription;
    const auto &usedKeys = description->getUsedKeys();
    isStyleStateDependant = usedKeys.isStateDependant();
    lastZoomEvaluation = -1;

    textSize.invalidate();
    textRotate.invalidate();
    textPadding.invalidate();
    textAlignment.invalidate();

    textOpacity.invalidate();
    textColor.invalidate();
    haloColor.invalidate();
    haloWidth.invalidate();
    haloBlur.invalidate();
}

int Tiled2dMapVectorSymbolLabelObject::getCharacterCount(){
    return characterCount;
}

void Tiled2dMapVectorSymbolLabelObject::setupProperties(VectorModificationWrapper<float> &textureCoordinates, VectorModificationWrapper<uint16_t> &styleIndices, int &countOffset, const double zoomIdentifier) {
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

            styleIndices[countOffset] = styleIndex;
            countOffset += 1;
        }
    }
}

void Tiled2dMapVectorSymbolLabelObject::evaluateStyleProperties(const double zoomIdentifier) {
    auto roundedZoom = std::round(zoomIdentifier * 100.0) / 100.0;

    if (roundedZoom == lastZoomEvaluation && !isStyleStateDependant) {
        return;
    }

    const auto evalContext = EvaluationContext(roundedZoom, dpFactor, featureContext, stateManager);

    if(textOpacity.isReevaluationNeeded(evalContext)) {
        textOpacity = description->style.getTextOpacity(evalContext);
    }
    
    if (textOpacity.value == 0.0) {
        lastZoomEvaluation = roundedZoom;
        return;
    }

    if(textSize.isReevaluationNeeded(evalContext)) {
        textSize = description->style.getTextSize(evalContext);
    }

    if(textAlignment.isReevaluationNeeded(evalContext)) {
        textAlignment = description->style.getTextRotationAlignment(evalContext);
    }

    if(textRotate.isReevaluationNeeded(evalContext)) {
        textRotate = description->style.getTextRotate(evalContext);
    }

    if(textPadding.isReevaluationNeeded(evalContext)) {
        textPadding = description->style.getTextPadding(evalContext);
    }

    if(textColor.isReevaluationNeeded(evalContext)) {
        textColor = description->style.getTextColor(evalContext);
    }

    if(haloColor.isReevaluationNeeded(evalContext)) {
        haloColor = description->style.getTextHaloColor(evalContext);
    }

    auto textSizeReeval = textSize.isReevaluationNeeded(evalContext);
    if(haloWidth.isReevaluationNeeded(evalContext) || textSizeReeval) {
        haloWidth = description->style.getTextHaloWidth(evalContext, textSize);
    }

    if(haloBlur.isReevaluationNeeded(evalContext) || textSizeReeval) {
        haloBlur = description->style.getTextHaloBlur(evalContext, textSize);
    }

    lastZoomEvaluation = roundedZoom;
}


void Tiled2dMapVectorSymbolLabelObject::updateProperties(VectorModificationWrapper<float> &positions, VectorModificationWrapper<float> &referencePositions, VectorModificationWrapper<float> &scales, VectorModificationWrapper<float> &rotations, VectorModificationWrapper<float> &alphas, VectorModificationWrapper<float> &styles, int &countOffset, const double zoomIdentifier, const double scaleFactor, const bool collides, const double rotation, const float alpha, const bool isCoordinateOwner, long long now, const Vec2I &viewportSize, const std::vector<float>& vpMatrix, const Vec3D& origin) {
    const auto evalContext = EvaluationContext(zoomIdentifier, dpFactor, featureContext, stateManager);

    evaluateStyleProperties(zoomIdentifier);

    float alphaFactor;
    if (!isCoordinateOwner) {
        alphaFactor = 0.0;
    } else if (collides || !(description->minZoom <= zoomIdentifier && description->maxZoom >= zoomIdentifier)) {
        alphaFactor = animationCoordinator->getTextAlpha(0.0, now);
    } else {
        float targetAlpha = textOpacity.value * alpha;
        alphaFactor = animationCoordinator->getTextAlpha(targetAlpha, now);
    }

    float colorAlpha = textColor.value.a * alphaFactor;

    union { uint32_t i; float f; } packedColorConverter{};
    packedColorConverter.i = ((uint8_t) round(textColor.value.r * 255) << 24)
                           | ((uint8_t) round(textColor.value.g * 255) << 16)
                           | ((uint8_t) round(textColor.value.b * 255) << 8)
                           | ((uint8_t) round(colorAlpha * 255)); // color RGBA
    styles[4 * styleIndex + 0] = packedColorConverter.f;
    packedColorConverter.i = ((uint8_t) round(haloColor.value.r * 255) << 24)
                  | ((uint8_t) round(haloColor.value.g * 255) << 16)
                  | ((uint8_t) round(haloColor.value.b * 255) << 8)
                  | ((uint8_t) round(haloColor.value.a * alphaFactor * 255)); // halo color RGBA
    styles[4 * styleIndex + 1] = packedColorConverter.f;
    styles[4 * styleIndex + 2] = haloWidth.value; // halo width
    styles[4 * styleIndex + 3] = haloBlur.value; // halo blur

    isOpaque = colorAlpha > 0.0;

    switch(textSymbolPlacement) {
        case TextSymbolPlacement::POINT: {
            updatePropertiesPoint(positions, referencePositions, scales, rotations, alphas, countOffset, alphaFactor, zoomIdentifier, scaleFactor, rotation, viewportSize);
            break;
        }
        case TextSymbolPlacement::LINE_CENTER:
        case TextSymbolPlacement::LINE: {
            if (rotationAlignment == SymbolAlignment::VIEWPORT) {
                updatePropertiesPoint(positions, referencePositions, scales, rotations, alphas, countOffset, alphaFactor, zoomIdentifier, scaleFactor, rotation, viewportSize);
            } else {
                setupCameraFor3D(vpMatrix, origin, viewportSize);
                auto rotatedFactor = updatePropertiesLine(positions, referencePositions, scales, rotations, alphas, countOffset, alphaFactor, zoomIdentifier, scaleFactor, rotation, viewportSize);

                if(rotatedFactor > 0.5 && lineCoordinates) {
                    std::reverse((*lineCoordinates).begin(), (*lineCoordinates).end());
                    std::reverse(renderLineCoordinates.begin(), renderLineCoordinates.end());
                    std::reverse(screenLineCoordinates.begin(), screenLineCoordinates.end());
                    std::reverse(cartesianRenderLineCoordinates.begin(), cartesianRenderLineCoordinates.end());
                    if(!is3d) {
                        currentReferencePointIndex = findReferencePointIndices();
                    }

                    wasReversed = !wasReversed;

                    countOffset -= characterCount;

                    setupCameraFor3D(vpMatrix, origin, viewportSize);

                    updatePropertiesLine(positions, referencePositions, scales, rotations, alphas, countOffset, alphaFactor, zoomIdentifier, scaleFactor, rotation, viewportSize);
                }
            }

            break;
        }
    }
}

void Tiled2dMapVectorSymbolLabelObject::updatePropertiesPoint(VectorModificationWrapper<float> &positions, VectorModificationWrapper<float> &referencePositions, VectorModificationWrapper<float> &scales, VectorModificationWrapper<float> &rotations, VectorModificationWrapper<float> &alphas, int &countOffset, float alphaFactor, const double zoomIdentifier, const double scaleFactor, const double rotation, const Vec2I &viewportSize) {

    const auto evalContext = EvaluationContext(zoomIdentifier, dpFactor, featureContext, stateManager);
    const float fontSize = is3d ? textSize.value : (scaleFactor * textSize.value);

    Vec2D boxMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2D boxMax(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

    Vec2D centerPosBoxMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2D centerPosBoxMax(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

    auto zero = Vec2D(0.0, 0.0);
    while(centerPositions.size() < characterCount) {
        centerPositions.push_back(zero);
    }

    auto pen = zero;

    const float angle = (textAlignment.value == SymbolAlignment::MAP) ? textRotate.value : textRotate.value - rotation;

    Vec2D anchorOffset(0.0, 0.0);
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

    if (numberOfCharacters > 0) {
        lineEndIndices[lineEndIndicesIndex] = numberOfCharacters - 1;
        lineEndIndicesIndex++;
        assert((countOffset + numberOfCharacters-1)*2 < scales.size());
    }

    auto median = fontSize * medianLastBaseLine;

    const Vec2D size((boxMax.x - boxMin.x), (median - boxMin.y));

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

    Vec3D boundingBoxMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), referencePoint.z);
    Vec3D boundingBoxMax(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), referencePoint.z);

    double sinAngle, cosAngle;
    lut::sincos(angle * M_PI / 180.0, sinAngle, cosAngle);
    Vec2D anchorOffsetRot = Vec2DHelper::rotate(anchorOffset, Vec2D(0, 0), sinAngle, cosAngle);

    const auto dx = referencePoint.x + anchorOffset.x;
    const auto dy = referencePoint.y + anchorOffset.y;
    const auto dxRot = referencePoint.x + anchorOffsetRot.x;
    const auto dyRot = referencePoint.y + anchorOffsetRot.y;

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

        alphas[countOffset] = alphaFactor;

        countOffset += 1;
    }

    const float scaledTextPadding = is3d ? textPadding.value : scaleFactor * textPadding.value;

    // dimensions before rotation, with padding included
    dimensions.x = size.x + 2 * scaledTextPadding;
    dimensions.y = size.y + 2 * scaledTextPadding;

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
        const double distanceThreshold = (2.0 * maxSymbolRadius) * collisionDistanceBias;
        const double distanceThresholdSq = distanceThreshold * distanceThreshold;
        size_t count = numberOfCharacters;
        for (int i = 0; i < count; i++) {
            Vec2D newPos = centerPositions[i];
            newPos = Vec2DHelper::rotate(newPos, Vec2D(0, 0), sinAngle, cosAngle);
            newPos.x += dxRot;
            newPos.y += dyRot;
            if (i != count - 1 && Vec2DHelper::distanceSquared(newPos, lastCirclePosition) <= distanceThresholdSq) {
                continue;
            }
            circles.emplace_back(newPos.x, newPos.y, maxSymbolRadius + scaledTextPadding);
            lastCirclePosition = newPos;
        }
        boundingBoxCircles = circles;
        boundingBoxViewportAligned = std::nullopt;
    }
}

double Tiled2dMapVectorSymbolLabelObject::updatePropertiesLine(VectorModificationWrapper<float> &positions, VectorModificationWrapper<float> &referencePositions, VectorModificationWrapper<float> &scales, VectorModificationWrapper<float> &rotations, VectorModificationWrapper<float> &alphas, int &countOffset, float alphaFactor, const double zoomIdentifier, const double scaleFactor, const double rotation, const Vec2I &viewportSize) {
    if(lineCoordinates == std::nullopt) {
        countOffset += characterCount;
        return 0;
    }

    auto evalContext = EvaluationContext(zoomIdentifier, dpFactor, featureContext, stateManager);

    const double fontSize = scaleFactor * textSize.value;
    const double scaleCorrection = is3d ? (1.0 / scaleFactor) : 1.0;

    Vec2D boxMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2D boxMax(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

    centerPositions.clear();
    centerPositions.reserve(characterCount);

    auto currentIndex = currentReferencePointIndex;

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
    auto currentIndexPoint = pointForIndex(currentIndex, std::nullopt);

    switch (textAnchor) {
        case Anchor::TOP_LEFT:
        case Anchor::LEFT:
        case Anchor::BOTTOM_LEFT:
            indexAtDistance(currentIndex, currentIndexPoint, fontSize * scaleCorrection, currentIndex);
            break;
        case Anchor::TOP_RIGHT:
        case Anchor::RIGHT:
        case Anchor::BOTTOM_RIGHT:
            indexAtDistance(currentIndex, currentIndexPoint, -size * 1.0 * scaleCorrection, currentIndex);
            break;
        case Anchor::CENTER:
        case Anchor::TOP:
        case Anchor::BOTTOM:
            indexAtDistance(currentIndex, currentIndexPoint, -size * 0.5 * scaleCorrection, currentIndex);
            break;
    }
    
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
            auto currentIndexPoint = pointForIndex(currentIndex, std::nullopt);
            indexAtDistance(currentIndex, currentIndexPoint, spaceAdvance * fontSize * i.scale * scaleCorrection, currentIndex);
            index = 0;
        } else {
            auto& d = glyphs[i.glyphIndex];
            auto charSize = Vec2D(d.boundingBoxSize.x * fontSize * i.scale, d.boundingBoxSize.y * fontSize * i.scale);
            auto bearing = Vec2D(d.bearing.x * fontSize * i.scale, d.bearing.y * fontSize * i.scale);
            auto advance = Vec2D(d.advance.x * fontSize * i.scale, d.advance.y * fontSize * i.scale);

            auto halfSpace = std::max(charSize.x, advance.x) * 0.5;

            // Punkt auf Linie
            const auto &p = pointAtIndex(currentIndex, true);
            auto currentIndexPoint = pointForIndex(currentIndex, p);

            // get before and after to calculate angle
            indexAtDistance(currentIndex, currentIndexPoint, -halfSpace * scaleCorrection, indexBefore);
            indexAtDistance(currentIndex, currentIndexPoint, halfSpace * scaleCorrection, indexAfter);

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

            indexAtDistance(currentIndex, currentIndexPoint, advance.x * (1.0 + letterSpacing) * scaleCorrection, currentIndex);

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
            quad = TextHelper::rotateQuad2d(quad, p, sinAngle, cosAngle);

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
                    auto charScaled = Vec2D(d.boundingBoxSize.x * textSize.value * i.scale, d.boundingBoxSize.y * textSize.value * i.scale);
                    scales[2 * (countOffset + centerPositionSize) + 0] = (charScaled.x / viewportSize.x) * 2.0;
                    scales[2 * (countOffset + centerPositionSize) + 1] = (charScaled.y / viewportSize.x) * 2.0;

                    maxSymbolRadius = std::max(maxSymbolRadius, std::max(charScaled.x * 0.5, charScaled.y * 0.5));

                    rotations[countOffset + centerPositionSize] = angleDeg;

                    alphas[countOffset + centerPositionSize] = alphaFactor;
                } else {
                    scales[2 * (countOffset + centerPositionSize) + 0] = charSize.x;
                    scales[2 * (countOffset + centerPositionSize) + 1] = charSize.y;

                    maxSymbolRadius = std::max(maxSymbolRadius, std::max(charSize.x * 0.5, charSize.y * 0.5));

                    rotations[countOffset + centerPositionSize] = -angleDeg;

                    alphas[countOffset + centerPositionSize] = alphaFactor;
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

        isPlaced = true;
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
            alphas[countOffset] = alphaFactor;
            countOffset += 1;
        }
        boxMin.x = std::numeric_limits<float>::max();

        isPlaced = false;
    }

    assert(countOffset == countBefore + characterCount);

    if (boxMin.x != std::numeric_limits<float>::max()) {
        const float scaledTextPadding = is3d ? textPadding.value : scaleFactor * textPadding.value;

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

void Tiled2dMapVectorSymbolLabelObject::setupCameraFor3D(const std::vector<float>& vpMatrix, const Vec3D& origin, const Vec2I& viewportSize) {

    // only needed for 3d text on line rendering
    if(!is3d || (renderLineCoordinatesCount == 0)) { return; }

    size_t i = 0;
    for(const auto& ls : cartesianRenderLineCoordinates) {
        const auto &cc = Vec4D(ls.x - origin.x, ls.y - origin.y, ls.z - origin.z, 1.0);
        const auto &projected = Matrix::multiply(vpMatrix, cc);

        // Map from [-1, 1] to screenPixels, with (0,0) being the top left corner
        double screenXDiffToCenter = projected.x * viewportSize.x / 2.0;
        double screenYDiffToCenter = projected.y * viewportSize.y / 2.0;

        double posScreenX = screenXDiffToCenter + ((double)viewportSize.x / 2.0);
        double posScreenY = ((double)viewportSize.y / 2.0) - screenYDiffToCenter;

        // don't care about systemIdentifier, only used for indexAtPoint
        screenLineCoordinates[i].x = posScreenX;
        screenLineCoordinates[i].y = posScreenY;
        ++i;
    }

    const auto &cc = Vec4D(cartesianReferencePoint.x - origin.x, cartesianReferencePoint.y - origin.y, cartesianReferencePoint.z - origin.z, 1.0);
    const auto &projected = Matrix::multiply(vpMatrix, cc);

    // Map from [-1, 1] to screenPixels, with (0,0) being the top left corner
    double screenXDiffToCenter = projected.x * viewportSize.x / 2.0;
    double screenYDiffToCenter = projected.y * viewportSize.y / 2.0;

    double posScreenX = screenXDiffToCenter + ((double)viewportSize.x / 2.0);
    double posScreenY = ((double)viewportSize.y / 2.0) - screenYDiffToCenter;

    // don't care about systemIdentifier, only used for findReferencePointIndices
    referencePointScreen = Vec3D((double)posScreenX, (double)posScreenY, 0.0);
    currentReferencePointIndex = findReferencePointIndices();
}

DistanceIndex Tiled2dMapVectorSymbolLabelObject::findReferencePointIndices() {
    auto point = is3d ? referencePointScreen : referencePoint;
    auto distance = std::numeric_limits<double>::max();

    auto point2D = Vec2D(point.x, point.y);

    double tMin = 0.0f;
    int iMin = 0;

    for(int i=1; i < renderLineCoordinatesCount; ++i) {
        const auto &start = screenLineCoordinates[i-1];
        const auto &end = screenLineCoordinates[i];

        auto lengthSquared = Vec2DHelper::distanceSquared(Vec2D(start.x, start.y), Vec2D(end.x, end.y));

        double t = 0.0;
        double dist = 0.0;

        if(lengthSquared > 0) {
            auto endMinusStart = Vec2D(end.x - start.x, end.y - start.y);
            auto dot = Vec2D(point.x - start.x, point.y - start.y) * endMinusStart;
            t = dot / lengthSquared;

            if(t > 1.0) {
                // outside, so skip computation
                continue;
            }

            auto proj = Vec2D(start.x + t * endMinusStart.x, start.y + t * endMinusStart.y);
            dist = Vec2DHelper::distanceSquared(proj, point2D);
        } else {
            // here t == 0.0
            dist = Vec2DHelper::distanceSquared(Vec2D(start.x, start.y), point2D);
        }

        if(dist < distance) {
            tMin = t;
            iMin = i-1;
            distance = dist;
        }
    }

    return DistanceIndex(iMin, tMin);
}

void Tiled2dMapVectorSymbolLabelObject::writePosition(const double x_, const double y_, const size_t offset, VectorModificationWrapper<float> &buffer) {
    const size_t baseIndex = positionSize * offset;
    if (is3d) {
        double sinX, cosX, sinY, cosY;
        lut::sincos(y_, sinY, cosY);
        lut::sincos(x_, sinX, cosX);

        buffer[baseIndex]     = sinY * cosX - tileOrigin.x;
        buffer[baseIndex + 1] = cosY - tileOrigin.y;
        buffer[baseIndex + 2] = -sinY * sinX - tileOrigin.z;
    } else {
        buffer[baseIndex]     = x_ - tileOrigin.x;
        buffer[baseIndex + 1] = y_ - tileOrigin.y;
    }
}
