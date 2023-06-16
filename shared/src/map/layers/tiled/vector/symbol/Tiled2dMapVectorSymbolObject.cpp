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
#include "CoordinateSystemIdentifiers.h"


Tiled2dMapVectorSymbolObject::Tiled2dMapVectorSymbolObject(const std::weak_ptr<MapInterface> &mapInterface,
                                                           const WeakActor<Tiled2dMapVectorFontProvider> &fontProvider,
                                                           const Tiled2dMapTileInfo &tileInfo,
                                                           const std::string &layerIdentifier,
                                                           const std::shared_ptr<SymbolVectorLayerDescription> &description,
                                                           const std::shared_ptr<FeatureContext> featureContext,
                                                           const std::vector<FormattedStringEntry> &text,
                                                           const std::string &fullText,
                                                           const ::Coord &coordinate,
                                                           const std::optional<std::vector<Coord>> &lineCoordinates,
                                                           const std::vector<std::string> &fontList,
                                                           const Anchor &textAnchor,
                                                           const std::optional<double> &angle,
                                                           const TextJustify &textJustify,
                                                           const TextSymbolPlacement &textSymbolPlacement) :
    description(description), coordinate(coordinate), mapInterface(mapInterface), featureContext(featureContext),
    iconBoundingBox(Vec2D(0, 0), Vec2D(0, 0), Vec2D(0, 0), Vec2D(0, 0)),
    stretchIconBoundingBox(Vec2D(0, 0), Vec2D(0, 0), Vec2D(0, 0), Vec2D(0, 0)) {
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

    renderCoordinate = converter->convertToRenderSystem(coordinate);
    initialRenderCoordinateVec = Vec2D(renderCoordinate.x, renderCoordinate.y);

    textAllowOverlap = description->style.getTextAllowOverlap(evalContext);
    iconAllowOverlap = description->style.getIconAllowOverlap(evalContext);

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
        if (textRadialOffset != 0) {
            textOffset.x += textRadialOffset;
            textOffset.y += textRadialOffset;
        }

        const auto letterSpacing = description->style.getTextLetterSpacing(evalContext);

        labelObject = std::make_shared<Tiled2dMapVectorSymbolLabelObject>(converter, featureContext, description, text, fullText, coordinate, lineCoordinates, textAnchor, angle, textJustify, fontResult, textOffset, description->style.getTextLineHeight(evalContext), letterSpacing, description->style.getTextMaxWidth(evalContext), description->style.getTextMaxAngle(evalContext), description->style.getTextRotationAlignment(evalContext), textSymbolPlacement);

        instanceCounts.textCharacters = labelObject->getCharacterCount();

    }

    symbolSortKey = description->style.getSymbolSortKey(evalContext);
}

void Tiled2dMapVectorSymbolObject::setCollisionAt(float zoom, bool isCollision) {
    collisionMap[zoom] = isCollision;
}

std::optional<bool> Tiled2dMapVectorSymbolObject::hasCollision(float zoom) {
    if(collisionMap.size() == 0) {
        return std::nullopt;
    }

    auto low = collisionMap.lower_bound(zoom);

    if(low == collisionMap.end()) {
        auto prev = std::prev(low);
        if (std::fabs(prev->first - zoom) < 0.1) {
            return prev->second;
        }
    } else if(low == collisionMap.begin()) {
        if (std::fabs(low->first - zoom) < 0.1) {
            return low->second;
        }
    } else {
        auto prev = std::prev(low);
        if (std::fabs(zoom - prev->first) < std::fabs(zoom - low->first)) {
            low = prev;
        }
        if (std::fabs(low->first - zoom) < 0.1) {
            return low->second;
        }
    }

    return std::nullopt;
}

const Tiled2dMapVectorSymbolObject::SymbolObjectInstanceCounts Tiled2dMapVectorSymbolObject::getInstanceCounts() const {
    return instanceCounts;
}

