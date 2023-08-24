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
#include "Tiled2dMapVectorLayerConfig.h"
#include "CoordinateConversionHelperInterface.h"
#include "CoordinateSystemIdentifiers.h"
#include "HashedTuple.h"
#include "DateHelper.h"
#include "Tiled2dMapSourceInterface.h"
#include "Tiled2dMapSource.h"
#include "Tiled2dMapVectorStyleParser.h"

Tiled2dMapVectorSymbolObject::Tiled2dMapVectorSymbolObject(const std::weak_ptr<MapInterface> &mapInterface,
                                                           const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
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
                                                           const TextSymbolPlacement &textSymbolPlacement,
                                                           const bool hideIcon,
                                                           std::shared_ptr<SymbolAnimationCoordinatorMap> animationCoordinatorMap,
                                                           const std::shared_ptr<Tiled2dMapVectorFeatureStateManager> &featureStateManager) :
    description(description),
    layerConfig(layerConfig),
    coordinate(coordinate),
    mapInterface(mapInterface),
    featureContext(featureContext),
    iconBoundingBox(Vec2D(0, 0), Vec2D(0, 0), Vec2D(0, 0), Vec2D(0, 0)),
    iconBoundingBoxViewportAligned(0, 0, 0, 0),
    stretchIconBoundingBox(Vec2D(0, 0), Vec2D(0, 0), Vec2D(0, 0), Vec2D(0, 0)),
    stretchIconBoundingBoxViewportAligned(0, 0, 0, 0),
textSymbolPlacement(textSymbolPlacement),
featureStateManager(featureStateManager) {
    auto strongMapInterface = mapInterface.lock();
    auto objectFactory = strongMapInterface ? strongMapInterface->getGraphicsObjectFactory() : nullptr;
    auto camera = strongMapInterface ? strongMapInterface->getCamera() : nullptr;
    auto converter = strongMapInterface ? strongMapInterface->getCoordinateConverterHelper() : nullptr;
    
    if (!objectFactory || !camera || !converter) {
        return;
    }
    
    const auto evalContext = EvaluationContext(tileInfo.zoomIdentifier, featureContext, featureStateManager);
    std::string iconName = description->style.getIconImage(evalContext);
    contentHash = std::hash<std::tuple<std::string, std::string, std::string>>()(std::tuple<std::string, std::string, std::string>(layerIdentifier, iconName, fullText));
    
    const bool hasIcon = description->style.hasIconImagePotentially();
    
    renderCoordinate = converter->convertToRenderSystem(coordinate);
    initialRenderCoordinateVec = Vec2D(renderCoordinate.x, renderCoordinate.y);
    
    evaluateStyleProperties(tileInfo.zoomIdentifier);
    
    iconRotationAlignment = description->style.getIconRotationAlignment(evalContext);
    
    if (hasIcon && !hideIcon) {
        if (iconTextFit == IconTextFit::NONE) {
            instanceCounts.icons = 1;
        } else {
            instanceCounts.stretchedIcons = 1;
        }
    }

    isInteractable = description->isInteractable(evalContext);

    const bool hasText = !fullText.empty();

    crossTileIdentifier = std::hash<std::tuple<std::string, std::string, bool>>()(std::tuple<std::string, std::string, bool>(fullText, layerIdentifier, hasIcon));
    double xTolerance = std::ceil(std::abs(tileInfo.bounds.bottomRight.x - tileInfo.bounds.topLeft.x) / 4096.0);
    double yTolerance = std::ceil(std::abs(tileInfo.bounds.bottomRight.y - tileInfo.bounds.topLeft.y) / 4096.0);

    animationCoordinator = animationCoordinatorMap->getOrAddAnimationController(crossTileIdentifier, coordinate, tileInfo.zoomIdentifier, xTolerance, yTolerance);
    animationCoordinator->increaseUsage();
    if (!animationCoordinator->isOwned.test_and_set()) {
        isCoordinateOwner = true;
    }

    // only load the font if we actually need it
    if (hasText) {

        std::shared_ptr<FontLoaderResult> fontResult;

        for (const auto &font: fontList) {
            // try to load a font until we succeed
            fontResult = fontProvider.syncAccess([font] (auto provider) {
                return provider.lock()->loadFont(font);
            });

            if (fontResult && fontResult->status == LoaderStatus::OK) {
                break;
            }
        }


        auto textOffset = description->style.getTextOffset(evalContext);
        const auto textRadialOffset = description->style.getTextRadialOffset(evalContext);
        const auto letterSpacing = description->style.getTextLetterSpacing(evalContext);

        SymbolAlignment labelRotationAlignment = description->style.getTextRotationAlignment(evalContext);
        boundingBoxRotationAlignment = labelRotationAlignment;
        labelObject = std::make_shared<Tiled2dMapVectorSymbolLabelObject>(converter, featureContext, description, text, fullText, coordinate, lineCoordinates, textAnchor, angle, textJustify, fontResult, textOffset, textRadialOffset, description->style.getTextLineHeight(evalContext), letterSpacing, description->style.getTextMaxWidth(evalContext), description->style.getTextMaxAngle(evalContext), labelRotationAlignment, textSymbolPlacement, animationCoordinator, featureStateManager);

        instanceCounts.textCharacters = labelObject->getCharacterCount();

    } else {
        boundingBoxRotationAlignment = iconRotationAlignment;
    }

    if (boundingBoxRotationAlignment == SymbolAlignment::AUTO) {
        if (textSymbolPlacement == TextSymbolPlacement::POINT) {
            boundingBoxRotationAlignment = SymbolAlignment::VIEWPORT;
        } else {
            boundingBoxRotationAlignment = SymbolAlignment::MAP;
        }
    }

    symbolSortKey = description->style.getSymbolSortKey(evalContext);

    const auto &usedKeys = description->getUsedKeys();
    isStyleFeatureStateDependant = usedKeys.find(Tiled2dMapVectorStyleParser::featureStateExpression) != usedKeys.end();
}

