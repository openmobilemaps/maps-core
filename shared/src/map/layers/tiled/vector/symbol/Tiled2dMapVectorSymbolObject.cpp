/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorSymbolObject.h"
#include "Tiled2dMapVectorRasterSubLayerConfig.h"
#include "CoordinateConversionHelperInterface.h"

Tiled2dMapVectorSymbolObject::Tiled2dMapVectorSymbolObject(const std::weak_ptr<MapInterface> &mapInterface,
                                                           const WeakActor<Tiled2dMapVectorFontProvider> &fontProvider,
                                                           const Tiled2dMapTileInfo &tileInfo,
                                                           const std::string &layerIdentifier,
                                                           const std::shared_ptr<SymbolVectorLayerDescription> &description,
                                                           const FeatureContext &featureContext,
                                                           const std::vector<FormattedStringEntry> &text,
                                                           const std::string &fullText,
                                                           const ::Coord &coordinate,
                                                           const std::optional<std::vector<Coord>> &lineCoordinates,
                                                           const std::vector<std::string> &fontList,
                                                           const Anchor &textAnchor,
                                                           const std::optional<double> &angle,
                                                           const TextJustify &textJustify,
                                                           const TextSymbolPlacement &textSymbolPlacement) :
    description(description), coordinate(coordinate), mapInterface(mapInterface) {
    auto strongMapInterface = mapInterface.lock();
    auto objectFactory = strongMapInterface ? strongMapInterface->getGraphicsObjectFactory() : nullptr;
    auto camera = strongMapInterface ? strongMapInterface->getCamera() : nullptr;
    auto converter = strongMapInterface ? strongMapInterface->getCoordinateConverterHelper() : nullptr;

    if (!objectFactory || !camera || !converter) {
        return;
    }

    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(camera->getZoom());

    const auto evalContext = EvaluationContext(zoomIdentifier, featureContext);

    const bool hasIcon = description->style.hasIconImagePotentially();

    if (hasIcon) {
        if (description->style.getIconTextFit(evalContext) == IconTextFit::NONE) {
            instanceCounts.icons = 1;
        } else {
            instanceCounts.stretchedIcons = 1;
        }
    }

    isInteractable = description->isInteractable(evalContext);

    const bool hasText = !fullText.empty();

    // only load the font if we actually need it
    if (hasText) {

        std::shared_ptr<FontLoaderResult> fontResult;

        for (const auto &font: fontList) {
            // try to load a font until we succeed
            fontResult = fontProvider.syncAccess([font] (auto provider) {
                return provider.lock()->loadFont(font);
            });

            if (fontResult && fontResult->status != LoaderStatus::OK) {
                break;
            }
        }


        auto textOffset = description->style.getTextOffset(evalContext);

        const auto textRadialOffset = description->style.getTextRadialOffset(evalContext);
        // TODO: currently only shifting to top right
        textOffset.x += textRadialOffset;
        textOffset.y -= textRadialOffset;

        const auto letterSpacing = description->style.getTextLetterSpacing(evalContext);

        labelObject = std::make_shared<Tiled2dMapVectorSymbolLabelObject>(converter, featureContext, description, text, fullText, coordinate, lineCoordinates, textAnchor, angle, textJustify, fontResult, textOffset, description->style.getTextLineHeight(evalContext), letterSpacing, description->style.getTextMaxWidth(evalContext), description->style.getTextMaxAngle(evalContext), description->style.getTextRotationAlignment(evalContext), textSymbolPlacement);

        instanceCounts.textCharacters = labelObject->getCharacterCount();

    }

    // expose int64_t const symbolSortKey = description->style.getSymbolSortKey(evalContext);
}

void Tiled2dMapVectorSymbolObject::setCollisionAt(float zoom, bool isCollision) {
    collisionMap[zoom] = isCollision;
}

bool Tiled2dMapVectorSymbolObject::hasCollision(float zoom) {
    {
        // TODO: make better.
        float minDiff = std::numeric_limits<float>::max();
        float min = std::numeric_limits<float>::max();

        bool collides = true;

        for(auto& cm : collisionMap) {
            float d = abs(zoom - cm.first);

            if(d < minDiff && d < min) {
                min = d;
                collides = cm.second;
            }
        }

        return collides;
    }
}

const Tiled2dMapVectorSymbolObject::SymbolObjectInstanceCounts Tiled2dMapVectorSymbolObject::getInstanceCounts() const {
    return instanceCounts;
}