::Coord Tiled2dMapVectorSymbolObject::getRenderCoordinates(Anchor iconAnchor, double rotation, double iconWidth, double iconHeight) {
    Vec2D anchorOffset(0, 0);

    switch (iconAnchor) {
        case Anchor::CENTER:
            break;
        case Anchor::LEFT:
            anchorOffset.x -= iconWidth / 2.0;
            break;
        case Anchor::RIGHT:
            anchorOffset.x += iconWidth / 2.0;
            break;
        case Anchor::TOP:
            anchorOffset.y -= iconHeight / 2.0;
            break;
        case Anchor::BOTTOM:
            anchorOffset.y += iconHeight / 2.0;
            break;
        case Anchor::TOP_LEFT:
            anchorOffset.x -= iconWidth / 2.0;
            anchorOffset.y -= iconHeight / 2.0;
            break;
        case Anchor::TOP_RIGHT:
            anchorOffset.x += iconWidth / 2.0;
            anchorOffset.y -= iconHeight / 2.0;
            break;
        case Anchor::BOTTOM_LEFT:
            anchorOffset.x -= iconWidth / 2.0;
            anchorOffset.y += iconHeight / 2.0;
            break;
        case Anchor::BOTTOM_RIGHT:
            anchorOffset.x += iconWidth / 2.0;
            anchorOffset.y += iconHeight / 2.0;
            break;
        default:
            break;
    }

    auto c = initialRenderCoordinateVec - anchorOffset;
    auto rotated = Vec2DHelper::rotate(c, initialRenderCoordinateVec, rotation);

    return ::Coord(renderCoordinate.systemIdentifier, rotated.x, rotated.y, renderCoordinate.z);
}

void Tiled2dMapVectorSymbolObject::setupIconProperties(std::vector<float> &positions, std::vector<float> &rotations, std::vector<float> &textureCoordinates, int &countOffset, const double zoomIdentifier, const std::shared_ptr<TextureHolderInterface> spriteTexture, const std::shared_ptr<SpriteData> spriteData) {

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

    auto iconOffset = description->style.getIconOffset(evalContext);
    initialRenderCoordinateVec.y -= iconOffset.y;
    initialRenderCoordinateVec.x += iconOffset.x;

    auto iconImage = description->style.getIconImage(evalContext);

    if (iconImage.empty() || !spriteTexture) {
        // TODO: make sure icon is not rendered
    } else {
        const auto textureWidth = (double) spriteTexture->getImageWidth();
        const auto textureHeight = (double) spriteTexture->getImageHeight();

        renderCoordinate = getRenderCoordinates(description->style.getIconAnchor(evalContext), -rotations[countOffset], textureWidth, textureHeight);

        const auto spriteIt = spriteData->sprites.find(iconImage);
        if (spriteIt == spriteData->sprites.end()) {
            LogError << "Unable to find sprite " << iconImage;
            positions[2 * countOffset] = 0;
            positions[2 * countOffset + 1] = 0;
            countOffset += instanceCounts.icons;
            return;
        }

        const double densityOffset = (camera->getScreenDensityPpi() / 160.0) / spriteIt->second.pixelRatio;

        spriteSize.x = spriteIt->second.width * densityOffset;
        spriteSize.y = spriteIt->second.height * densityOffset;

        textureCoordinates[4 * countOffset + 0] = ((double) spriteIt->second.x) / textureWidth;
        textureCoordinates[4 * countOffset + 1] = ((double) spriteIt->second.y) / textureHeight;
        textureCoordinates[4 * countOffset + 2] = ((double) spriteIt->second.width) / textureWidth;
        textureCoordinates[4 * countOffset + 3] = ((double) spriteIt->second.height) / textureHeight;

        lastIconUpdateScaleFactor = -1.0;
        lastIconUpdateRotation = -1.0;
    }

    positions[2 * countOffset] = renderCoordinate.x;
    positions[2 * countOffset + 1] = renderCoordinate.y;

    iconRotationAlignment = description->style.getIconRotationAlignment(evalContext);

    if (iconRotationAlignment == SymbolAlignment::MAP) {
        rotations[countOffset] = -description->style.getIconRotate(evalContext);
    }

    countOffset += instanceCounts.icons;
}