void Tiled2dMapVectorSymbolObject::updateLayerDescription(const std::shared_ptr<SymbolVectorLayerDescription> layerDescription) {
    this->description = layerDescription;
    if (labelObject) {
        labelObject->updateLayerDescription(layerDescription);
    }

    lastZoomEvaluation = -1;

    const auto &usedKeys = description->getUsedKeys();
    isStyleFeatureStateDependant = usedKeys.find(Tiled2dMapVectorStyleParser::featureStateExpression) != usedKeys.end();

    lastIconUpdateScaleFactor = std::nullopt;
    lastIconUpdateRotation = std::nullopt;
    lastIconUpdateAlpha = std::nullopt;

    lastStretchIconUpdateScaleFactor = std::nullopt;
    lastStretchIconUpdateRotation = std::nullopt;

    lastTextUpdateScaleFactor = std::nullopt;
    lastTextUpdateRotation = std::nullopt;
}

void Tiled2dMapVectorSymbolObject::evaluateStyleProperties(const double zoomIdentifier) {

    if (isStyleZoomDependant == false && lastZoomEvaluation == -1) {
        return;
    }

    auto roundedZoom = std::round(zoomIdentifier * 100.0) / 100.0;

    if (isStyleFeatureStateDependant == false && roundedZoom == lastZoomEvaluation) {
        return;
    }
    
    const auto evalContext = EvaluationContext(roundedZoom, featureContext, featureStateManager);

    textAllowOverlap = description->style.getTextAllowOverlap(evalContext);
    iconAllowOverlap = description->style.getIconAllowOverlap(evalContext);

    iconRotate = -description->style.getIconRotate(evalContext);
    iconAnchor = description->style.getIconAnchor(evalContext);
    iconSize = description->style.getIconSize(evalContext);
    iconOffset = description->style.getIconOffset(evalContext);
    iconTextFitPadding = description->style.getIconTextFitPadding(evalContext);
    iconTextFit = description->style.getIconTextFit(evalContext);
    iconPadding = description->style.getIconPadding(evalContext);

    lastZoomEvaluation = roundedZoom;
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

    const auto evalContext = EvaluationContext(zoomIdentifier, featureContext, featureStateManager);

    initialRenderCoordinateVec.y -= iconOffset.y;
    initialRenderCoordinateVec.x += iconOffset.x;

    auto iconImage = description->style.getIconImage(evalContext);

    if (iconImage.empty() || !spriteTexture) {
        // TODO: make sure icon is not rendered
    } else {
        const auto textureWidth = (double) spriteTexture->getImageWidth();
        const auto textureHeight = (double) spriteTexture->getImageHeight();

        renderCoordinate = getRenderCoordinates(iconAnchor, -rotations[countOffset], textureWidth, textureHeight);

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

        lastIconUpdateScaleFactor = std::nullopt;
        lastIconUpdateRotation = std::nullopt;
        lastIconUpdateAlpha = std::nullopt;
    }

    positions[2 * countOffset] = renderCoordinate.x;
    positions[2 * countOffset + 1] = renderCoordinate.y;

    countOffset += instanceCounts.icons;
}