void Tiled2dMapVectorSymbolObject::setupIconProperties(std::vector<float> &positions, std::vector<float> &textureCoordinates, int &countOffset, const double zoomIdentifier, const std::shared_ptr<TextureHolderInterface> spriteTexture, const std::shared_ptr<SpriteData> spriteData) {

    if (instanceCounts.icons == 0) {
        return;
    }

    auto strongMapInterface = mapInterface.lock();
    auto converter = strongMapInterface ? strongMapInterface->getCoordinateConverterHelper() : nullptr;
    auto camera = strongMapInterface ? strongMapInterface->getCamera() : nullptr;

    if (!converter || !camera) {
        return;
    }

    const auto evalContext = EvaluationContext(zoomIdentifier, featureContext);

    Coord renderPos = converter->convertToRenderSystem(coordinate);

    auto iconOffset = description->style.getIconOffset(evalContext);
    renderPos.y -= iconOffset.y;
    renderPos.x += iconOffset.x;

    positions[2 * countOffset] = renderPos.x;
    positions[2 * countOffset + 1] = renderPos.y;

    auto iconImage = description->style.getIconImage(evalContext);

    if (iconImage.empty() || !spriteTexture) {
        // TODO: make sure icon is not rendered
    } else {

        const auto textureWidth = (double) spriteTexture->getImageWidth();
        const auto textureHeight = (double) spriteTexture->getImageHeight();

        const auto &spriteInfo = spriteData->sprites.at(iconImage);

        const double densityOffset = (camera->getScreenDensityPpi() / 160.0) / spriteInfo.pixelRatio;

        spriteSize.x = spriteInfo.width * densityOffset;
        spriteSize.y = spriteInfo.height * densityOffset;

        textureCoordinates[4 * countOffset + 0] = ((double) spriteInfo.x) / textureWidth;
        textureCoordinates[4 * countOffset + 1] = ((double) spriteInfo.y) / textureHeight;
        textureCoordinates[4 * countOffset + 2] = ((double) spriteInfo.width) / textureWidth;
        textureCoordinates[4 * countOffset + 3] = ((double) spriteInfo.height) / textureHeight;

    }

    countOffset += instanceCounts.icons;
}

void Tiled2dMapVectorSymbolObject::updateIconProperties(std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &alphas, int &countOffset, const double zoomIdentifier, const double scaleFactor) {

    if (instanceCounts.icons == 0) {
        return;
    }

    const auto evalContext = EvaluationContext(zoomIdentifier, featureContext);

    alphas[countOffset] = description->style.getIconOpacity(evalContext);

    //TODO: rotations
    rotations[countOffset] = 0.0;

    auto iconSize = description->style.getIconSize(evalContext) * scaleFactor;
    scales[2 * countOffset] = spriteSize.x * iconSize;
    scales[2 * countOffset + 1] = spriteSize.y * iconSize;

    countOffset += instanceCounts.icons;
}


void Tiled2dMapVectorSymbolObject::setupStretchIconProperties(std::vector<float> &positions, std::vector<float> &textureCoordinates, int &countOffset, const double zoomIdentifier, const std::shared_ptr<TextureHolderInterface> spriteTexture, const std::shared_ptr<SpriteData> spriteData) {
    if (instanceCounts.stretchedIcons == 0) {
        return;
    }

    auto strongMapInterface = mapInterface.lock();
    auto converter = strongMapInterface ? strongMapInterface->getCoordinateConverterHelper() : nullptr;
    auto camera = strongMapInterface ? strongMapInterface->getCamera() : nullptr;

    if (!converter || !camera) {
        return;
    }

    const auto evalContext = EvaluationContext(zoomIdentifier, featureContext);

    Coord renderPos = converter->convertToRenderSystem(coordinate);

    auto iconOffset = description->style.getIconOffset(evalContext);
    renderPos.y -= iconOffset.y;
    renderPos.x += iconOffset.x;

    positions[2 * countOffset] = renderPos.x;
    positions[2 * countOffset + 1] = renderPos.y;

    auto iconImage = description->style.getIconImage(evalContext);

    if (iconImage.empty() || !spriteTexture) {
        // TODO: make sure icon is not rendered
    } else {

        const auto textureWidth = (double) spriteTexture->getImageWidth();
        const auto textureHeight = (double) spriteTexture->getImageHeight();

        const auto &spriteInfo = spriteData->sprites.at(iconImage);

        const double densityOffset = (camera->getScreenDensityPpi() / 160.0) / spriteInfo.pixelRatio;

        stretchSpriteSize.x = spriteInfo.width * densityOffset;
        stretchSpriteSize.y = spriteInfo.height * densityOffset;

        stretchSpriteInfo = spriteInfo;

        textureCoordinates[4 * countOffset + 0] = ((double) spriteInfo.x) / textureWidth;
        textureCoordinates[4 * countOffset + 1] = ((double) spriteInfo.y) / textureHeight;
        textureCoordinates[4 * countOffset + 2] = ((double) spriteInfo.width) / textureWidth;
        textureCoordinates[4 * countOffset + 3] = ((double) spriteInfo.height) / textureHeight;

    }

    countOffset += instanceCounts.stretchedIcons;
}