void Tiled2dMapVectorSymbolObject::updateIconProperties(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &alphas, int &countOffset, const double zoomIdentifier, const double scaleFactor, const double rotation) {

    if (instanceCounts.icons == 0) {
        return;
    }

    if (lastIconUpdateScaleFactor == scaleFactor && lastIconUpdateRotation == rotation) {
        countOffset += instanceCounts.icons;
        return;
    }
    if (!(description->minZoom <= zoomIdentifier && description->maxZoom >= zoomIdentifier)) {
        alphas[countOffset] = 0;
        scales[2 * countOffset] = 0;
        scales[2 * countOffset + 1] = 0;
        countOffset += instanceCounts.icons;

        lastIconUpdateScaleFactor = scaleFactor;
        lastIconUpdateRotation = rotation;
        return;
    }


    auto strongMapInterface = mapInterface.lock();
    auto converter = strongMapInterface ? strongMapInterface->getCoordinateConverterHelper() : nullptr;
    auto camera = strongMapInterface ? strongMapInterface->getCamera() : nullptr;

    const auto evalContext = EvaluationContext(zoomIdentifier, featureContext);

    if (lastIconUpdateRotation != rotation && iconRotationAlignment != SymbolAlignment::MAP) {
        rotations[countOffset] = rotation;
    }

    auto iconSize = description->style.getIconSize(evalContext) * scaleFactor;
    auto iconWidth = spriteSize.x * iconSize;
    auto iconHeight = spriteSize.y * iconSize;

    scales[2 * countOffset] = iconWidth;
    scales[2 * countOffset + 1] = iconHeight;

    renderCoordinate = getRenderCoordinates(description->style.getIconAnchor(evalContext), -rotations[countOffset], iconWidth, iconHeight);

    positions[2 * countOffset] = renderCoordinate.x;
    positions[2 * countOffset + 1] = renderCoordinate.y;

    Vec2D origin(renderCoordinate.x, renderCoordinate.y);
    iconBoundingBox.topLeft = Vec2DHelper::rotate(Vec2D(renderCoordinate.x - iconWidth * 0.5, renderCoordinate.y - iconHeight * 0.5), origin, -rotations[countOffset]);
    iconBoundingBox.topRight = Vec2DHelper::rotate(Vec2D(renderCoordinate.x + iconWidth * 0.5, renderCoordinate.y - iconHeight * 0.5), origin, -rotations[countOffset]);
    iconBoundingBox.bottomRight = Vec2DHelper::rotate(Vec2D(renderCoordinate.x + iconWidth * 0.5, renderCoordinate.y + iconHeight * 0.5), origin, -rotations[countOffset]);
    iconBoundingBox.bottomLeft = Vec2DHelper::rotate(Vec2D(renderCoordinate.x - iconWidth * 0.5, renderCoordinate.y + iconHeight * 0.5), origin, -rotations[countOffset]);

    if (collides) {
        alphas[countOffset] = 0;
        scales[2 * countOffset] = 0;
        scales[2 * countOffset + 1] = 0;
        countOffset += instanceCounts.icons;

        lastIconUpdateScaleFactor = scaleFactor;
        lastIconUpdateRotation = rotation;
        return;
    }

    alphas[countOffset] = description->style.getIconOpacity(evalContext);

    countOffset += instanceCounts.icons;

    lastIconUpdateScaleFactor = scaleFactor;
    lastIconUpdateRotation = rotation;
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

    auto iconOffset = description->style.getIconOffset(evalContext);
    renderCoordinate.y -= iconOffset.y;
    renderCoordinate.x += iconOffset.x;

    positions[2 * countOffset] = renderCoordinate.x;
    positions[2 * countOffset + 1] = renderCoordinate.y;

    auto iconImage = description->style.getIconImage(evalContext);

    if (iconImage.empty() || !spriteTexture) {
        // TODO: make sure icon is not rendered
    } else {

        const auto textureWidth = (double) spriteTexture->getImageWidth();
        const auto textureHeight = (double) spriteTexture->getImageHeight();

        const auto spriteIt = spriteData->sprites.find(iconImage);
        if (spriteIt == spriteData->sprites.end()) {
            LogError << "Unable to find sprite " << iconImage;
            positions[2 * countOffset] = 0;
            positions[2 * countOffset + 1] = 0;
            countOffset += instanceCounts.stretchedIcons;
            return;
        }

        const double densityOffset = (camera->getScreenDensityPpi() / 160.0) / spriteIt->second.pixelRatio;

        stretchSpriteSize.x = spriteIt->second.width * densityOffset;
        stretchSpriteSize.y = spriteIt->second.height * densityOffset;

        stretchSpriteInfo = spriteIt->second;

        textureCoordinates[4 * countOffset + 0] = ((double) spriteIt->second.x) / textureWidth;
        textureCoordinates[4 * countOffset + 1] = ((double) spriteIt->second.y) / textureHeight;
        textureCoordinates[4 * countOffset + 2] = ((double) spriteIt->second.width) / textureWidth;
        textureCoordinates[4 * countOffset + 3] = ((double) spriteIt->second.height) / textureHeight;

    }

    countOffset += instanceCounts.stretchedIcons;

    lastStretchIconUpdateScaleFactor = std::nullopt;
}

