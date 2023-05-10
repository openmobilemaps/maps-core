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
            instanceCounts.strechedIcons = 1;
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
//    renderPos.y -= iconOffset.y;
//    renderPos.x += iconOffset.x;

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
    rotations[countOffset] = 0;

    auto iconSize = description->style.getIconSize(evalContext) * scaleFactor;
    scales[2 * countOffset] = spriteSize.x * iconSize;
    scales[2 * countOffset + 1] = spriteSize.y * iconSize;

    countOffset += instanceCounts.icons;
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