void Tiled2dMapVectorSymbolObject::updateIconProperties(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &alphas, int &countOffset, const double zoomIdentifier, const double scaleFactor, const double rotation, long long now) {

    if (instanceCounts.icons == 0) {
        return;
    }

    if (!isCoordinateOwner) {
        if (!animationCoordinator->isOwned.test_and_set()) {
            isCoordinateOwner = true;

            lastIconUpdateScaleFactor = std::nullopt;
            lastStretchIconUpdateScaleFactor = std::nullopt;
            lastTextUpdateScaleFactor = std::nullopt;
        }
    }

    if (lastIconUpdateScaleFactor && isStyleZoomDependant == false) {
        countOffset += instanceCounts.icons;
        return;
    }

    if (isStyleFeatureStateDependant == false && lastIconUpdateScaleFactor == scaleFactor && lastIconUpdateRotation == rotation && lastIconUpdateAlpha == alpha) {
        countOffset += instanceCounts.icons;
        return;
    }

    auto strongMapInterface = mapInterface.lock();
    auto converter = strongMapInterface ? strongMapInterface->getCoordinateConverterHelper() : nullptr;
    auto camera = strongMapInterface ? strongMapInterface->getCamera() : nullptr;

    evaluateStyleProperties(zoomIdentifier);

    const auto evalContext = EvaluationContext(zoomIdentifier, featureContext, featureStateManager);

    rotations[countOffset] = iconRotate;

    if (iconRotationAlignment == SymbolAlignment::VIEWPORT ||
        (iconRotationAlignment == SymbolAlignment::AUTO && textSymbolPlacement == TextSymbolPlacement::POINT)) {
        rotations[countOffset] += rotation;
    }

    const auto iconWidth = spriteSize.x * iconSize * scaleFactor;
    const auto iconHeight = spriteSize.y * iconSize * scaleFactor;

    const float scaledIconPadding = iconPadding * scaleFactor;

    scales[2 * countOffset] = iconWidth;
    scales[2 * countOffset + 1] = iconHeight;

    renderCoordinate = getRenderCoordinates(iconAnchor, -rotations[countOffset], iconWidth, iconHeight);

    positions[2 * countOffset] = renderCoordinate.x;
    positions[2 * countOffset + 1] = renderCoordinate.y;

    iconBoundingBoxViewportAligned.x = renderCoordinate.x - iconWidth * 0.5 - scaledIconPadding;
    iconBoundingBoxViewportAligned.y = renderCoordinate.y - iconWidth * 0.5 - scaledIconPadding;
    iconBoundingBoxViewportAligned.width = iconWidth + 2.0 * scaledIconPadding;
    iconBoundingBoxViewportAligned.height = iconHeight + 2.0 * scaledIconPadding;
    Vec2D origin(renderCoordinate.x, renderCoordinate.y);
    iconBoundingBox.topLeft = Vec2DHelper::rotate(Vec2D(renderCoordinate.x - iconWidth * 0.5 - scaledIconPadding, renderCoordinate.y - iconHeight * 0.5 - scaledIconPadding), origin, -rotations[countOffset]);
    iconBoundingBox.topRight = Vec2DHelper::rotate(Vec2D(renderCoordinate.x + iconWidth * 0.5 + scaledIconPadding, renderCoordinate.y - iconHeight * 0.5 - scaledIconPadding), origin, -rotations[countOffset]);
    iconBoundingBox.bottomRight = Vec2DHelper::rotate(Vec2D(renderCoordinate.x + iconWidth * 0.5 + scaledIconPadding, renderCoordinate.y + iconHeight * 0.5 + scaledIconPadding), origin, -rotations[countOffset]);
    iconBoundingBox.bottomLeft = Vec2DHelper::rotate(Vec2D(renderCoordinate.x - iconWidth * 0.5 - scaledIconPadding, renderCoordinate.y + iconHeight * 0.5 + scaledIconPadding), origin, -rotations[countOffset]);


    if (!isCoordinateOwner) {
        alphas[countOffset] = 0.0;
    } else if (!(description->minZoom <= zoomIdentifier && description->maxZoom >= zoomIdentifier)) {
        alphas[countOffset] = animationCoordinator->getIconAlpha(0.0, now);
    } else if (animationCoordinator->isColliding()) {
        alphas[countOffset] = animationCoordinator->getIconAlpha(0.0, now);
    } else {
        alphas[countOffset] = animationCoordinator->getIconAlpha(description->style.getIconOpacity(evalContext) * alpha, now);
    }

    isIconOpaque = alphas[countOffset] == 0;

    countOffset += instanceCounts.icons;

    if (!animationCoordinator->isIconAnimating()) {
        lastIconUpdateScaleFactor = scaleFactor;
        lastIconUpdateRotation = rotation;
        lastIconUpdateAlpha = alpha;
    }
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

    const auto evalContext = EvaluationContext(zoomIdentifier, featureContext, featureStateManager);

    const auto textureWidth = (double) spriteTexture->getImageWidth();
    const auto textureHeight = (double) spriteTexture->getImageHeight();
    
    initialRenderCoordinateVec.y -= iconOffset.y;
    initialRenderCoordinateVec.x += iconOffset.x;

    auto iconImage = description->style.getIconImage(evalContext);

    if (iconImage.empty() || !spriteTexture) {
        // TODO: make sure icon is not rendered
    } else {
        renderCoordinate = getRenderCoordinates(iconAnchor, 0.0, textureWidth, textureHeight);

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

    positions[2 * countOffset] = renderCoordinate.x;
    positions[2 * countOffset + 1] = renderCoordinate.y;
    
    countOffset += instanceCounts.stretchedIcons;

    lastStretchIconUpdateScaleFactor = std::nullopt;
}

void Tiled2dMapVectorSymbolObject::updateStretchIconProperties(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &alphas, std::vector<float> &stretchInfos, int &countOffset, const double zoomIdentifier, const double scaleFactor, const double rotation, long long now) {

    if (instanceCounts.stretchedIcons == 0) {
        return;
    }
    if (!isCoordinateOwner) {
        if (!animationCoordinator->isOwned.test_and_set()) {
            isCoordinateOwner = true;

            lastIconUpdateScaleFactor = std::nullopt;
            lastStretchIconUpdateScaleFactor = std::nullopt;
            lastTextUpdateScaleFactor = std::nullopt;
        }
    }

    if (!stretchSpriteInfo.has_value() || (lastStretchIconUpdateScaleFactor == zoomIdentifier && lastStretchIconUpdateRotation == rotation)) {
        countOffset += instanceCounts.stretchedIcons;
        return;
    }

    auto strongMapInterface = mapInterface.lock();
    auto converter = strongMapInterface ? strongMapInterface->getCoordinateConverterHelper() : nullptr;
    auto camera = strongMapInterface ? strongMapInterface->getCamera() : nullptr;

    if (!converter || !camera) {
        return;
    }

    evaluateStyleProperties(zoomIdentifier);

    const auto evalContext = EvaluationContext(zoomIdentifier, featureContext, featureStateManager);

    if (!isCoordinateOwner) {
        alphas[countOffset] = 0.0;
    } else if (animationCoordinator->isColliding() || !(description->minZoom <= zoomIdentifier && description->maxZoom >= zoomIdentifier) || !stretchSpriteInfo) {
        alphas[countOffset] = animationCoordinator->getStretchIconAlpha(0.0, now);
    } else {
        alphas[countOffset] = animationCoordinator->getStretchIconAlpha(description->style.getIconOpacity(evalContext) * alpha, now);
    }

    if (!animationCoordinator->isStretchIconAnimating()) {
        lastStretchIconUpdateScaleFactor = zoomIdentifier;
        lastStretchIconUpdateRotation = rotation;
    }

    isStretchIconOpaque = alphas[countOffset] == 0;

    if (lastIconUpdateRotation != rotation && iconRotationAlignment != SymbolAlignment::MAP) {
        rotations[countOffset] = rotation;
    }

    const double densityOffset = (camera->getScreenDensityPpi() / 160.0) / stretchSpriteInfo->pixelRatio;

    auto spriteWidth = stretchSpriteInfo->width * scaleFactor;
    auto spriteHeight = stretchSpriteInfo->height * scaleFactor;

    const float topPadding = iconTextFitPadding[0] * stretchSpriteInfo->pixelRatio * densityOffset * scaleFactor;
    const float rightPadding = iconTextFitPadding[1] * stretchSpriteInfo->pixelRatio * densityOffset * scaleFactor;
    const float bottomPadding = iconTextFitPadding[2] * stretchSpriteInfo->pixelRatio * densityOffset * scaleFactor;
    const float leftPadding = iconTextFitPadding[3] * stretchSpriteInfo->pixelRatio * densityOffset * scaleFactor;

    const auto textWidth = labelObject->dimensions.x + (leftPadding + rightPadding);
    const auto textHeight = labelObject->dimensions.y + (topPadding + bottomPadding);

    auto scaleX = std::max(1.0, textWidth / (spriteWidth * iconSize));
    auto scaleY = std::max(1.0, textHeight / (spriteHeight * iconSize));

    if (iconTextFit == IconTextFit::WIDTH || iconTextFit == IconTextFit::BOTH) {
        spriteWidth *= scaleX;
    }
    if (iconTextFit == IconTextFit::HEIGHT || iconTextFit == IconTextFit::BOTH) {
        spriteHeight *= scaleY;
    }

    scales[2 * countOffset] = spriteWidth;
    scales[2 * countOffset + 1] = spriteHeight;
    
    renderCoordinate = getRenderCoordinates(iconAnchor, -rotations[countOffset], spriteWidth, spriteHeight);
        
    Vec2D offset = Vec2D(0, 0);
    
    switch (iconAnchor) {
        case Anchor::CENTER:
            offset = Vec2D((-iconOffset.x) * scaleFactor - leftPadding * 0.5 + rightPadding * 0.5, iconOffset.y * scaleFactor - topPadding * 0.5 + bottomPadding * 0.5);
            break;
        case Anchor::LEFT:
            offset = Vec2D((-iconOffset.x) * scaleFactor - leftPadding, iconOffset.y * scaleFactor - topPadding * 0.5 + bottomPadding * 0.5);
            break;
        case Anchor::RIGHT:
            offset = Vec2D((-iconOffset.x) * scaleFactor + rightPadding, iconOffset.y * scaleFactor - topPadding * 0.5 + bottomPadding * 0.5);
            break;
        case Anchor::TOP:
            offset = Vec2D((-iconOffset.x) * scaleFactor - leftPadding * 0.5 + rightPadding * 0.5, iconOffset.y * scaleFactor - topPadding);
            break;
        case Anchor::BOTTOM:
            offset = Vec2D((-iconOffset.x) * scaleFactor - leftPadding * 0.5 + rightPadding * 0.5, iconOffset.y * scaleFactor + bottomPadding);
            break;
        case Anchor::TOP_LEFT:
            offset = Vec2D((-iconOffset.x) * scaleFactor - leftPadding, iconOffset.y * scaleFactor - topPadding);
            break;
        case Anchor::TOP_RIGHT:
            offset = Vec2D((-iconOffset.x) * scaleFactor + rightPadding, iconOffset.y * scaleFactor - topPadding);
            break;
        case Anchor::BOTTOM_LEFT:
            offset = Vec2D((-iconOffset.x) * scaleFactor - leftPadding, iconOffset.y * scaleFactor + bottomPadding);
            break;
        case Anchor::BOTTOM_RIGHT:
            offset = Vec2D((-iconOffset.x) * scaleFactor + rightPadding, iconOffset.y * scaleFactor + bottomPadding);
            break;
        default:
            break;
    }
    
    offset = Vec2DHelper::rotate(offset, Vec2D(0, 0), -rotation);

    positions[2 * countOffset] = renderCoordinate.x + offset.x;
    positions[2 * countOffset + 1] = renderCoordinate.y + offset.y;

    const float scaledIconPadding = iconPadding * scaleFactor;

    Vec2D origin(positions[2 * countOffset], positions[2 * countOffset + 1]);
    stretchIconBoundingBoxViewportAligned.x = origin.x - spriteWidth * 0.5 - scaledIconPadding;
    stretchIconBoundingBoxViewportAligned.y = origin.y - spriteHeight * 0.5 - scaledIconPadding;
    stretchIconBoundingBoxViewportAligned.width = spriteWidth + 2.0 * scaledIconPadding;
    stretchIconBoundingBoxViewportAligned.height = spriteHeight + 2.0 * scaledIconPadding;
    stretchIconBoundingBox.topLeft = Vec2DHelper::rotate(Vec2D(positions[2 * countOffset] - spriteWidth * 0.5 - scaledIconPadding, positions[2 * countOffset + 1] - spriteHeight * 0.5 - scaledIconPadding), origin, rotations[countOffset]);
    stretchIconBoundingBox.topRight = Vec2DHelper::rotate(Vec2D(positions[2 * countOffset] + spriteWidth * 0.5 + scaledIconPadding, positions[2 * countOffset + 1] - spriteHeight * 0.5 - scaledIconPadding), origin, rotations[countOffset]);
    stretchIconBoundingBox.bottomRight = Vec2DHelper::rotate(Vec2D(positions[2 * countOffset] + spriteWidth * 0.5 + scaledIconPadding, positions[2 * countOffset + 1] + spriteHeight * 0.5 + scaledIconPadding), origin, rotations[countOffset]);
    stretchIconBoundingBox.bottomLeft = Vec2DHelper::rotate(Vec2D(positions[2 * countOffset] - spriteWidth * 0.5 - scaledIconPadding, positions[2 * countOffset + 1] + spriteHeight * 0.5 + scaledIconPadding), origin, rotations[countOffset]);

    const int infoOffset = countOffset * 10;

    // TODO: maybe most of this can be done in setup?
    stretchInfos[infoOffset + 0] = scaleX;
    stretchInfos[infoOffset + 1] = scaleY;

    if (stretchSpriteInfo->stretchX.size() >= 1) {
        auto [begin, end] = stretchSpriteInfo->stretchX[0];
        stretchInfos[infoOffset + 2] = (begin / stretchSpriteInfo->width);
        stretchInfos[infoOffset + 3] = (end / stretchSpriteInfo->width);

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

void Tiled2dMapVectorSymbolObject::updateTextProperties(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor, const double rotation, long long now) {

    if (!isCoordinateOwner) {
        if (!animationCoordinator->isOwned.test_and_set()) {
            isCoordinateOwner = true;

            lastIconUpdateScaleFactor = std::nullopt;
            lastStretchIconUpdateScaleFactor = std::nullopt;
            lastTextUpdateScaleFactor = std::nullopt;
        }
    }


    if (lastTextUpdateScaleFactor == scaleFactor && lastTextUpdateRotation == rotation) {
        styleOffset += instanceCounts.textCharacters == 0 ? 0 : 1;
        countOffset += instanceCounts.textCharacters;
        return;
    }

    if (instanceCounts.textCharacters ==  0 || !labelObject) {
        styleOffset += instanceCounts.textCharacters == 0 ? 0 : 1;
        countOffset += instanceCounts.textCharacters;
        return;
    }

    labelObject->updateProperties(positions, scales, rotations, styles, countOffset, styleOffset, zoomIdentifier, scaleFactor,
                                      animationCoordinator->isColliding(), rotation, alpha, isCoordinateOwner, now);

    if (!animationCoordinator->isTextAnimating()) {
        lastTextUpdateScaleFactor = scaleFactor;
        lastTextUpdateRotation = rotation;
    }

    lastStretchIconUpdateScaleFactor = std::nullopt;
}


std::optional<Quad2dD> Tiled2dMapVectorSymbolObject::getCombinedBoundingBox(bool considerOverlapFlag) {
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

std::optional<CollisionRectF> Tiled2dMapVectorSymbolObject::getViewportAlignedBoundingBox(double zoomIdentifier, bool considerSymbolSpacing, bool considerOverlapFlag) {
    double minX = std::numeric_limits<double>::max(), maxX = std::numeric_limits<double>::lowest(), minY = std::numeric_limits<double>::max(), maxY = std::numeric_limits<double>::lowest();
    bool hasBox = false;

    float anchorX = renderCoordinate.x;
    float anchorY = renderCoordinate.y;

    if ((!considerOverlapFlag || !textAllowOverlap) && labelObject && labelObject->boundingBoxViewportAligned.has_value()) {
        minX = std::min(minX, std::min(labelObject->boundingBoxViewportAligned->x, labelObject->boundingBoxViewportAligned->x + labelObject->boundingBoxViewportAligned->width));
        maxX = std::max(maxX, std::max(labelObject->boundingBoxViewportAligned->x, labelObject->boundingBoxViewportAligned->x + labelObject->boundingBoxViewportAligned->width));
        minY = std::min(minY, std::min(labelObject->boundingBoxViewportAligned->y, labelObject->boundingBoxViewportAligned->y + labelObject->boundingBoxViewportAligned->height));
        maxY = std::max(maxY, std::max(labelObject->boundingBoxViewportAligned->y, labelObject->boundingBoxViewportAligned->y + labelObject->boundingBoxViewportAligned->height));
        anchorX = labelObject->boundingBoxViewportAligned->anchorX;
        anchorY = labelObject->boundingBoxViewportAligned->anchorY;
        hasBox = true;
    }
    if ((!considerOverlapFlag || !iconAllowOverlap) && iconBoundingBoxViewportAligned.x != 0) {
        minX = std::min(minX, std::min(iconBoundingBoxViewportAligned.x, iconBoundingBoxViewportAligned.x + iconBoundingBoxViewportAligned.width));
        maxX = std::max(maxX, std::max(iconBoundingBoxViewportAligned.x, iconBoundingBoxViewportAligned.x + iconBoundingBoxViewportAligned.width));
        minY = std::min(minY, std::min(iconBoundingBoxViewportAligned.y, iconBoundingBoxViewportAligned.y + iconBoundingBoxViewportAligned.height));
        maxY = std::max(maxY, std::max(iconBoundingBoxViewportAligned.y, iconBoundingBoxViewportAligned.y + iconBoundingBoxViewportAligned.height));
        hasBox = true;
    }
    if ((!considerOverlapFlag || !iconAllowOverlap) && stretchIconBoundingBoxViewportAligned.x != 0) {
        minX = std::min(minX, std::min(stretchIconBoundingBoxViewportAligned.x, stretchIconBoundingBoxViewportAligned.x + stretchIconBoundingBoxViewportAligned.width));
        maxX = std::max(maxX, std::max(stretchIconBoundingBoxViewportAligned.x, stretchIconBoundingBoxViewportAligned.x + stretchIconBoundingBoxViewportAligned.width));
        minY = std::min(minY, std::min(stretchIconBoundingBoxViewportAligned.y, stretchIconBoundingBoxViewportAligned.y + stretchIconBoundingBoxViewportAligned.height));
        maxY = std::max(maxY, std::max(stretchIconBoundingBoxViewportAligned.y, stretchIconBoundingBoxViewportAligned.y + stretchIconBoundingBoxViewportAligned.height));
        hasBox = true;
    }

    if (!hasBox) {
        return std::nullopt;
    }

    double symbolSpacingPx = 0;
    size_t contentHash = 0;
    if (considerSymbolSpacing) {
        const auto evalContext = EvaluationContext(zoomIdentifier, featureContext, featureStateManager);
        symbolSpacingPx = description->style.getSymbolSpacing(evalContext);
        contentHash = this->contentHash;
    }

    return CollisionRectF(anchorX, anchorY, minX, minY, maxX - minX, maxY - minY, contentHash, symbolSpacingPx);
}

std::optional<std::vector<CollisionCircleF>> Tiled2dMapVectorSymbolObject::getMapAlignedBoundingCircles(double zoomIdentifier, bool considerSymbolSpacing, bool considerOverlapFlag) {
    std::vector<CollisionCircleF> circles;

    double symbolSpacingPx = 0;
    size_t contentHash = 0;
    if (considerSymbolSpacing) {
        const auto evalContext = EvaluationContext(zoomIdentifier, featureContext, featureStateManager);
        symbolSpacingPx = description->style.getSymbolSpacing(evalContext);
        contentHash = this->contentHash;
    }

    if ((!considerOverlapFlag || !textAllowOverlap) && labelObject && labelObject->boundingBoxCircles.has_value()) {
        for (const auto &circle: *labelObject->boundingBoxCircles) {
            circles.emplace_back(circle.x, circle.y, circle.radius, contentHash, symbolSpacingPx);
        }
    }
    if ((!considerOverlapFlag || !iconAllowOverlap) && iconBoundingBoxViewportAligned.width != 0) {
        circles.emplace_back(iconBoundingBoxViewportAligned.x + 0.5 * iconBoundingBoxViewportAligned.width,
                             iconBoundingBoxViewportAligned.y + 0.5 * iconBoundingBoxViewportAligned.height,
                             iconBoundingBoxViewportAligned.width * 0.5, contentHash, symbolSpacingPx);
    }
    if ((!considerOverlapFlag || !iconAllowOverlap) && stretchIconBoundingBoxViewportAligned.width != 0) {
        circles.emplace_back(stretchIconBoundingBoxViewportAligned.x + 0.5 * stretchIconBoundingBoxViewportAligned.width,
                             stretchIconBoundingBoxViewportAligned.y + 0.5 * stretchIconBoundingBoxViewportAligned.height,
                             stretchIconBoundingBoxViewportAligned.width * 0.5, contentHash, symbolSpacingPx);
    }

    if (circles.empty()) {
        return std::nullopt;
    }

    return circles;
}

bool Tiled2dMapVectorSymbolObject::isPlaced() {
    if (labelObject && labelObject->boundingBox.topLeft.x != 0) {
        return true;
    }
    if (iconBoundingBox.topLeft.x != 0) {
        return true;
    }
    if (stretchIconBoundingBox.topLeft.x != 0) {
        return true;
    }
    return false;
}

void Tiled2dMapVectorSymbolObject::collisionDetection(const double zoomIdentifier, const double rotation, const double scaleFactor, std::shared_ptr<CollisionGrid> collisionGrid) {

    if (!isCoordinateOwner) {
        return;
    }

    if (!(description->minZoom <= zoomIdentifier && description->maxZoom >= zoomIdentifier) || !getIsOpaque() || !isPlaced()) {
        // not visible
        animationCoordinator->setColliding(true);
        lastIconUpdateScaleFactor = std::nullopt;
        lastStretchIconUpdateScaleFactor = std::nullopt;
        lastTextUpdateScaleFactor = std::nullopt;
        return;
    }


    bool willCollide = true;
    if (boundingBoxRotationAlignment == SymbolAlignment::VIEWPORT) {
        std::optional<CollisionRectF> boundingRect = getViewportAlignedBoundingBox(zoomIdentifier, false, true);
        // Collide, if no valid boundingRect
        if (boundingRect.has_value()) {
            willCollide = collisionGrid->addAndCheckCollisionAlignedRect(*boundingRect);
        } else {
            willCollide = false;
        }

    } else {
        std::optional<std::vector<CollisionCircleF>> boundingCircles = getMapAlignedBoundingCircles(zoomIdentifier, textSymbolPlacement != TextSymbolPlacement::POINT, true);
        // Collide, if no valid boundingCircles
        if (boundingCircles.has_value()) {
            willCollide = collisionGrid->addAndCheckCollisionCircles(*boundingCircles);
        } else {
            willCollide = false;
        }
    }

    if (animationCoordinator->setColliding(willCollide)) {
        lastIconUpdateScaleFactor = std::nullopt;
        lastStretchIconUpdateScaleFactor = std::nullopt;
        lastTextUpdateScaleFactor = std::nullopt;
    }

}

std::optional<std::tuple<Coord, VectorLayerFeatureInfo>> Tiled2dMapVectorSymbolObject::onClickConfirmed(const OBB2D &tinyClickBox) {
    if (animationCoordinator->isColliding()) {
        return std::nullopt;
    }

    auto combinedBox = getCombinedBoundingBox(false);
    orientedBox.update(*combinedBox);

    if (orientedBox.overlaps(tinyClickBox)) {
        return std::make_tuple(coordinate, featureContext->getFeatureInfo());
    }
    
    return std::nullopt;
}

void Tiled2dMapVectorSymbolObject::setAlpha(float alpha) {
    this->alpha = alpha;
}

bool Tiled2dMapVectorSymbolObject::getIsOpaque() {
    return (!labelObject || labelObject->isOpaque) || isIconOpaque || isStretchIconOpaque;
}