void Tiled2dMapVectorSymbolObject::updateStretchIconProperties(std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &alphas, std::vector<float> &stretchInfos, int &countOffset, const double zoomIdentifier, const double scaleFactor) {
    if (instanceCounts.stretchedIcons == 0) {
        return;
    }


    auto strongMapInterface = mapInterface.lock();
    auto camera = strongMapInterface ? strongMapInterface->getCamera() : nullptr;

    if (!camera) {
        return;
    }

    const auto evalContext = EvaluationContext(zoomIdentifier, featureContext);

    alphas[countOffset] = description->style.getIconOpacity(evalContext);

    //TODO: rotations
    rotations[countOffset] = 0.0;

    auto iconSize = description->style.getIconSize(evalContext) * scaleFactor;
    scales[2 * countOffset] = stretchSpriteSize.x * iconSize;
    scales[2 * countOffset + 1] = stretchSpriteSize.y * iconSize;

    auto padding = description->style.getIconTextFitPadding(evalContext);

    const double densityOffset = (camera->getScreenDensityPpi() / 160.0) / stretchSpriteInfo->pixelRatio;

    const float topPadding = padding[0] * stretchSpriteInfo->pixelRatio * densityOffset;
    const float rightPadding = padding[1] * stretchSpriteInfo->pixelRatio * densityOffset;
    const float bottomPadding = padding[2] * stretchSpriteInfo->pixelRatio * densityOffset;
    const float leftPadding = padding[3] * stretchSpriteInfo->pixelRatio * densityOffset;

    auto textWidth = (labelObject->boundingBox.bottomRight.x - labelObject->boundingBox.topLeft.x) / scaleFactor;
    textWidth += (leftPadding + rightPadding);

    auto textHeight = (labelObject->boundingBox.bottomRight.y - labelObject->boundingBox.topLeft.y) / scaleFactor;
    textHeight+= (topPadding + bottomPadding);

    auto scaleX = std::max(1.0, textWidth / scales[2 * countOffset]);
    auto scaleY = std::max(1.0, textHeight / scales[2 * countOffset + 1]);

    auto textFit = description->style.getIconTextFit(evalContext);

    if (textFit == IconTextFit::WIDTH || textFit == IconTextFit::BOTH) {
        scales[2 * countOffset] *= scaleX;
    }
    if (textFit == IconTextFit::HEIGHT || textFit == IconTextFit::BOTH) {
        scales[2 * countOffset + 1] *= scaleY;
    }

    const int infoOffset = countOffset * 10;

    // TODO: maybe most of this can be done in setup?
    
    stretchInfos[infoOffset + 0] = scaleX;
    stretchInfos[infoOffset + 1] = 1;
    stretchInfos[infoOffset + 2] = 1;
    stretchInfos[infoOffset + 3] = 1;
    stretchInfos[infoOffset + 4] = 1;
    stretchInfos[infoOffset + 5] = scaleY;
    stretchInfos[infoOffset + 6] = 1;
    stretchInfos[infoOffset + 7] = 1;
    stretchInfos[infoOffset + 8] = 1;
    stretchInfos[infoOffset + 9] = 1;

//TODO: fixme on Android
//#ifdef __ANDROID__
//    stretchinfo.uv.y -= stretchinfo.uv.height; // OpenGL uv coordinates are bottom to top
//    stretchinfo.uv.width *= textureWidthFactor; // Android textures are scaled to the size of a power of 2
//    stretchinfo.uv.height *= textureHeightFactor;
//#endif

    if (stretchSpriteInfo->stretchX.size() >= 1) {
        auto [begin, end] = stretchSpriteInfo->stretchX[0];
        stretchInfos[infoOffset + 1] = (begin / stretchSpriteInfo->width);
        stretchInfos[infoOffset + 2] = (end / stretchSpriteInfo->width) ;

        if (stretchSpriteInfo->stretchX.size() >= 2) {
            auto [begin, end] = stretchSpriteInfo->stretchX[1];
            stretchInfos[infoOffset + 3] = (begin / stretchSpriteInfo->width);
            stretchInfos[infoOffset + 4] = (end / stretchSpriteInfo->width);
        } else {
            stretchInfos[infoOffset + 3] = stretchInfos[infoOffset + 2];
            stretchInfos[infoOffset + 4] = stretchInfos[infoOffset + 2];
        }

        const float sumStretchedX = (stretchInfos[infoOffset + 2] - stretchInfos[infoOffset + 1]) + (stretchInfos[infoOffset + 4] - stretchInfos[infoOffset + 3]);
        const float sumUnstretchedX = 1.0f - sumStretchedX;
        const float scaleStretchX = (scaleX - sumUnstretchedX) / sumStretchedX;

        const float sX0 = stretchInfos[infoOffset + 1] / scaleX;
        const float eX0 = sX0 + scaleStretchX / scaleX * (stretchInfos[infoOffset + 2] - stretchInfos[infoOffset + 1]);
        const float sX1 = eX0 + (stretchInfos[infoOffset + 3] - stretchInfos[infoOffset + 2]) / scaleX;
        const float eX1 = sX1 + scaleStretchX / scaleX * (stretchInfos[infoOffset + 4] - stretchInfos[infoOffset + 3]);
        stretchInfos[infoOffset + 1] = sX0;
        stretchInfos[infoOffset + 2] = eX0;
        stretchInfos[infoOffset + 3] = sX1;
        stretchInfos[infoOffset + 4] = eX1;
    }

    if (stretchSpriteInfo->stretchY.size() >= 1) {
        auto [begin, end] = stretchSpriteInfo->stretchY[0];
        stretchInfos[infoOffset + 6] = (begin / stretchSpriteInfo->height);
        stretchInfos[infoOffset + 7] = (end / stretchSpriteInfo->height);

        if (stretchSpriteInfo->stretchY.size() >= 2) {
            auto [begin, end] = stretchSpriteInfo->stretchY[1];
            stretchInfos[infoOffset + 8] = (begin / stretchSpriteInfo->height);
            stretchInfos[infoOffset + 9] = (end / stretchSpriteInfo->height);
        } else {
            stretchInfos[infoOffset + 8] = stretchInfos[infoOffset + 7];
            stretchInfos[infoOffset + 9] = stretchInfos[infoOffset + 7];
        }

        const float sumStretchedY = (stretchInfos[infoOffset + 7] - stretchInfos[infoOffset + 6]) + (stretchInfos[infoOffset + 9] - stretchInfos[infoOffset + 8]);
        const float sumUnstretchedY = 1.0f - sumStretchedY;
        const float scaleStretchY = (scaleY - sumUnstretchedY) / sumStretchedY;

        const float sY0 = stretchInfos[infoOffset + 6] / scaleY;
        const float eY0 = sY0 + scaleStretchY / scaleY * (stretchInfos[infoOffset + 7] - stretchInfos[infoOffset + 6]);
        const float sY1 = eY0 + (stretchInfos[infoOffset + 8] - stretchInfos[infoOffset + 7]) / scaleY;
        const float eY1 = sY1 + scaleStretchY / scaleY * (stretchInfos[infoOffset + 9] - stretchInfos[infoOffset + 8]);
        stretchInfos[infoOffset + 6] = sY0;
        stretchInfos[infoOffset + 7] = eY0;
        stretchInfos[infoOffset + 8] = sY1;
        stretchInfos[infoOffset + 9] = eY1;
    }

    countOffset += instanceCounts.stretchedIcons;
}

void Tiled2dMapVectorSymbolObject::setupTextProperties(std::vector<float> &textureCoordinates, std::vector<uint16_t> &styleIndices, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier) {
    if (instanceCounts.textCharacters ==  0 || !labelObject) {
        return;
    }

    labelObject->setupProperties(textureCoordinates, styleIndices, countOffset, styleOffset, zoomIdentifier);
}

void Tiled2dMapVectorSymbolObject::updateTextProperties(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor) {
    if (instanceCounts.textCharacters ==  0 || !labelObject) {
        return;
    }
    labelObject->updateProperties(positions, scales, rotations, styles, countOffset, styleOffset, zoomIdentifier, scaleFactor);
}
