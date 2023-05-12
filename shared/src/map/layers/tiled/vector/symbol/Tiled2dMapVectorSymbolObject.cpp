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
    description(description), coordinate(coordinate), mapInterface(mapInterface), featureContext(featureContext),
    iconBoundingBox(Coord("", 0.0, 0.0, 0.0), Coord("", 0.0, 0.0, 0.0)),
    stretchIconBoundingBox(Coord("", 0.0, 0.0, 0.0), Coord("", 0.0, 0.0, 0.0)){
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

void Tiled2dMapVectorSymbolObject::updateIconProperties(std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &alphas, int &countOffset, const double zoomIdentifier, const double scaleFactor, const double rotation) {
    if (lastIconUpdateZoomIdentifier == zoomIdentifier) {
        return;
    }
    lastIconUpdateZoomIdentifier = zoomIdentifier;

    double lastUpdateRotation;

    if (instanceCounts.icons == 0) {
        return;
    }

    if (collides) {
        alphas[countOffset] = 0;
        scales[2 * countOffset] = 0;
        scales[2 * countOffset + 1] = 0;
        countOffset += instanceCounts.icons;
        return;
    }

    auto strongMapInterface = mapInterface.lock();
    auto converter = strongMapInterface ? strongMapInterface->getCoordinateConverterHelper() : nullptr;
    auto camera = strongMapInterface ? strongMapInterface->getCamera() : nullptr;

    const auto evalContext = EvaluationContext(zoomIdentifier, featureContext);

    alphas[countOffset] = description->style.getIconOpacity(evalContext);

    const auto iconAlignment = description->style.getIconRotationAlignment(evalContext);

    if (iconAlignment == SymbolAlignment::MAP) {
        rotations[countOffset] = -description->style.getIconRotate(evalContext);
    } else {
        rotations[countOffset] = rotation;
    }

    auto iconSize = description->style.getIconSize(evalContext) * scaleFactor;
    scales[2 * countOffset] = spriteSize.x * iconSize;
    scales[2 * countOffset + 1] = spriteSize.y * iconSize;

    iconBoundingBox = RectCoord(Coord(renderCoordinate.systemIdentifier, renderCoordinate.x - spriteSize.x * iconSize * 0.5,
                                                                           renderCoordinate.y - spriteSize.y * iconSize * 0.5, renderCoordinate.z),
                                Coord(renderCoordinate.systemIdentifier,  renderCoordinate.x + spriteSize.x * iconSize * 0.5,
                                                                           renderCoordinate.y + spriteSize.y * iconSize * 0.5, renderCoordinate.z));
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

void Tiled2dMapVectorSymbolObject::updateStretchIconProperties(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &alphas, std::vector<float> &stretchInfos, int &countOffset, const double zoomIdentifier, const double scaleFactor, const double rotation) {
    if (lastStretchIconUpdateZoomIdentifier == zoomIdentifier) {
        return;
    }
    lastStretchIconUpdateZoomIdentifier = zoomIdentifier;

    if (instanceCounts.stretchedIcons == 0) {
        return;
    }
    
    if (collides) {
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

    //TODO: rotations
    rotations[countOffset] = 0.0;

    auto padding = description->style.getIconTextFitPadding(evalContext);

    const double densityOffset = (camera->getScreenDensityPpi() / 160.0) / stretchSpriteInfo->pixelRatio;

    auto spriteWidth = stretchSpriteInfo->width * scaleFactor;
    auto spriteHeight = stretchSpriteInfo->height * scaleFactor;

    const float topPadding = padding[0] * stretchSpriteInfo->pixelRatio * densityOffset * scaleFactor;
    const float rightPadding = padding[1] * stretchSpriteInfo->pixelRatio * densityOffset * scaleFactor;
    const float bottomPadding = padding[2] * stretchSpriteInfo->pixelRatio * densityOffset * scaleFactor;
    const float leftPadding = padding[3] * stretchSpriteInfo->pixelRatio * densityOffset * scaleFactor;

    const auto iconSize = description->style.getIconSize(evalContext);

    const auto textWidth = (labelObject->boundingBox.bottomRight.x - labelObject->boundingBox.topLeft.x) + (leftPadding + rightPadding);
    const auto textHeight = (labelObject->boundingBox.bottomRight.y - labelObject->boundingBox.topLeft.y) + (topPadding + bottomPadding);

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
    renderPos.y -= iconOffset.y * scaleFactor;
    renderPos.x += iconOffset.x * scaleFactor;

    positions[2 * countOffset] = renderCoordinate.x - leftPadding * 0.25 + rightPadding * 0.25;
    positions[2 * countOffset + 1] = renderCoordinate.y + topPadding * 0.25 + bottomPadding * 0.25;

    stretchIconBoundingBox = RectCoord(Coord(renderCoordinate.systemIdentifier, positions[2 * countOffset] - spriteWidth * 0.5,
                                                                                  positions[2 * countOffset + 1] - spriteHeight * 0.5, renderCoordinate.z),
                                       Coord(renderCoordinate.systemIdentifier,  positions[2 * countOffset] + spriteWidth * 0.5,
                                                                                  positions[2 * countOffset + 1] + spriteHeight * 0.5, renderCoordinate.z));

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

void Tiled2dMapVectorSymbolObject::updateTextProperties(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor, const double rotation) {
//    if (lastTextUpdateZoomIdentifier == zoomIdentifier) {
//        return;
//    }
//    lastTextUpdateZoomIdentifier = zoomIdentifier;

    if (instanceCounts.textCharacters ==  0 || !labelObject) {
        return;
    }
    labelObject->updateProperties(positions, scales, rotations, styles, countOffset, styleOffset, zoomIdentifier, scaleFactor, collides, rotation);
}

std::optional<RectCoord> Tiled2dMapVectorSymbolObject::getCombinedBoundingBox() {
    std::optional<RectCoord> combined;

    std::vector<RectCoord*> boxes;
    if (labelObject && !labelObject->boundingBox.topLeft.systemIdentifier.empty()) {
        boxes.push_back(&labelObject->boundingBox);
    }
    if (!iconBoundingBox.topLeft.systemIdentifier.empty()){
        boxes.push_back(&iconBoundingBox);
    }
    if (!stretchIconBoundingBox.topLeft.systemIdentifier.empty()){
        boxes.push_back(&stretchIconBoundingBox);
    }

    if (boxes.empty()) {
        return std::nullopt;
    }

    combined = *(*boxes.rbegin());
    boxes.pop_back();

    while (!boxes.empty()) {
        combined = RectCoord(Coord(combined->topLeft.systemIdentifier,std::min( combined->topLeft.x, (*boxes.rbegin())->topLeft.x), std::min(combined->topLeft.y, (*boxes.rbegin())->topLeft.y), combined->topLeft.z),
                             Coord(combined->topLeft.systemIdentifier, std::max(combined->bottomRight.x,  (*boxes.rbegin())->bottomRight.x), std::max(combined->bottomRight.y,  (*boxes.rbegin())->bottomRight.y), combined->topLeft.z));
        boxes.pop_back();
    }

    return combined;
}

void Tiled2dMapVectorSymbolObject::collisionDetection(const double zoomIdentifier, const double rotation, const double scaleFactor, std::shared_ptr<std::vector<OBB2D>> placements) {
    if(instanceCounts.textCharacters == 0) {
        this->collides = false;
        return;
    }

    auto const cachedCollision = hasCollision(zoomIdentifier);
    if(cachedCollision) {
        if (this->collides != *cachedCollision) {
            lastIconUpdateZoomIdentifier = -1;
            lastStretchIconUpdateZoomIdentifier = -1;
            lastTextUpdateZoomIdentifier = -1;
        }
        this->collides = *cachedCollision;
        if (!this->collides){
            auto combinedBox = getCombinedBoundingBox();
            OBB2D orientedBox = OBB2D(Quad2dD(Vec2D(combinedBox->topLeft.x, combinedBox->topLeft.y),
                                              Vec2D(combinedBox->bottomRight.x, combinedBox->topLeft.y),
                                              Vec2D(combinedBox->bottomRight.x, combinedBox->bottomRight.y),
                                              Vec2D(combinedBox->topLeft.x, combinedBox->bottomRight.y)));
            placements->push_back(orientedBox);
        }
        return;
    }

    auto combinedBox = getCombinedBoundingBox();
    OBB2D orientedBox = OBB2D(Quad2dD(Vec2D(combinedBox->topLeft.x, combinedBox->topLeft.y),
                                      Vec2D(combinedBox->bottomRight.x, combinedBox->topLeft.y),
                                      Vec2D(combinedBox->bottomRight.x, combinedBox->bottomRight.y),
                                      Vec2D(combinedBox->topLeft.x, combinedBox->bottomRight.y)));

    for(auto it = placements->begin(); it != placements->end(); it++) {
        if (it->overlaps(orientedBox)) {
            setCollisionAt(zoomIdentifier, true);
            if (this->collides == false) {
                lastIconUpdateZoomIdentifier = -1;
                lastStretchIconUpdateZoomIdentifier = -1;
                lastTextUpdateZoomIdentifier = -1;
            }
            this->collides = true;
            return;
        }
    }
    setCollisionAt(zoomIdentifier, false);
    if (this->collides == true) {
        lastIconUpdateZoomIdentifier = -1;
        lastStretchIconUpdateZoomIdentifier = -1;
        lastTextUpdateZoomIdentifier = -1;
    }
    this->collides = false;
    placements->push_back(orientedBox);
}