void Tiled2dMapVectorSymbolObject::updateStretchIconProperties(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &alphas, std::vector<float> &stretchInfos, int &countOffset, const double zoomIdentifier, const double scaleFactor, const double rotation) {

    if (instanceCounts.stretchedIcons == 0) {
        return;
    }

    if (lastStretchIconUpdateScaleFactor == zoomIdentifier && lastStretchIconUpdateRotation == rotation) {
        countOffset += instanceCounts.stretchedIcons;
        return;
    }
    lastStretchIconUpdateScaleFactor = zoomIdentifier;
    lastStretchIconUpdateRotation = rotation;

    if (collides || !(description->minZoom <= zoomIdentifier && description->maxZoom >= zoomIdentifier) || !stretchSpriteInfo) {
        alphas[countOffset] = 0;
        scales[2 * countOffset] = 0;
        scales[2 * countOffset + 1] = 0;
        positions[2 * countOffset] = 0;
        positions[2 * countOffset + 1] = 0;
        countOffset += instanceCounts.stretchedIcons;
        return;
    }

    auto strongMapInterface = mapInterface.lock();
    auto converter = strongMapInterface ? strongMapInterface->getCoordinateConverterHelper() : nullptr;
    auto camera = strongMapInterface ? strongMapInterface->getCamera() : nullptr;

    if (!converter || !camera) {
        return;
    }

    const auto evalContext = EvaluationContext(zoomIdentifier, featureContext);

    alphas[countOffset] = description->style.getIconOpacity(evalContext);

    if (lastIconUpdateRotation != rotation && iconRotationAlignment != SymbolAlignment::MAP) {
        rotations[countOffset] = rotation;
    }

    auto padding = description->style.getIconTextFitPadding(evalContext);

    const double densityOffset = (camera->getScreenDensityPpi() / 160.0) / stretchSpriteInfo->pixelRatio;

    auto spriteWidth = stretchSpriteInfo->width * scaleFactor;
    auto spriteHeight = stretchSpriteInfo->height * scaleFactor;

    const float topPadding = padding[0] * stretchSpriteInfo->pixelRatio * densityOffset * scaleFactor;
    const float rightPadding = padding[1] * stretchSpriteInfo->pixelRatio * densityOffset * scaleFactor;
    const float bottomPadding = padding[2] * stretchSpriteInfo->pixelRatio * densityOffset * scaleFactor;
    const float leftPadding = padding[3] * stretchSpriteInfo->pixelRatio * densityOffset * scaleFactor;

    const auto iconSize = description->style.getIconSize(evalContext);

    const auto textWidth = labelObject->dimensions.x + (leftPadding + rightPadding);
    const auto textHeight = labelObject->dimensions.y + (topPadding + bottomPadding);

    auto scaleX = std::max(1.0, textWidth / (spriteWidth * iconSize));
    auto scaleY = std::max(1.0, textHeight / (spriteHeight * iconSize));

    auto textFit = description->style.getIconTextFit(evalContext);

    if (textFit == IconTextFit::WIDTH || textFit == IconTextFit::BOTH) {
        spriteWidth *= scaleX;
    }
    if (textFit == IconTextFit::HEIGHT || textFit == IconTextFit::BOTH) {
        spriteHeight *= scaleY;
    }

    scales[2 * countOffset] = spriteWidth;
    scales[2 * countOffset + 1] = spriteHeight;

    Coord renderPos = renderCoordinate;
    auto iconOffset = description->style.getIconOffset(evalContext);
    auto offset = Vec2D((-iconOffset.x) * scaleFactor - leftPadding * 0.5 + rightPadding * 0.5, iconOffset.y * scaleFactor + topPadding * 0.5 - bottomPadding * 0.5);

    offset = Vec2DHelper::rotate(offset, Vec2D(0, 0), -rotation);

    positions[2 * countOffset] = renderPos.x + offset.x;
    positions[2 * countOffset + 1] = renderPos.y + offset.y;

    const float textPadding = description->style.getTextPadding(evalContext) * scaleFactor;

    Vec2D origin(positions[2 * countOffset], positions[2 * countOffset + 1]);
    stretchIconBoundingBox.topLeft = Vec2DHelper::rotate(Vec2D(positions[2 * countOffset] - spriteWidth * 0.5 - textPadding, positions[2 * countOffset + 1] - spriteHeight * 0.5 - textPadding), origin, rotations[countOffset]);
    stretchIconBoundingBox.topRight = Vec2DHelper::rotate(Vec2D(positions[2 * countOffset] + spriteWidth * 0.5 + textPadding, positions[2 * countOffset + 1] - spriteHeight * 0.5 - textPadding), origin, rotations[countOffset]);
    stretchIconBoundingBox.bottomRight = Vec2DHelper::rotate(Vec2D(positions[2 * countOffset] + spriteWidth * 0.5 + textPadding, positions[2 * countOffset + 1] + spriteHeight * 0.5 + textPadding), origin, rotations[countOffset]);
    stretchIconBoundingBox.bottomLeft = Vec2DHelper::rotate(Vec2D(positions[2 * countOffset] - spriteWidth * 0.5 - textPadding, positions[2 * countOffset + 1] + spriteHeight * 0.5 + textPadding), origin, rotations[countOffset]);

    const int infoOffset = countOffset * 10;

    // TODO: maybe most of this can be done in setup?
    stretchInfos[infoOffset + 0] = scaleX;
    stretchInfos[infoOffset + 1] = scaleY;

//TODO: fixme on Android
//#ifdef __ANDROID__
//    stretchinfo.uv.y -= stretchinfo.uv.height; // OpenGL uv coordinates are bottom to top
//    stretchinfo.uv.width *= textureWidthFactor; // Android textures are scaled to the size of a power of 2
//    stretchinfo.uv.height *= textureHeightFactor;
//#endif

    if (stretchSpriteInfo->stretchX.size() >= 1) {
        auto [begin, end] = stretchSpriteInfo->stretchX[0];
        stretchInfos[infoOffset + 2] = (begin / stretchSpriteInfo->width);
        stretchInfos[infoOffset + 3] = (end / stretchSpriteInfo->width) ;

        if (stretchSpriteInfo->stretchX.size() >= 2) {
            auto [begin1, end1] = stretchSpriteInfo->stretchX[1];
            stretchInfos[infoOffset + 4] = (begin1 / stretchSpriteInfo->width);
            stretchInfos[infoOffset + 5] = (end1 / stretchSpriteInfo->width);
        } else {
            stretchInfos[infoOffset + 4] = stretchInfos[infoOffset + 3];
            stretchInfos[infoOffset + 5] = stretchInfos[infoOffset + 3];
        }

        const float sumStretchedX = (stretchInfos[infoOffset + 3] - stretchInfos[infoOffset + 2]) + (stretchInfos[infoOffset + 5] - stretchInfos[infoOffset + 4]);
        const float sumUnstretchedX = 1.0f - sumStretchedX;
        const float scaleStretchX = (scaleX - sumUnstretchedX) / sumStretchedX;

        const float sX0 = stretchInfos[infoOffset + 2] / scaleX;
        const float eX0 = sX0 + scaleStretchX / scaleX * (stretchInfos[infoOffset + 3] - stretchInfos[infoOffset + 2]);
        const float sX1 = eX0 + (stretchInfos[infoOffset + 4] - stretchInfos[infoOffset + 3]) / scaleX;
        const float eX1 = sX1 + scaleStretchX / scaleX * (stretchInfos[infoOffset + 5] - stretchInfos[infoOffset + 4]);
        stretchInfos[infoOffset + 2] = sX0;
        stretchInfos[infoOffset + 3] = eX0;
        stretchInfos[infoOffset + 4] = sX1;
        stretchInfos[infoOffset + 5] = eX1;
    }

    if (stretchSpriteInfo->stretchY.size() >= 1) {
        auto [begin, end] = stretchSpriteInfo->stretchY[0];
        stretchInfos[infoOffset + 6] = (begin / stretchSpriteInfo->height);
        stretchInfos[infoOffset + 7] = (end / stretchSpriteInfo->height);

        if (stretchSpriteInfo->stretchY.size() >= 2) {
            auto [begin1, end1] = stretchSpriteInfo->stretchY[1];
            stretchInfos[infoOffset + 8] = (begin1 / stretchSpriteInfo->height);
            stretchInfos[infoOffset + 9] = (end1 / stretchSpriteInfo->height);
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

void Tiled2dMapVectorSymbolObject::updateTextProperties(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor, const double rotation) {
    if (lastTextUpdateScaleFactor == scaleFactor && lastTextUpdateRotation == rotation) {
        styleOffset += instanceCounts.textCharacters == 0 ? 0 : 1;
        countOffset += instanceCounts.textCharacters;
        return;
    }
    lastTextUpdateScaleFactor = scaleFactor;
    lastTextUpdateRotation = rotation;

    if (instanceCounts.textCharacters ==  0 || !labelObject) {
        styleOffset += instanceCounts.textCharacters == 0 ? 0 : 1;
        countOffset += instanceCounts.textCharacters;
        return;
    }
    labelObject->updateProperties(positions, scales, rotations, styles, countOffset, styleOffset, zoomIdentifier, scaleFactor, collides, rotation);

    lastStretchIconUpdateScaleFactor = std::nullopt;
}

std::optional<Quad2dD> Tiled2dMapVectorSymbolObject::getCombinedBoundingBox(bool considerOverlapFlag) {
    std::optional<Quad2dD> combined;

    const Quad2dD* boxes[3] = { nullptr };
    int boxCount = 0;

        if ((!considerOverlapFlag || !textAllowOverlap) && labelObject && labelObject->boundingBox.topLeft.x != 0) {
        boxes[boxCount++] = &labelObject->boundingBox;
    }
        if ((!considerOverlapFlag || !iconAllowOverlap) && iconBoundingBox.topLeft.x != 0) {
        boxes[boxCount++] = &iconBoundingBox;
    }
        if ((!considerOverlapFlag || !iconAllowOverlap) && stretchIconBoundingBox.topLeft.x != 0) {
        boxes[boxCount++] = &stretchIconBoundingBox;
    }

    if (boxCount == 0) {
        return std::nullopt;
    }

    std::vector<Vec2D> points;
    points.reserve(boxCount * 4);

    for (int i = boxCount - 1; i >= 0; --i) {
        points.push_back(boxes[i]->topLeft);
        points.push_back(boxes[i]->topRight);
        points.push_back(boxes[i]->bottomLeft);
        points.push_back(boxes[i]->bottomRight);
    }

    return Vec2DHelper::minimumAreaEnclosingRectangle(points);
}

void Tiled2dMapVectorSymbolObject::collisionDetection(const double zoomIdentifier, const double rotation, const double scaleFactor, std::shared_ptr<std::vector<OBB2D>> placements) {
    if (!(description->minZoom <= zoomIdentifier && description->maxZoom >= zoomIdentifier)) {
        iconBoundingBox.topLeft.x = 0.0;
        iconBoundingBox.topLeft.y = 0.0;
        iconBoundingBox.bottomRight.x = 0.0;
        iconBoundingBox.bottomRight.y = 0.0;

        stretchIconBoundingBox.topLeft.x = 0.0;
        stretchIconBoundingBox.topLeft.y = 0.0;
        stretchIconBoundingBox.bottomRight.x = 0.0;
        stretchIconBoundingBox.bottomRight.y = 0.0;

        if (labelObject) {
            labelObject->boundingBox.topLeft.x = 0.0;
            labelObject->boundingBox.topLeft.y = 0.0;
            labelObject->boundingBox.bottomRight.x = 0.0;
            labelObject->boundingBox.bottomRight.y = 0.0;
        }
        return;
    }

    auto const cachedCollision = hasCollision(zoomIdentifier);
    if(cachedCollision) {
        if (this->collides != *cachedCollision) {
            lastIconUpdateScaleFactor = std::nullopt;
            lastStretchIconUpdateScaleFactor = std::nullopt;
            lastTextUpdateScaleFactor = std::nullopt;
        }
        this->collides = *cachedCollision;
        if (!this->collides){
            auto combinedBox = getCombinedBoundingBox(true);
            orientedBox.update(*combinedBox);
            placements->push_back(orientedBox);
        }
        return;
    }

    auto combinedBox = getCombinedBoundingBox(true);
    if (combinedBox == std::nullopt) {
        this->collides = false;
        setCollisionAt(zoomIdentifier, false);
        return;
    }

    orientedBox.update(*combinedBox);

    for(auto it = placements->begin(); it != placements->end(); it++) {
        if (it->overlaps(orientedBox)) {
            setCollisionAt(zoomIdentifier, true);
            if (this->collides == false) {
                lastIconUpdateScaleFactor = std::nullopt;
                lastStretchIconUpdateScaleFactor = std::nullopt;
                lastTextUpdateScaleFactor = std::nullopt;
            }
            this->collides = true;
            return;
        }
    }
    setCollisionAt(zoomIdentifier, false);
    if (this->collides == true) {
        lastIconUpdateScaleFactor = std::nullopt;
        lastStretchIconUpdateScaleFactor = std::nullopt;
        lastTextUpdateScaleFactor = std::nullopt;
    }
    this->collides = false;
    placements->push_back(orientedBox);
}

void Tiled2dMapVectorSymbolObject::resetCollisionCache() {
    lastIconUpdateScaleFactor = std::nullopt;
    lastStretchIconUpdateScaleFactor = std::nullopt;
    lastTextUpdateScaleFactor = std::nullopt;
    this->collides = false;
    collisionMap.clear();
}

std::optional<std::tuple<Coord, VectorLayerFeatureInfo>> Tiled2dMapVectorSymbolObject::onClickConfirmed(const OBB2D &tinyClickBox) {
    if (collides) {
        return std::nullopt;
    }

    auto combinedBox = getCombinedBoundingBox(false);
    orientedBox.update(*combinedBox);

    if (orientedBox.overlaps(tinyClickBox)) {
        return std::make_tuple(coordinate, featureContext->getFeatureInfo());
    }
    
    return std::nullopt;
}
