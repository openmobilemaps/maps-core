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
#include "CollisionUtil.h"
#include "ExceptionLogger.h"
#include "MapCamera3dInterface.h"
#include "MapCamera3d.h"

Tiled2dMapVectorSymbolObject::Tiled2dMapVectorSymbolObject(const std::weak_ptr<MapInterface> &mapInterface,
                                                           const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                                           const WeakActor<Tiled2dMapVectorFontProvider> &fontProvider,
                                                           const Tiled2dMapVersionedTileInfo &tileInfo,
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
                                                           const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager,
                                                           const UsedKeysCollection &usedKeys,
                                                           const size_t symbolTileIndex,
                                                           const bool hasCustomTexture,
                                                           const double dpFactor,
                                                           const bool persistingSymbolPlacement,
                                                           bool is3d,
                                                           const Vec3D &tileOrigin) :
    description(description),
    layerConfig(layerConfig),
    coordinate(coordinate),
    mapInterface(mapInterface),
    featureContext(featureContext),
    iconBoundingBoxViewportAligned(0, 0, 0, 0),
    stretchIconBoundingBoxViewportAligned(0, 0, 0, 0),
    textSymbolPlacement(textSymbolPlacement),
    featureStateManager(featureStateManager),
    symbolTileIndex(symbolTileIndex),
    hasCustomTexture(hasCustomTexture),
    dpFactor(dpFactor),
    is3d(is3d),
    positionSize(is3d ? 3 : 2),
    tileOrigin(tileOrigin),
    persistingSymbolPlacement(persistingSymbolPlacement) {
    auto strongMapInterface = mapInterface.lock();
    auto objectFactory = strongMapInterface ? strongMapInterface->getGraphicsObjectFactory() : nullptr;
    auto camera = strongMapInterface ? strongMapInterface->getCamera() : nullptr;
    auto converter = strongMapInterface ? strongMapInterface->getCoordinateConverterHelper() : nullptr;
    
    if (!objectFactory || !camera || !converter) {
        return;
    }
    
    const auto evalContext = EvaluationContext(tileInfo.tileInfo.zoomIdentifier, dpFactor, featureContext, featureStateManager);
    std::string iconName = description->style.getIconImage(evalContext);
    contentHash = std::hash<std::tuple<std::string, std::string, std::string>>()(std::tuple<std::string, std::string, std::string>(layerIdentifier, iconName, fullText));
    
    const bool hasIcon = description->style.hasIconImagePotentially();
    
    renderCoordinate = converter->convertToRenderSystem(coordinate);
    initialRenderCoordinateVec = Vec2D(renderCoordinate.x, renderCoordinate.y);
    
    evaluateStyleProperties(tileInfo.tileInfo.zoomIdentifier);

    iconRotationAlignment = description->style.getIconRotationAlignment(evalContext);

    stringIdentifier = std::to_string(featureContext->identifier);

    if (hasIcon && !hideIcon) {
        if (iconTextFit == IconTextFit::NONE) {
            instanceCounts.icons = 1;
        } else {
            instanceCounts.stretchedIcons = 1;
        }
    }

    isInteractable = description->isInteractable(evalContext);

    const bool hasText = !fullText.empty();
    const size_t contentHash = usedKeys.getHash(evalContext);

    crossTileIdentifier = std::hash<std::tuple<std::string, std::string, bool, size_t>>()(std::tuple<std::string, std::string, bool, size_t>(fullText, layerIdentifier, hasIcon, contentHash));
    double xTolerance = std::ceil(std::abs(tileInfo.tileInfo.bounds.bottomRight.x - tileInfo.tileInfo.bounds.topLeft.x) / 4096.0);
    double yTolerance = std::ceil(std::abs(tileInfo.tileInfo.bounds.bottomRight.y - tileInfo.tileInfo.bounds.topLeft.y) / 4096.0);

    animationCoordinator = animationCoordinatorMap->getOrAddAnimationController(crossTileIdentifier, coordinate, tileInfo.tileInfo.zoomIdentifier, xTolerance, yTolerance, description->style.getTransitionDuration(), description->style.getTransitionDelay());
    animationCoordinator->increaseUsage();
    if (!animationCoordinator->isOwned.test_and_set()) {
        isCoordinateOwner = true;
    }

    // only load the font if we actually need it
    if (hasText) {

        std::shared_ptr<FontLoaderResult> fontResult = nullptr;

        for (const auto &font: fontList) {
            // try to load a font until we succeed
            fontResult = fontProvider.syncAccess([font] (auto provider) -> std::shared_ptr<FontLoaderResult>  {
                auto ptr = provider.lock();
                if (ptr) {
                    return ptr->loadFont(font);
                } else {
                    return nullptr;
                }
            });

            if (fontResult && fontResult->status == LoaderStatus::OK) {
                break;
            }
        }

        if (fontResult && fontResult->status == LoaderStatus::OK) {
            auto textOffset = description->style.getTextOffset(evalContext);
            const auto textRadialOffset = description->style.getTextRadialOffset(evalContext);
            const auto letterSpacing = description->style.getTextLetterSpacing(evalContext);

            SymbolAlignment labelRotationAlignment = description->style.getTextRotationAlignment(evalContext);
            boundingBoxRotationAlignment = labelRotationAlignment;
            labelObject = std::make_shared<Tiled2dMapVectorSymbolLabelObject>(converter, featureContext, description, text, fullText,
                                                                              coordinate, lineCoordinates, textAnchor, angle,
                                                                              textJustify, fontResult, textOffset, textRadialOffset,
                                                                              description->style.getTextLineHeight(evalContext),
                                                                              letterSpacing,
                                                                              description->style.getTextMaxWidth(evalContext),
                                                                              description->style.getTextMaxAngle(evalContext),
                                                                              labelRotationAlignment, textSymbolPlacement,
                                                                              animationCoordinator, featureStateManager,
                                                                              dpFactor,
                                                                              is3d,
                                                                              tileOrigin);

            instanceCounts.textCharacters = labelObject->getCharacterCount();
        } else {
            // font could not be loaded
            std::string fontString;
            for (const auto &font: fontList) {
                fontString.append(font + ",");
            }
            fontString.pop_back();
            ExceptionLogger::instance().logMessage("map-core:Tiled2dMapVectorSymbolObject", 0, "font (" + fontString + ") could not be loaded");
            LogError << "font (" << fontString <<=") could not be loaded";
        }
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

    isStyleStateDependant = usedKeys.isStateDependant();
}

Tiled2dMapVectorSymbolObject::~Tiled2dMapVectorSymbolObject() {
    if (animationCoordinator) {
        if (isCoordinateOwner) {
            animationCoordinator->isOwned.clear();
            isCoordinateOwner = false;
        }
        animationCoordinator->decreaseUsage();
    }
}

void Tiled2dMapVectorSymbolObject::placedInCache() {
    if (animationCoordinator) {
        if (isCoordinateOwner) {
            animationCoordinator->isOwned.clear();
            isCoordinateOwner = false;
        }
        animationCoordinator->increaseCache();
    }
}

void Tiled2dMapVectorSymbolObject::removeFromCache() {
    if (animationCoordinator) {
        animationCoordinator->decreaseCache();
    }
}

void Tiled2dMapVectorSymbolObject::updateLayerDescription(const std::shared_ptr<SymbolVectorLayerDescription> layerDescription,
                                                          const UsedKeysCollection &usedKeys) {
    this->description = layerDescription;
    if (labelObject) {
        labelObject->updateLayerDescription(layerDescription);
    }

    lastZoomEvaluation = -1;

    isStyleStateDependant = usedKeys.isStateDependant();

    lastIconUpdateScaleFactor = -1;
    lastIconUpdateRotation = -1;
    lastIconUpdateAlpha = -1;

    lastStretchIconUpdateScaleFactor = -1;
    lastStretchIconUpdateRotation = -1;

    lastTextUpdateScaleFactor = -1;
    lastTextUpdateRotation = -1;
}

void Tiled2dMapVectorSymbolObject::evaluateStyleProperties(const double zoomIdentifier) {

    if (!isStyleZoomDependant && lastZoomEvaluation == -1) {
        return;
    }

    auto roundedZoom = std::round(zoomIdentifier * 100.0) / 100.0;

    if (!isStyleStateDependant && roundedZoom == lastZoomEvaluation) {
        return;
    }
    
    const auto evalContext = EvaluationContext(roundedZoom, dpFactor, featureContext, featureStateManager);

    iconOpacity = description->style.getIconOpacity(evalContext);

    if (iconOpacity == 0.0) {
        lastZoomEvaluation = roundedZoom;
        return;
    }

    textAllowOverlap = description->style.getTextAllowOverlap(evalContext);
    iconAllowOverlap = description->style.getIconAllowOverlap(evalContext);

    iconRotate = -description->style.getIconRotate(evalContext);
    iconSize = description->style.getIconSize(evalContext);
    iconOffset = description->style.getIconOffset(evalContext);
    iconTextFit = description->style.getIconTextFit(evalContext);
    iconPadding = description->style.getIconPadding(evalContext);

    if(description->style.symbolSortKeyNeedsRecomputation()) {
        symbolSortKey = description->style.getSymbolSortKey(evalContext);
    }

    // only evaluate these properties once since they are expensive and should not change
    if (lastZoomEvaluation == -1) {
        iconTextFitPadding = description->style.getIconTextFitPadding(evalContext);
        iconAnchor = description->style.getIconAnchor(evalContext);
    }

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

void Tiled2dMapVectorSymbolObject::setupIconProperties(std::vector<float> &positions, std::vector<float> &rotations, std::vector<float> &textureCoordinates, int &countOffset, const double zoomIdentifier, const std::shared_ptr<TextureHolderInterface> spriteTexture, const std::shared_ptr<SpriteData> spriteData, const std::optional<RectI> customUv) {

    if (instanceCounts.icons == 0) {
        return;
    }

    auto strongMapInterface = mapInterface.lock();
    auto converter = strongMapInterface ? strongMapInterface->getCoordinateConverterHelper() : nullptr;
    auto camera = strongMapInterface ? strongMapInterface->getCamera() : nullptr;

    if (!converter || !camera) {
        return;
    }

    const auto evalContext = EvaluationContext(zoomIdentifier, dpFactor, featureContext, featureStateManager);

    auto iconImage = description->style.getIconImage(evalContext);

    if ((iconImage.empty() && !hasCustomTexture) || !spriteTexture) {
        // TODO: make sure icon is not rendered
    } else {
        const auto textureWidth = (double) spriteTexture->getImageWidth();
        const auto textureHeight = (double) spriteTexture->getImageHeight();

        if (is3d) {
            renderCoordinate = getRenderCoordinates(Anchor::CENTER, -rotations[countOffset], textureWidth, textureHeight);
        } else {
            renderCoordinate = getRenderCoordinates(iconAnchor, -rotations[countOffset], textureWidth, textureHeight);
        }


        int spriteX = 0;
        int spriteY = 0;
        int spriteWidth = textureWidth;
        int spriteHeight = textureHeight;
        float spritePixelRatio = float(camera->getScreenDensityPpi() / 160.0);

        if (!hasCustomTexture) {
            const auto spriteIt = spriteData->sprites.find(iconImage);
            if (spriteIt == spriteData->sprites.end()) {
                LogError << "Unable to find sprite " << iconImage;
                writePosition(0, 0, countOffset, positions);
                countOffset += instanceCounts.icons;
                return;
            }
            spriteX = spriteIt->second.x;
            spriteY = spriteIt->second.y;
            spriteWidth = spriteIt->second.width;
            spriteHeight = spriteIt->second.height;
            spritePixelRatio = spriteIt->second.pixelRatio;
        } else {
            spriteX = customUv->x;
            spriteY = customUv->y;
            spriteWidth = customUv->width;
            spriteHeight = customUv->height;
        }

        const double densityOffset = (camera->getScreenDensityPpi() / 160.0) / spritePixelRatio;

        spriteSize.x = spriteWidth * densityOffset;
        spriteSize.y = spriteHeight * densityOffset;

        textureCoordinates[4 * countOffset + 0] = ((double) spriteX) / textureWidth;
        textureCoordinates[4 * countOffset + 1] = ((double) spriteY) / textureHeight;
        textureCoordinates[4 * countOffset + 2] = ((double) spriteWidth) / textureWidth;
        textureCoordinates[4 * countOffset + 3] = ((double) spriteHeight) / textureHeight;

        lastIconUpdateScaleFactor = -1;
        lastIconUpdateRotation = -1;
        lastIconUpdateAlpha = -1;
    }
    writePosition(renderCoordinate.x, renderCoordinate.y, countOffset, positions);

    countOffset += instanceCounts.icons;
}

void Tiled2dMapVectorSymbolObject::updateIconProperties(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &alphas, std::vector<float> &offsets, int &countOffset, const double zoomIdentifier, const double scaleFactor, const double rotation, long long now, const Vec2I viewPortSize) {

    if (instanceCounts.icons == 0) {
        return;
    }

    if (!isCoordinateOwner) {
        if (!animationCoordinator->isOwned.test_and_set()) {
            isCoordinateOwner = true;

            lastIconUpdateScaleFactor = -1;
            lastStretchIconUpdateScaleFactor = -1;
            lastTextUpdateScaleFactor = -1;
        }
    }

    if (lastIconUpdateScaleFactor != -1 && !isStyleZoomDependant) {
        countOffset += instanceCounts.icons;
        return;
    }

    if (!isStyleStateDependant && lastIconUpdateScaleFactor == scaleFactor && lastIconUpdateRotation == rotation && lastIconUpdateAlpha == alpha) {
        countOffset += instanceCounts.icons;
        return;
    }

    auto strongMapInterface = mapInterface.lock();
    auto converter = strongMapInterface ? strongMapInterface->getCoordinateConverterHelper() : nullptr;
    auto camera = strongMapInterface ? strongMapInterface->getCamera() : nullptr;

    evaluateStyleProperties(zoomIdentifier);

    rotations[countOffset] = iconRotate;

    if (iconRotationAlignment == SymbolAlignment::VIEWPORT ||
        (iconRotationAlignment == SymbolAlignment::AUTO && textSymbolPlacement == TextSymbolPlacement::POINT)) {
        rotations[countOffset] += rotation;
    }

    const auto iconWidth = spriteSize.x * iconSize * (is3d ? 1.0 : scaleFactor);
    const auto iconHeight = spriteSize.y * iconSize * (is3d ? 1.0 : scaleFactor);

    const float scaledIconPadding = iconPadding * (is3d ? 1.0 : scaleFactor);

    scales[2 * countOffset] = iconWidth;
    scales[2 * countOffset + 1] = iconHeight;

    if (is3d) {
        scales[2 * countOffset] *= 2.0 / viewPortSize.x;
        scales[2 * countOffset + 1] *= 2.0 / viewPortSize.y;
    }

    if (is3d) {
        offsets[2 * countOffset] = 0;
        offsets[2 * countOffset + 1] = 0;

        switch (iconAnchor) {
            case Anchor::CENTER:
                offsets[2 * countOffset] = 0;
                offsets[2 * countOffset + 1] = 0;
                break;
            case Anchor::LEFT:
                offsets[2 * countOffset] = iconWidth * 1.0 / viewPortSize.x;
                offsets[2 * countOffset + 1] = 0;
                break;
            case Anchor::RIGHT:
                offsets[2 * countOffset] = -iconWidth * 1.0 / viewPortSize.x;
                offsets[2 * countOffset + 1] = 0;
                break;
            case Anchor::TOP:
                offsets[2 * countOffset] = 0;
                offsets[2 * countOffset + 1] = -iconHeight * 1.0 / viewPortSize.y;
                break;
            case Anchor::BOTTOM:
                offsets[2 * countOffset] = 0;
                offsets[2 * countOffset + 1] = iconHeight * 1.0 / viewPortSize.y;
                break;
            case Anchor::TOP_LEFT:
                offsets[2 * countOffset] = iconWidth * 1.0 / viewPortSize.x;
                offsets[2 * countOffset + 1] = -iconHeight * 1.0 / viewPortSize.y;
                break;
            case Anchor::TOP_RIGHT:
                offsets[2 * countOffset] = -iconWidth * 1.0 / viewPortSize.x;
                offsets[2 * countOffset + 1] = -iconHeight * 1.0 / viewPortSize.y;
                break;
            case Anchor::BOTTOM_LEFT:
                offsets[2 * countOffset] = iconWidth * 1.0 / viewPortSize.x;
                offsets[2 * countOffset + 1] = iconHeight * 1.0 / viewPortSize.y;
                break;
            case Anchor::BOTTOM_RIGHT:
                offsets[2 * countOffset] = -iconWidth * 1.0 / viewPortSize.x;
                offsets[2 * countOffset + 1] = iconHeight * 1.0 / viewPortSize.y;
                break;
            default:
                break;
        }

        iconBoundingBoxViewportAligned.x = (-iconWidth * 0.5) - scaledIconPadding + offsets[2 * countOffset] * viewPortSize.x * 0.5;
        iconBoundingBoxViewportAligned.y = (-iconHeight * 0.5) - scaledIconPadding - offsets[2 * countOffset + 1] * viewPortSize.y * 0.5;

        offsets[2 * countOffset] += (iconOffset.x * iconWidth) * 1.0 / viewPortSize.x;
        offsets[2 * countOffset + 1] += (iconOffset.y * iconHeight) * 1.0 / viewPortSize.y;
    } else {
        renderCoordinate = getRenderCoordinates(iconAnchor, -rotations[countOffset], iconWidth, iconHeight);
    }


    const double x = renderCoordinate.x + (is3d ? 0.0 : iconOffset.x * scaleFactor * iconSize);
    const double y = renderCoordinate.y + (is3d ? 0.0 : iconOffset.y * scaleFactor * iconSize);

    writePosition(x, y, countOffset, positions);

    if (is3d) {
        iconBoundingBoxViewportAligned.width = iconWidth;
        iconBoundingBoxViewportAligned.height = iconHeight;
    } else {
        iconBoundingBoxViewportAligned.x = (renderCoordinate.x - iconWidth * 0.5) - scaledIconPadding;
        iconBoundingBoxViewportAligned.y = (renderCoordinate.y - iconHeight * 0.5) - scaledIconPadding;
        iconBoundingBoxViewportAligned.width = iconWidth + 2.0 * scaledIconPadding;
        iconBoundingBoxViewportAligned.height = iconHeight + 2.0 * scaledIconPadding;
    }

    if (!isCoordinateOwner) {
        alphas[countOffset] = 0.0;
    } else if (!(description->minZoom <= zoomIdentifier && description->maxZoom >= zoomIdentifier)) {
        alphas[countOffset] = animationCoordinator->getIconAlpha(0.0, now);
    } else if (animationCoordinator->isColliding()) {
        alphas[countOffset] = animationCoordinator->getIconAlpha(0.0, now);
    } else {
        alphas[countOffset] = animationCoordinator->getIconAlpha(iconOpacity * alpha, now);
    }

    isIconOpaque = alphas[countOffset] == 0;

    countOffset += instanceCounts.icons;

    if (!animationCoordinator->isIconAnimating()) {
        lastIconUpdateScaleFactor = scaleFactor;
        lastIconUpdateRotation = rotation;
        lastIconUpdateAlpha = alpha;
    }
}

void Tiled2dMapVectorSymbolObject::writePosition(const double x_, const double y_, const size_t offset, std::vector<float> &buffer) {
    double x = is3d ? 1.0 * sin(y_) * cos(x_) - tileOrigin.x : x_ - tileOrigin.x;
    double y = is3d ?  1.0 * cos(y_) - tileOrigin.y : y_ - tileOrigin.y;
    double z = is3d ? -1.0 * sin(y_) * sin(x_) - tileOrigin.z : 0.0;

    buffer[positionSize * offset] = x;
    buffer[positionSize * offset + 1] = y;
    if (is3d) {
        buffer[positionSize * offset + 2] = z;
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

    const auto evalContext = EvaluationContext(zoomIdentifier, dpFactor, featureContext, featureStateManager);

    auto iconImage = description->style.getIconImage(evalContext);

    if (iconImage.empty() || !spriteTexture) {
        // TODO: make sure icon is not rendered
    } else {
        const auto textureWidth = (double) spriteTexture->getImageWidth();
        const auto textureHeight = (double) spriteTexture->getImageHeight();

        renderCoordinate = getRenderCoordinates(iconAnchor, 0.0, textureWidth, textureHeight);

        const auto spriteIt = spriteData->sprites.find(iconImage);
        if (spriteIt == spriteData->sprites.end()) {
            LogError << "Unable to find sprite " << iconImage;
            writePosition(0, 0, countOffset, positions);
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

    writePosition(renderCoordinate.x, renderCoordinate.y, countOffset, positions);

    countOffset += instanceCounts.stretchedIcons;

    lastStretchIconUpdateScaleFactor = -1;
}

void Tiled2dMapVectorSymbolObject::updateStretchIconProperties(std::vector<float> &positions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &alphas, std::vector<float> &stretchInfos, int &countOffset, const double zoomIdentifier, const double scaleFactor, const double rotation, long long now, const Vec2I viewPortSize) {

    if (instanceCounts.stretchedIcons == 0) {
        return;
    }
    if (!isCoordinateOwner) {
        if (!animationCoordinator->isOwned.test_and_set()) {
            isCoordinateOwner = true;

            lastIconUpdateScaleFactor = -1;
            lastStretchIconUpdateScaleFactor = -1;
            lastTextUpdateScaleFactor = -1;
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

    if (!isCoordinateOwner) {
        alphas[countOffset] = 0.0;
    } else if (animationCoordinator->isColliding() || !(description->minZoom <= zoomIdentifier && description->maxZoom >= zoomIdentifier) || !stretchSpriteInfo) {
        alphas[countOffset] = animationCoordinator->getStretchIconAlpha(0.0, now);
    } else {
        alphas[countOffset] = animationCoordinator->getStretchIconAlpha(iconOpacity * alpha, now);
    }

    if (!animationCoordinator->isStretchIconAnimating()) {
        lastStretchIconUpdateScaleFactor = zoomIdentifier;
        lastStretchIconUpdateRotation = rotation;
    }

    isStretchIconOpaque = alphas[countOffset] == 0;

    rotations[countOffset] = iconRotate;

    if (lastIconUpdateRotation != rotation && iconRotationAlignment != SymbolAlignment::MAP) {
        rotations[countOffset] += rotation;
    }

    const double densityOffset = (camera->getScreenDensityPpi() / 160.0) / stretchSpriteInfo->pixelRatio;

    auto spriteWidth = stretchSpriteInfo->width * densityOffset * scaleFactor;
    auto spriteHeight = stretchSpriteInfo->height * densityOffset * scaleFactor;

    const float topPadding = iconTextFitPadding[0] * stretchSpriteInfo->pixelRatio * densityOffset * scaleFactor;
    const float rightPadding = iconTextFitPadding[1] * stretchSpriteInfo->pixelRatio * densityOffset * scaleFactor;
    const float bottomPadding = iconTextFitPadding[2] * stretchSpriteInfo->pixelRatio * densityOffset * scaleFactor;
    const float leftPadding = iconTextFitPadding[3] * stretchSpriteInfo->pixelRatio * densityOffset * scaleFactor;

    auto scaleX = 1.0;
    auto scaleY = 1.0;
    if (labelObject) {
        const double textWidth = labelObject->dimensions.x + (leftPadding + rightPadding);
        const double textHeight = labelObject->dimensions.y + (topPadding + bottomPadding);
        scaleX = std::max(1.0, textWidth / (spriteWidth * iconSize));
        scaleY = std::max(1.0, textHeight / (spriteHeight * iconSize));
    }

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

    writePosition(renderCoordinate.x + offset.x, renderCoordinate.y + offset.y, countOffset, positions);

    const float scaledIconPadding = iconPadding * scaleFactor;

    Vec2D origin(renderCoordinate.x + offset.x, renderCoordinate.y + offset.y);
    stretchIconBoundingBoxViewportAligned.x = origin.x - spriteWidth * 0.5 - scaledIconPadding;
    stretchIconBoundingBoxViewportAligned.y = origin.y - spriteHeight * 0.5 - scaledIconPadding;
    stretchIconBoundingBoxViewportAligned.width = spriteWidth + 2.0 * scaledIconPadding;
    stretchIconBoundingBoxViewportAligned.height = spriteHeight + 2.0 * scaledIconPadding;

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

void Tiled2dMapVectorSymbolObject::updateTextProperties(std::vector<float> &positions, std::vector<float> &referencePositions, std::vector<float> &scales, std::vector<float> &rotations, std::vector<float> &styles, int &countOffset, uint16_t &styleOffset, const double zoomIdentifier, const double scaleFactor, const double rotation, long long now, const Vec2I viewPortSize) {
    if (instanceCounts.textCharacters ==  0 || !labelObject) {
        return;
    }

    if (!isCoordinateOwner) {
        if (!animationCoordinator->isOwned.test_and_set()) {
            isCoordinateOwner = true;

            lastIconUpdateScaleFactor = -1;
            lastStretchIconUpdateScaleFactor = -1;
            lastTextUpdateScaleFactor = -1;
        }
    }

    if (lastTextUpdateScaleFactor == scaleFactor && lastTextUpdateRotation == rotation && !isStyleZoomDependant) {
        styleOffset += instanceCounts.textCharacters == 0 ? 0 : 1;
        countOffset += instanceCounts.textCharacters;
        return;
    }

    {
        std::shared_ptr<::MapCameraInterface> camera = nullptr;

        if(is3d) {
            auto strongMapInterface = mapInterface.lock();
            camera = strongMapInterface->getCamera();
        }

        labelObject->updateProperties(positions, referencePositions, scales, rotations, styles, countOffset, styleOffset, zoomIdentifier, scaleFactor, animationCoordinator->isColliding(), rotation, alpha, isCoordinateOwner, now, viewPortSize, camera);
    }

    if (!animationCoordinator->isTextAnimating()) {
        lastTextUpdateScaleFactor = scaleFactor;
        lastTextUpdateRotation = rotation;
    }

    lastStretchIconUpdateScaleFactor = -1;
}

std::optional<CollisionRectD> Tiled2dMapVectorSymbolObject::getViewportAlignedBoundingBox(double zoomIdentifier, bool considerSymbolSpacing, bool considerOverlapFlag) {
    double minX = std::numeric_limits<double>::max(), maxX = std::numeric_limits<double>::lowest(), minY = std::numeric_limits<double>::max(), maxY = std::numeric_limits<double>::lowest();
    bool hasBox = false;

    double anchorX = renderCoordinate.x;
    double anchorY = renderCoordinate.y;

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
        const auto evalContext = EvaluationContext(zoomIdentifier, dpFactor, featureContext, featureStateManager);
        symbolSpacingPx = description->style.getSymbolSpacing(evalContext);
        contentHash = this->contentHash;
    }

    return CollisionRectD(anchorX, anchorY, minX, minY, maxX - minX, maxY - minY, contentHash, symbolSpacingPx);
}

std::optional<std::vector<CollisionCircleD>> Tiled2dMapVectorSymbolObject::getMapAlignedBoundingCircles(double zoomIdentifier, bool considerSymbolSpacing, bool considerOverlapFlag) {
    std::vector<CollisionCircleD> circles;

    double symbolSpacingPx = 0;
    size_t contentHash = 0;
    if (considerSymbolSpacing) {
        const auto evalContext = EvaluationContext(zoomIdentifier, dpFactor, featureContext, featureStateManager);
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
    if (labelObject && (labelObject->boundingBoxViewportAligned.has_value() || labelObject->boundingBoxCircles.has_value())) {
        return true;
    }
    if (iconBoundingBoxViewportAligned.x != 0) {
        return true;
    }
    if (stretchIconBoundingBoxViewportAligned.x != 0) {
        return true;
    }
    return false;
}

void Tiled2dMapVectorSymbolObject::setHideFromCollision(bool hide) {
    if(animationCoordinator->setColliding(hide)) {
        lastIconUpdateScaleFactor = -1;
        lastStretchIconUpdateScaleFactor = -1;
        lastTextUpdateScaleFactor = -1;
    }
}

void Tiled2dMapVectorSymbolObject::collisionDetection(const double zoomIdentifier, const double rotation, const double scaleFactor, std::shared_ptr<CollisionGrid> collisionGrid) {

    if (!isCoordinateOwner) {
        return;
    }

    if (!(description->minZoom <= zoomIdentifier && description->maxZoom >= zoomIdentifier) || !getIsOpaque() || !isPlaced()) {
        // not visible
        setHideFromCollision(true);
        return;
    }

    auto visibleIn3d = true;
//    {
//        auto strongMapInterface = mapInterface.lock();
//        auto camera3d = strongMapInterface ? strongMapInterface->getCamera()->asMapCamera3d() : nullptr;
//        if(camera3d != nullptr) {
//            if (auto cam = std::dynamic_pointer_cast<MapCamera3d>(camera3d)) {
//                visibleIn3d = !cam->coordIsFarAwayFromFocusPoint(coordinate);
//            }
//        }
//    }

    if(!visibleIn3d) {
        // not visible
        setHideFromCollision(true);
        return;
    }

    bool willCollide = true;
    bool outside = true;
    if (boundingBoxRotationAlignment == SymbolAlignment::VIEWPORT) {
        std::optional<CollisionRectD> boundingRect = getViewportAlignedBoundingBox(zoomIdentifier, false, true);
        // Collide, if no valid boundingRect
        if (boundingRect.has_value()) {
            auto check = collisionGrid->addAndCheckCollisionAlignedRect(*boundingRect);
            willCollide = check == 1;
            outside = check == 2;
        } else {
            willCollide = false;
            outside = false;
        }
    } else {
        std::optional<std::vector<CollisionCircleD>> boundingCircles = getMapAlignedBoundingCircles(zoomIdentifier, textSymbolPlacement != TextSymbolPlacement::POINT, true);
        // Collide, if no valid boundingCircles
        if (boundingCircles.has_value()) {
            auto check = collisionGrid->addAndCheckCollisionCircles(*boundingCircles);
            willCollide = check == 1;
            outside = check == 2;
        } else {
            willCollide = false;
            outside = false;
        }
    }

    if (!outside) {
        setHideFromCollision(willCollide);
    }

}

std::optional<std::tuple<Coord, VectorLayerFeatureInfo>> Tiled2dMapVectorSymbolObject::onClickConfirmed(const CircleD &clickHitCircle, double zoomIdentifier, CollisionUtil::CollisionEnvironment &collisionEnvironment) {
    if (animationCoordinator->isColliding()) {
        return std::nullopt;
    }

    if (boundingBoxRotationAlignment == SymbolAlignment::VIEWPORT) {
        
        std::optional<CollisionRectD> boundingRect = getViewportAlignedBoundingBox(zoomIdentifier, false, false);
        if (boundingRect) {
            auto projectedRectangle = CollisionUtil::getProjectedRectangle(*boundingRect, collisionEnvironment);
            if (projectedRectangle && CollisionUtil::checkRectCircleCollision(RectD(projectedRectangle->x, projectedRectangle->y, projectedRectangle->width, projectedRectangle->height), clickHitCircle)) {
                return std::make_tuple(coordinate, featureContext->getFeatureInfo());
            }
        }

    } else {
        if ((labelObject && labelObject->boundingBoxCircles.has_value() && CollisionUtil::checkCirclesCollision(*labelObject->boundingBoxCircles, clickHitCircle))
        || (iconBoundingBoxViewportAligned.width != 0 && CollisionUtil::checkRectCircleCollision(iconBoundingBoxViewportAligned, clickHitCircle))
        || (stretchIconBoundingBoxViewportAligned.width != 0 && CollisionUtil::checkRectCircleCollision(stretchIconBoundingBoxViewportAligned, clickHitCircle))) {
            return std::make_tuple(coordinate, featureContext->getFeatureInfo());
        }
    }

    return std::nullopt;
}

void Tiled2dMapVectorSymbolObject::setAlpha(float alpha) {
    this->alpha = alpha;
}

bool Tiled2dMapVectorSymbolObject::getIsOpaque() {
    return (!labelObject || labelObject->isOpaque) || isIconOpaque || isStretchIconOpaque;
}
