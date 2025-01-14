/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorSymbolGroup.h"
#include "TextHelper.h"
#include "Vec2DHelper.h"
#include "AlphaInstancedShaderInterface.h"
#include "TextInstancedShaderInterface.h"
#include "Quad2dInstancedInterface.h"
#include "StretchInstancedShaderInterface.h"
#include "CoordinateSystemIdentifiers.h"
#include "Tiled2dMapVectorSourceSymbolDataManager.h"
#include "RenderObject.h"
#include "Tiled2dMapVectorAssetInfo.h"

Tiled2dMapVectorSymbolGroup::Tiled2dMapVectorSymbolGroup(uint32_t groupId,
                                                         const std::weak_ptr<MapInterface> &mapInterface,
                                                         const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                                         const WeakActor<Tiled2dMapVectorFontProvider> &fontProvider,
                                                         const Tiled2dMapVersionedTileInfo &tileInfo,
                                                         const std::string &layerIdentifier,
                                                         const std::shared_ptr<SymbolVectorLayerDescription> &layerDescription,
                                                         const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager,
                                                         const std::shared_ptr<Tiled2dMapVectorLayerSymbolDelegateInterface> &symbolDelegate,
                                                         const bool persistingSymbolPlacement)
: groupId(groupId),
mapInterface(mapInterface),
layerConfig(layerConfig),
tileInfo(tileInfo),
layerIdentifier(layerIdentifier),
layerDescription(layerDescription),
fontProvider(fontProvider),
featureStateManager(featureStateManager),
symbolDelegate(symbolDelegate),
usedKeys(layerDescription->getUsedKeys()),
persistingSymbolPlacement(persistingSymbolPlacement),
tileOrigin(0, 0, 0),
is3d(mapInterface.lock()->is3d()){
    if (auto strongMapInterface = mapInterface.lock()) {
        dpFactor = strongMapInterface->getCamera()->getScreenDensityPpi() / 160.0;
    }
    auto convertedTileBounds = mapInterface.lock()->getCoordinateConverterHelper()->convertRectToRenderSystem(tileInfo.tileInfo.bounds);
    double cx = (convertedTileBounds.bottomRight.x + convertedTileBounds.topLeft.x) / 2.0;
    double cy = (convertedTileBounds.bottomRight.y + convertedTileBounds.topLeft.y) / 2.0;
    double rx = is3d ? 1.0 * sin(cy) * cos(cx) : cx;
    double ry = is3d ? 1.0 * cos(cy) : cy;
    double rz = is3d ? -1.0 * sin(cy) * sin(cx) : 0.0;
    tileOrigin = Vec3D(rx, ry, rz);
}

void Tiled2dMapVectorSymbolGroup::initialize(std::weak_ptr<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>> weakFeatures,
                                             int32_t featuresBase,
                                             int32_t featuresCount,
                                             std::shared_ptr<SymbolAnimationCoordinatorMap> animationCoordinatorMap,
                                             const WeakActor<Tiled2dMapVectorSourceSymbolDataManager> &symbolManagerActor,
                                             float alpha) {
    auto selfActor = WeakActor<Tiled2dMapVectorSymbolGroup>(mailbox, shared_from_this());

    auto features = weakFeatures.lock();
    if (!features) {
        symbolManagerActor.message(MFN(&Tiled2dMapVectorSourceSymbolDataManager::onSymbolGroupInitialized), false, tileInfo, layerIdentifier, selfActor);
        return;
    }


    auto strongMapInterface = this->mapInterface.lock();
    auto camera = strongMapInterface ? strongMapInterface->getCamera() : nullptr;
    if (!strongMapInterface || !camera) {
        symbolManagerActor.message(MFN(&Tiled2dMapVectorSourceSymbolDataManager::onSymbolGroupInitialized), false, tileInfo, layerIdentifier, selfActor);
        return;
    }

    auto alphaInstancedShader = is3d ? strongMapInterface->getShaderFactory()->createUnitSphereAlphaInstancedShader()->asShaderProgramInterface() : strongMapInterface->getShaderFactory()->createAlphaInstancedShader()->asShaderProgramInterface();

    const double tilePixelFactor =
            (0.0254 / camera->getScreenDensityPpi()) * layerConfig->getZoomFactorAtIdentifier(tileInfo.tileInfo.zoomIdentifier - 1);

    std::unordered_map<std::string, std::vector<Coord>> textPositionMap;

    std::vector<VectorLayerFeatureInfo> featureInfosWithCustomAssets;

    size_t featureTileIndex = -1;
    int32_t featuresRBase = (int32_t)features->size() - (featuresBase + featuresCount);
    for (auto it = features->rbegin() + featuresRBase; it != features->rbegin() + featuresRBase + featuresCount; it++) {
        featureTileIndex++;
        auto const &[context, geometry] = *it;
        const auto evalContext = EvaluationContext(tileInfo.tileInfo.zoomIdentifier, dpFactor, context, featureStateManager);

        if ((layerDescription->filter != nullptr && !layerDescription->filter->evaluateOr(evalContext, false))) {
            continue;
        }

        if (!anyInteractable && layerDescription->isInteractable(evalContext)) {
            anyInteractable = true;
        }

        std::vector<FormattedStringEntry> text = layerDescription->style.getTextField(evalContext);

        if (layerDescription->style.getTextTransform(evalContext) == TextTransform::UPPERCASE) {
            for (auto &e: text) {
                e.text = TextHelper::uppercase(e.text);
            }
        }

        std::string fullText;
        for (const auto &textEntry: text) {
            fullText += textEntry.text;
        }

        auto anchor = layerDescription->style.getTextAnchor(evalContext);
        const auto &justify = layerDescription->style.getTextJustify(evalContext);
        const auto &placement = layerDescription->style.getTextSymbolPlacement(evalContext);

        const auto &variableTextAnchor = layerDescription->style.getTextVariableAnchor(evalContext);

        if (!variableTextAnchor.empty()) {
            // TODO: evaluate all anchors correctly and choose best one
            // for now the first one is simply used
            anchor = *variableTextAnchor.begin();
        }

        const auto &fontList = layerDescription->style.getTextFont(evalContext);

        const double symbolSpacingPx = layerDescription->style.getSymbolSpacing(evalContext);
        const double symbolSpacingMeters = symbolSpacingPx * tilePixelFactor;

        const auto iconOptional = layerDescription->style.getIconOptional(evalContext);
        const auto textOptional = layerDescription->style.getTextOptional(evalContext);
        const auto hasIcon = layerDescription->style.hasIconImagePotentially();

        const auto &pointCoordinates = geometry->getPointCoordinates();

        bool wasPlaced = false;

        const bool hasImageFromCustomProvider = layerDescription->style.getIconImageCustomProvider(evalContext);

        if (context->geomType != vtzero::GeomType::POINT) {
            double distance = 0;
            double totalDistance = 0;

            bool isLineCenter = placement == TextSymbolPlacement::LINE_CENTER;

            std::vector<Coord> line = {};
            for (const auto &points: pointCoordinates) {
                line.insert(line.end(), points.begin(), points.end());
            }

            for (const auto &points: pointCoordinates) {

                for (auto pointIt = points.begin(); pointIt != points.end(); pointIt++) {
                    auto point = *pointIt;

                    double interpolationValue = 0.0;
                    if (pointIt != points.begin() && !fullText.empty()) {
                        auto last = std::prev(pointIt);
                        double addDistance = Vec2DHelper::distance(Vec2D(last->x, last->y), Vec2D(point.x, point.y));
                        interpolationValue = (symbolSpacingMeters - distance) / addDistance;
                        distance += addDistance;
                        totalDistance += addDistance;
                    }

                    auto pos = getPositioning(pointIt, points, interpolationValue);

                    if (!isLineCenter && distance > symbolSpacingMeters && pos) {

                        auto position = pos->centerPosition;

                        const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription, layerConfig,
                                                                     context, text, fullText, position, line, fontList, anchor,
                                                                     pos->angle, justify, placement, false, animationCoordinatorMap, featureTileIndex, hasImageFromCustomProvider);
                        if (symbolObject) {
                            symbolObjects.push_back(symbolObject);
                            textPositionMap[fullText].push_back(position);
                            wasPlaced = true;
                        }

                        if (hasIcon) {
                            if (textOptional) {
                                const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription,
                                                                             layerConfig, context, {}, "", position, line, fontList,
                                                                             anchor, pos->angle, justify, placement, false, animationCoordinatorMap, featureTileIndex, hasImageFromCustomProvider);

                                if (symbolObject) {
                                    symbolObjects.push_back(symbolObject);
                                    textPositionMap[fullText].push_back(position);
                                    wasPlaced = true;
                                }
                            }
                            if (iconOptional) {
                                const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription,
                                                                             layerConfig, context, text, fullText, position, line,
                                                                             fontList, anchor, pos->angle, justify, placement,
                                                                             true, animationCoordinatorMap, featureTileIndex, hasImageFromCustomProvider);

                                if (symbolObject) {
                                    symbolObjects.push_back(symbolObject);
                                    textPositionMap[fullText].push_back(position);
                                    wasPlaced = true;
                                }
                            }
                        }

                        distance = 0;
                    }
                }
            }

            // if no label was placed, place it in the middle of the line
            if (isLineCenter || !wasPlaced) {
                distance = 0;
                double targetDistancePosition = (totalDistance / 2.0);
                for (const auto &points: pointCoordinates) {
                    for (auto pointIt = points.begin() + 1; pointIt != points.end(); pointIt++) {
                        auto point = *pointIt;

                        double interpolationValue;
                        auto last = std::prev(pointIt);
                        double addDistance = Vec2DHelper::distance(Vec2D(last->x, last->y), Vec2D(point.x, point.y));
                        interpolationValue = (targetDistancePosition - distance) / addDistance;
                        distance += addDistance;

                        auto pos = getPositioning(pointIt, points, interpolationValue);

                        std::optional<double> closestOther;
                        auto existingPositionsIt = textPositionMap.find(fullText);
                        if (existingPositionsIt != textPositionMap.end() && !fullText.empty()) {
                            for (const auto existingPosition: existingPositionsIt->second) {
                                closestOther = std::min(Vec2DHelper::distance(Vec2D(pos->centerPosition.x, pos->centerPosition.y),
                                                                              Vec2D(existingPosition.x, existingPosition.y)),
                                                        closestOther.value_or(std::numeric_limits<double>::max()));
                                if (closestOther <= symbolSpacingMeters) {
                                    continue;
                                }
                            }
                        }


                        if (distance >= targetDistancePosition && pos &&
                            (!closestOther || *closestOther > symbolSpacingMeters)) {

                            auto position = pos->centerPosition;

                            const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription, layerConfig,
                                                                         context, text, fullText, position, line, fontList, anchor,
                                                                         pos->angle, justify, placement, false, animationCoordinatorMap, featureTileIndex, hasImageFromCustomProvider);
                            if (symbolObject) {
                                symbolObjects.push_back(symbolObject);
                                textPositionMap[fullText].push_back(position);
                                wasPlaced = true;
                            }

                            if (hasIcon) {
                                if (textOptional) {
                                    const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription,
                                                                                 layerConfig, context, {}, "", position, line,
                                                                                 fontList, anchor, pos->angle, justify, placement,
                                                                                 false, animationCoordinatorMap, featureTileIndex, hasImageFromCustomProvider);
                                    if (symbolObject) {
                                        symbolObjects.push_back(symbolObject);
                                        textPositionMap[fullText].push_back(position);
                                        wasPlaced = true;
                                    }
                                }
                                if (iconOptional) {
                                    const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription,
                                                                                 layerConfig, context, text, fullText, position,
                                                                                 line, fontList, anchor, pos->angle, justify,
                                                                                 placement, true, animationCoordinatorMap, featureTileIndex, hasImageFromCustomProvider);
                                    if (symbolObject) {
                                        symbolObjects.push_back(symbolObject);
                                        textPositionMap[fullText].push_back(position);
                                        wasPlaced = true;
                                    }
                                }
                            }

                            distance = 0;
                        }
                    }
                }
            }
        } else {
            for (const auto &p: pointCoordinates) {
                for (const auto &mp: p) {
                    std::optional<double> angle = std::nullopt;

                    const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription, layerConfig, context,
                                                                 text, fullText, mp, std::nullopt, fontList, anchor, angle, justify,
                                                                 placement, false, animationCoordinatorMap, featureTileIndex, hasImageFromCustomProvider);

                    if (symbolObject) {
                        symbolObjects.push_back(symbolObject);
                        wasPlaced = true;
                    }

                    if (hasIcon) {
                        if (textOptional) {
                            const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription, layerConfig,
                                                                         context, {}, "", mp, std::nullopt, fontList, anchor, angle,
                                                                         justify, placement, false, animationCoordinatorMap, featureTileIndex, hasImageFromCustomProvider);

                            if (symbolObject) {
                                symbolObjects.push_back(symbolObject);
                                wasPlaced = true;
                            }
                        }
                        if (iconOptional) {
                            const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription, layerConfig,
                                                                         context, text, fullText, mp, std::nullopt, fontList, anchor,
                                                                         angle, justify, placement, true, animationCoordinatorMap, featureTileIndex, hasImageFromCustomProvider);

                            if (symbolObject) {
                                symbolObjects.push_back(symbolObject);
                                wasPlaced = true;
                            }
                        }
                    }
                }
            }
        }
        if (wasPlaced) {
            // load custom icon if flag is set
            if (hasImageFromCustomProvider && symbolDelegate) {
                featureInfosWithCustomAssets.push_back(context->getFeatureInfo());
            }
        }
    }

    if (symbolObjects.empty()) {
        symbolManagerActor.message(MFN(&Tiled2dMapVectorSourceSymbolDataManager::onSymbolGroupInitialized), false, tileInfo, layerIdentifier, selfActor);
        return;
    }

    if (symbolDelegate && !featureInfosWithCustomAssets.empty()) {
        auto customAssetPages = symbolDelegate->getCustomAssetsFor(featureInfosWithCustomAssets, layerDescription->identifier);
        for (const auto &page: customAssetPages) {
            auto object = strongMapInterface->getGraphicsObjectFactory()->createQuadInstanced(alphaInstancedShader);
            object->setInstanceCount((int32_t) page.featureIdentifiersUv.size());
            customTextures.push_back(CustomIconDescriptor(page.texture, object, page.featureIdentifiersUv, is3d));
        }
    }

    int positionSize = is3d ? 3 : 2;

    //Tiled2dMapVectorSymbolObject::SymbolObjectInstanceCounts instanceCounts{0, 0, 0};
    int32_t iconCount = 0;
    int32_t stretchedIconCount = 0;
    std::unordered_map<std::shared_ptr<FontLoaderResult>, std::tuple<int32_t, int32_t>> fontStylesAndCharactersCountMap;

    for (auto const object: symbolObjects) {
        const auto &counts = object->getInstanceCounts();
        if (!object->hasCustomTexture) {
            iconCount += counts.icons;
        }
        if (counts.textCharacters > 0) {
            auto font = object->getFont();
            int32_t stylesCount = 0;
            int32_t charactersCount = 0;
            auto currentEntry = fontStylesAndCharactersCountMap.find(font);
            if (currentEntry != fontStylesAndCharactersCountMap.end()) {
                stylesCount = std::get<0>(currentEntry->second);
                charactersCount = std::get<1>(currentEntry->second);
            }
            fontStylesAndCharactersCountMap[font] = {stylesCount + 1, charactersCount + counts.textCharacters};
        }
        stretchedIconCount += counts.stretchedIcons;
    }

    if (iconCount != 0) {
        alphaInstancedShader->setBlendMode(
                layerDescription->style.getBlendMode(EvaluationContext(0.0, dpFactor, std::make_shared<FeatureContext>(), featureStateManager)));
        iconInstancedObject = strongMapInterface->getGraphicsObjectFactory()->createQuadInstanced(alphaInstancedShader);
#if DEBUG
        iconInstancedObject->asGraphicsObject()->setDebugLabel(layerDescription->identifier + "_" + tileInfo.tileInfo.to_string_short());
#endif

        iconInstancedObject->setInstanceCount(iconCount);

        iconAlphas.resize(iconCount, 0.0);
        iconRotations.resize(iconCount, 0.0);
        iconScales.resize(iconCount * 2, 0.0);
        iconPositions.resize(iconCount * positionSize, 0.0);
        iconTextureCoordinates.resize(iconCount * 4, 0.0);
        iconOffsets.resize(iconCount * 2, 0.0);
    }


    if (stretchedIconCount != 0) {
        auto shader = strongMapInterface->getShaderFactory()->createStretchInstancedShader(is3d)->asShaderProgramInterface();
        shader->setBlendMode(
                layerDescription->style.getBlendMode(EvaluationContext(0.0, dpFactor, std::make_shared<FeatureContext>(), featureStateManager)));
        stretchedInstancedObject = strongMapInterface->getGraphicsObjectFactory()->createQuadStretchedInstanced(shader);
#if DEBUG
        stretchedInstancedObject->asGraphicsObject()->setDebugLabel(layerDescription->identifier + "_" + tileInfo.tileInfo.to_string_short());
#endif

        stretchedInstancedObject->setInstanceCount(stretchedIconCount);

        stretchedIconAlphas.resize(stretchedIconCount, 0.0);
        stretchedIconRotations.resize(stretchedIconCount, 0.0);
        stretchedIconScales.resize(stretchedIconCount * 2, 0.0);
        stretchedIconPositions.resize(stretchedIconCount * positionSize, 0.0);
        stretchedIconStretchInfos.resize(stretchedIconCount * 10, 1.0);
        stretchedIconTextureCoordinates.resize(stretchedIconCount * 4, 0.0);
    }

    if (!fontStylesAndCharactersCountMap.empty()) {
        for (auto const &[fontResult, styleTextCharactersCounts]: fontStylesAndCharactersCountMap) {
            auto shader =  is3d ? strongMapInterface->getShaderFactory()->createUnitSphereTextInstancedShader()->asShaderProgramInterface() : strongMapInterface->getShaderFactory()->createTextInstancedShader()->asShaderProgramInterface();
            shader->setBlendMode(
                    layerDescription->style.getBlendMode(EvaluationContext(0.0, dpFactor, std::make_shared<FeatureContext>(), featureStateManager)));
            auto textInstancedObject = strongMapInterface->getGraphicsObjectFactory()->createTextInstanced(shader);
#if DEBUG
            textInstancedObject->asGraphicsObject()->setDebugLabel(layerDescription->identifier + "_" + tileInfo.tileInfo.to_string_short());
#endif
            auto textStyleCount = std::get<0>(styleTextCharactersCounts);
            auto textCharactersCount = std::get<1>(styleTextCharactersCounts);

            textInstancedObject->setInstanceCount(textCharactersCount);

            textInstancedObjects.push_back(textInstancedObject);
            textDescriptors.emplace_back(std::make_shared<TextDescriptor>(textStyleCount, textCharactersCount, fontResult, is3d));
        }
    }

#ifdef DRAW_TEXT_BOUNDING_BOX
    textSymbolPlacement = layerDescription->style.getTextSymbolPlacement(EvaluationContext(0.0, dpFactor, std::make_shared<FeatureContext>(), featureStateManager));
    labelRotationAlignment = layerDescription->style.getTextRotationAlignment(EvaluationContext(0.0, dpFactor, std::make_shared<FeatureContext>(), featureStateManager));
    if (labelRotationAlignment == SymbolAlignment::AUTO) {
        switch (textSymbolPlacement) {
            case TextSymbolPlacement::POINT:
                labelRotationAlignment = SymbolAlignment::VIEWPORT;
                break;
            case TextSymbolPlacement::LINE:
            case TextSymbolPlacement::LINE_CENTER:
                labelRotationAlignment = SymbolAlignment::MAP;
                break;
        }
    }
    if (iconCount + stretchedIconCount > 0 || !fontStylesAndCharactersCountMap.empty()) {
        auto shader = strongMapInterface->getShaderFactory()->createPolygonGroupShader(false, is3d);
        auto object = strongMapInterface->getGraphicsObjectFactory()->createPolygonGroup(shader->asShaderProgramInterface());
        boundingBoxLayerObject = std::make_shared<PolygonGroup2dLayerObject>(strongMapInterface->getCoordinateConverterHelper(),
                                                                             object, shader);
        boundingBoxLayerObject->setStyles({
                                                  PolygonStyle(Color(0, 1, 0, 0.3), 0.3),
                                                  PolygonStyle(Color(1, 0, 0, 0.3), 1.0),
                                                  PolygonStyle(Color(0, 0, 1, 0.3), 1.0)
                                          });
    }
#endif

    setAlpha(alpha);

    symbolManagerActor.message(MFN(&Tiled2dMapVectorSourceSymbolDataManager::onSymbolGroupInitialized), true, tileInfo, layerIdentifier, selfActor);
}

void Tiled2dMapVectorSymbolGroup::updateLayerDescription(const std::shared_ptr<SymbolVectorLayerDescription> layerDescription) {
    this->layerDescription = layerDescription;
    usedKeys = layerDescription->getUsedKeys();
    for (auto const &object: symbolObjects) {
        object->updateLayerDescription(layerDescription, usedKeys);
    }
    if (spriteData && spriteTexture) {
        setupObjects(spriteData, spriteTexture);
    }
}

void Tiled2dMapVectorSymbolGroup::setupObjects(const std::shared_ptr<SpriteData> &spriteData,
                                               const std::shared_ptr<TextureHolderInterface> &spriteTexture,
                                               const std::optional<WeakActor<Tiled2dMapVectorSourceSymbolDataManager>> &symbolDataManager) {
    const auto context = mapInterface.lock()->getRenderingContext();

    int iconOffset = 0;
    int stretchedIconOffset = 0;
    std::unordered_map<std::string, int32_t> textOffsets;
    std::unordered_map<std::string, uint16_t> textStyleOffsets;

    for (auto const &object: symbolObjects) {

        if (!object->hasCustomTexture && spriteTexture && spriteData) {
            object->setupIconProperties(iconPositions, iconRotations, iconTextureCoordinates, iconOffset, tileInfo.tileInfo.zoomIdentifier,
                                        spriteTexture, spriteData, std::nullopt);
        } else {
            for (size_t i = 0; i != customTextures.size(); i++) {
                auto identifier = object->stringIdentifier;
                auto uvIt = customTextures[i].featureIdentifiersUv.find(identifier);
                if (uvIt != customTextures[i].featureIdentifiersUv.end()) {
                    object->customTexturePage = i;
                    object->customTextureOffset = (int)std::distance(customTextures[i].featureIdentifiersUv.begin(),uvIt);

                    int offset = object->customTextureOffset;
                    assert(offset < customTextures[i].iconRotations.size());
                    object->setupIconProperties(customTextures[i].iconPositions, customTextures[i].iconRotations, customTextures[i].iconTextureCoordinates, offset, tileInfo.tileInfo.zoomIdentifier, customTextures[i].texture, nullptr, uvIt->second);
                    break;
                }
            }
        }
        object->setupStretchIconProperties(stretchedIconPositions, stretchedIconTextureCoordinates, stretchedIconOffset,
                                           tileInfo.tileInfo.zoomIdentifier, spriteTexture, spriteData);
        auto font = object->getFont();
        if (font) {
            const auto &textDescriptor = std::find_if(textDescriptors.begin(), textDescriptors.end(),
                                                      [&font](const auto &textDescriptor) {
                                                          return font->fontData->info.name ==
                                                                 textDescriptor->fontResult->fontData->info.name;
                                                      });
            if (textDescriptor != textDescriptors.end()) {
                int32_t currentTextOffset = textOffsets[font->fontData->info.name];
                uint16_t currentStyleOffset = textStyleOffsets[font->fontData->info.name];
                object->setupTextProperties((*textDescriptor)->textTextureCoordinates,
                                            (*textDescriptor)->textStyleIndices,
                                            currentTextOffset, currentStyleOffset,
                                            tileInfo.tileInfo.zoomIdentifier);
                textOffsets[font->fontData->info.name] = currentTextOffset;
                textStyleOffsets[font->fontData->info.name] = currentStyleOffset;
            }
        }
    }

    for (const auto &customDescriptor: customTextures) {
        customDescriptor.renderObject->setFrame(Quad2dD(Vec2D(-0.5, 0.5), Vec2D(0.5, 0.5), Vec2D(0.5, -0.5), Vec2D(-0.5, -0.5)), tileOrigin, is3d);
        customDescriptor.renderObject->loadTexture(context, customDescriptor.texture);
        customDescriptor.renderObject->asGraphicsObject()->setup(context);

        int32_t count = (int32_t)customDescriptor.featureIdentifiersUv.size();
        customDescriptor.renderObject->setPositions(SharedBytes((int64_t) customDescriptor.iconPositions.data(), count, 3 * (int32_t) sizeof(float)));
        customDescriptor.renderObject->setTextureCoordinates(SharedBytes((int64_t) customDescriptor.iconTextureCoordinates.data(), count, 4 * (int32_t) sizeof(float)));
    }

    if (spriteTexture && spriteData && iconInstancedObject) {
        if (this->spriteData == nullptr) {
            iconInstancedObject->setFrame(Quad2dD(Vec2D(-0.5, 0.5), Vec2D(0.5, 0.5), Vec2D(0.5, -0.5), Vec2D(-0.5, -0.5)), tileOrigin, is3d);
            iconInstancedObject->loadTexture(context, spriteTexture);
            iconInstancedObject->asGraphicsObject()->setup(context);
        }
        iconInstancedObject->setPositions(
                SharedBytes((int64_t) iconPositions.data(), (int32_t) iconAlphas.size(), 3 * (int32_t) sizeof(float)));
        iconInstancedObject->setTextureCoordinates(
                SharedBytes((int64_t) iconTextureCoordinates.data(), (int32_t) iconAlphas.size(), 4 * (int32_t) sizeof(float)));
    }

    if (spriteTexture && spriteData && stretchedInstancedObject) {
        if (this->spriteData == nullptr) {
            stretchedInstancedObject->setFrame(Quad2dD(Vec2D(-0.5, 0.5), Vec2D(0.5, 0.5), Vec2D(0.5, -0.5), Vec2D(-0.5, -0.5)), tileOrigin, is3d);
            stretchedInstancedObject->loadTexture(context, spriteTexture);
            stretchedInstancedObject->asGraphicsObject()->setup(context);
        }
        stretchedInstancedObject->setPositions(
                SharedBytes((int64_t) stretchedIconPositions.data(), (int32_t) stretchedIconAlphas.size(),
                            3 * (int32_t) sizeof(float)));
        stretchedInstancedObject->setTextureCoordinates(
                SharedBytes((int64_t) stretchedIconTextureCoordinates.data(), (int32_t) stretchedIconAlphas.size(),
                            4 * (int32_t) sizeof(float)));
    }

    for (size_t i = 0; i < textDescriptors.size(); i++) {
        const auto &textDescriptor = textDescriptors[i];
        const auto &textInstancedObject = textInstancedObjects[i];
        if (this->spriteData == nullptr) {
            textInstancedObject->setFrame(Quad2dD(Vec2D(-0.5, 0.5), Vec2D(0.5, 0.5), Vec2D(0.5, -0.5), Vec2D(-0.5, -0.5)), tileOrigin, is3d);
            textInstancedObject->loadTexture(context, textDescriptor->fontResult->imageData);
            textInstancedObject->asGraphicsObject()->setup(context);
        }
        textInstancedObject->setTextureCoordinates(
                SharedBytes((int64_t) textDescriptor->textTextureCoordinates.data(), (int32_t) textDescriptor->textRotations.size(), 4 * (int32_t) sizeof(float)));
        textInstancedObject->setStyleIndices(
                SharedBytes((int64_t) textDescriptor->textStyleIndices.data(), (int32_t) textDescriptor->textStyleIndices.size(), 1 * (int32_t) sizeof(uint16_t)));
    }

    this->spriteData = spriteData;
    this->spriteTexture = spriteTexture;

    if (symbolDataManager.has_value()) {
        auto selfActor = WeakActor<Tiled2dMapVectorSymbolGroup>(mailbox, shared_from_this());
        symbolDataManager->message(MFN(&Tiled2dMapVectorSourceSymbolDataManager::onSymbolGroupInitialized), true, tileInfo,
                                   layerIdentifier, selfActor);
    }

    isInitialized = true;
}

void Tiled2dMapVectorSymbolGroup::update(const double zoomIdentifier, const double rotation, const double scaleFactor, long long now, const Vec2I viewPortSize, const std::vector<float>& vpMatrix, const Vec3D& origin) {
    if (!isInitialized) {
        return;
    }

    int positionSize = is3d ? 3 : 2;
    // icons
    if (!symbolObjects.empty()) {

        int iconOffset = 0;
        int stretchedIconOffset = 0;
        std::unordered_map<std::string, int32_t> textOffsets;
        std::unordered_map<std::string, uint16_t> textStyleOffsets;

        for (auto const &object: symbolObjects) {
            if (object->hasCustomTexture) {
                auto &page = customTextures[object->customTexturePage];
                int offset = (int)object->customTextureOffset;
                object->updateIconProperties(page.iconPositions, page.iconScales, page.iconRotations, page.iconAlphas, page.iconOffsets, page.iconTextureCoordinates, offset, zoomIdentifier,scaleFactor, rotation, now, viewPortSize, spriteTexture, spriteData);

            } else {
                object->updateIconProperties(iconPositions, iconScales, iconRotations, iconAlphas, iconOffsets, iconTextureCoordinates, iconOffset, zoomIdentifier,
                                             scaleFactor, rotation, now, viewPortSize, spriteTexture, spriteData);
            }
            object->updateStretchIconProperties(stretchedIconPositions, stretchedIconScales, stretchedIconRotations,
                                                stretchedIconAlphas, stretchedIconStretchInfos, stretchedIconTextureCoordinates, stretchedIconOffset, zoomIdentifier,
                                                scaleFactor, rotation, now, viewPortSize, spriteTexture, spriteData);
            auto font = object->getFont();
            if (font) {
                const auto &textDescriptor = std::find_if(textDescriptors.begin(), textDescriptors.end(),
                                                          [&font](const auto &textDescriptor) {
                                                              return font->fontData->info.name ==
                                                                     textDescriptor->fontResult->fontData->info.name;
                                                          });
                if (textDescriptor != textDescriptors.end()) {
                    int32_t currentTextOffset = textOffsets[font->fontData->info.name];
                    uint16_t currentStyleOffset = textStyleOffsets[font->fontData->info.name];
                    object->updateTextProperties((*textDescriptor)->textPositions, (*textDescriptor)->textReferencePositions,
                                                 (*textDescriptor)->textScales, (*textDescriptor)->textRotations,
                                                 (*textDescriptor)->textStyles,
                                                 currentTextOffset, currentStyleOffset,
                                                 zoomIdentifier, scaleFactor, rotation, now, viewPortSize, vpMatrix, origin);
                    textOffsets[font->fontData->info.name] = currentTextOffset;
                    textStyleOffsets[font->fontData->info.name] = currentStyleOffset;
                }
            }
        }

        for (auto &customDescriptor: customTextures) {
            int32_t count = (int32_t)customDescriptor.featureIdentifiersUv.size();

            if (customDescriptor.iconPositions.wasModified()) {
                customDescriptor.renderObject->setPositions(
                        SharedBytes((int64_t) customDescriptor.iconPositions.data(), (int32_t) count, positionSize * (int32_t) sizeof(float)));
                customDescriptor.iconPositions.resetModificationFlag();
            }

            if (customDescriptor.iconAlphas.wasModified()) {
                customDescriptor.renderObject->setAlphas(
                                                         SharedBytes((int64_t) customDescriptor.iconAlphas.data(), (int32_t) count, (int32_t) sizeof(float)));
                customDescriptor.iconAlphas.resetModificationFlag();
            }

            if (customDescriptor.iconScales.wasModified()) {
                customDescriptor.renderObject->setScales(
                                                         SharedBytes((int64_t) customDescriptor.iconScales.data(), (int32_t) count, 2 * (int32_t) sizeof(float)));
                customDescriptor.iconScales.resetModificationFlag();
            }

            if (customDescriptor.iconRotations.wasModified()) {
                customDescriptor.renderObject->setRotations(
                                                            SharedBytes((int64_t) customDescriptor.iconRotations.data(), (int32_t) count, 1 * (int32_t) sizeof(float)));
                customDescriptor.iconRotations.resetModificationFlag();
            }

            if (customDescriptor.iconOffsets.wasModified()) {
                customDescriptor.renderObject->setPositionOffset(
                                                                 SharedBytes((int64_t) customDescriptor.iconOffsets.data(), (int32_t) count, 2 * (int32_t) sizeof(float)));
                customDescriptor.iconOffsets.resetModificationFlag();
            }

            if (customDescriptor.iconTextureCoordinates.wasModified()) {
                customDescriptor.renderObject->setTextureCoordinates(
                                                                 SharedBytes((int64_t) customDescriptor.iconTextureCoordinates.data(), (int32_t) count, 4 * (int32_t) sizeof(float)));
                customDescriptor.iconTextureCoordinates.resetModificationFlag();
            }
        }

        if (iconInstancedObject) {
            int32_t iconCount = (int32_t) iconAlphas.size();
            if (iconPositions.wasModified()) {
                iconInstancedObject->setPositions(
                                                  SharedBytes((int64_t) iconPositions.data(), iconCount, positionSize * (int32_t) sizeof(float)));
                iconPositions.resetModificationFlag();
            }

            if (iconAlphas.wasModified()) {
                iconInstancedObject->setAlphas(
                                               SharedBytes((int64_t) iconAlphas.data(), iconCount, (int32_t) sizeof(float)));
                iconAlphas.resetModificationFlag();
            }

            if (iconScales.wasModified()) {
                iconInstancedObject->setScales(
                                               SharedBytes((int64_t) iconScales.data(), iconCount, 2 * (int32_t) sizeof(float)));
                iconScales.resetModificationFlag();
            }

            if (iconRotations.wasModified()) {
                iconInstancedObject->setRotations(
                                                  SharedBytes((int64_t) iconRotations.data(), iconCount, 1 * (int32_t) sizeof(float)));
                iconRotations.resetModificationFlag();
            }

            if (iconOffsets.wasModified()) {
                iconInstancedObject->setPositionOffset(
                                                       SharedBytes((int64_t) iconOffsets.data(), iconCount, 2 * (int32_t) sizeof(float)));
                iconOffsets.resetModificationFlag();
            }

            if (iconTextureCoordinates.wasModified()) {
                iconInstancedObject->setTextureCoordinates(
                                                       SharedBytes((int64_t) iconTextureCoordinates.data(), iconCount, 4 * (int32_t) sizeof(float)));
                iconTextureCoordinates.resetModificationFlag();
            }
        }

        if (stretchedInstancedObject) {
            int32_t iconCount = (int32_t) stretchedIconAlphas.size();
            if (stretchedIconPositions.wasModified()) {
                stretchedInstancedObject->setPositions(SharedBytes((int64_t) stretchedIconPositions.data(), iconCount,
                                                                   positionSize * (int32_t) sizeof(float)));
                stretchedIconPositions.resetModificationFlag();
            }

            if (stretchedIconAlphas.wasModified()) {
                stretchedInstancedObject->setAlphas(SharedBytes((int64_t) stretchedIconAlphas.data(), iconCount,
                                                                (int32_t) sizeof(float)));
                stretchedIconAlphas.resetModificationFlag();
            }

            if (stretchedIconScales.wasModified()) {
                stretchedInstancedObject->setScales(SharedBytes((int64_t) stretchedIconScales.data(), iconCount,
                                                                2 * (int32_t) sizeof(float)));
                stretchedIconScales.resetModificationFlag();
            }

            if (stretchedIconRotations.wasModified()) {
                stretchedInstancedObject->setRotations(SharedBytes((int64_t) stretchedIconRotations.data(), iconCount,
                                                                   1 * (int32_t) sizeof(float)));
                stretchedIconRotations.resetModificationFlag();
            }

            if (stretchedIconStretchInfos.wasModified()) {
                stretchedInstancedObject->setStretchInfos(SharedBytes((int64_t) stretchedIconStretchInfos.data(), iconCount,
                                                                      10 * (int32_t) sizeof(float)));
                stretchedIconStretchInfos.resetModificationFlag();
            }

            if (stretchedIconTextureCoordinates.wasModified()) {
                stretchedInstancedObject->setStretchInfos(SharedBytes((int64_t) stretchedIconTextureCoordinates.data(), iconCount,
                                                                      4 * (int32_t) sizeof(float)));
                stretchedIconTextureCoordinates.resetModificationFlag();
            }
        }

        for (size_t i = 0; i < textDescriptors.size(); i++) {
            const auto &textDescriptor = textDescriptors[i];
            const auto &textInstancedObject = textInstancedObjects[i];
            if (textDescriptor->textPositions.wasModified()) {
                textInstancedObject->setPositions(
                                                  SharedBytes((int64_t) textDescriptor->textPositions.data(), (int32_t) textDescriptor->textRotations.size(), 2 * (int32_t) sizeof(float)));
                textDescriptor->textPositions.resetModificationFlag();
            }
            if (is3d && textDescriptor->textReferencePositions.wasModified()) {
                textInstancedObject->setReferencePositions(
                        SharedBytes((int64_t) textDescriptor->textReferencePositions.data(), (int32_t) textDescriptor->textRotations.size(), positionSize * (int32_t) sizeof(float)));
                textDescriptor->textReferencePositions.resetModificationFlag();
            }

            if (textDescriptor->textStyles.wasModified()) {
                textInstancedObject->setStyles(
                                               SharedBytes((int64_t) textDescriptor->textStyles.data(), (int32_t) textDescriptor->textStyles.size() / 10, 10 * (int32_t) sizeof(float)));
                textDescriptor->textStyles.resetModificationFlag();
            }

            if (textDescriptor->textScales.wasModified()) {
                textInstancedObject->setScales(
                                               SharedBytes((int64_t) textDescriptor->textScales.data(), (int32_t) textDescriptor->textRotations.size(), 2 * (int32_t) sizeof(float)));
                textDescriptor->textScales.resetModificationFlag();
            }

            if (textDescriptor->textRotations.wasModified()) {
                textInstancedObject->setRotations(
                                                  SharedBytes((int64_t) textDescriptor->textRotations.data(), (int32_t) textDescriptor->textRotations.size(), 1 * (int32_t) sizeof(float)));
                textDescriptor->textRotations.resetModificationFlag();
            }

        }

#ifdef DRAW_TEXT_BOUNDING_BOX

        if (boundingBoxLayerObject) {
            std::vector<std::tuple<std::vector<::Coord>, int>> vertices;
            std::vector<uint16_t> indices;


            auto camera = mapInterface.lock()->getCamera();
            auto renderingContext = mapInterface.lock()->getRenderingContext();

            auto viewport = renderingContext->getViewportSize();
            auto angle = camera->getRotation();
            auto origin = camera->asCameraInterface()->getOrigin();
            auto vpMatrix = *camera->getLastVpMatrixD();

            int32_t currentVertexIndex = 0;

            double halfWidth = viewport.x / 2.0f;
            double halfHeight = viewport.y / 2.0f;
            double sinNegGridAngle(std::sin(-angle * M_PI / 180.0));
            double cosNegGridAngle(std::cos(-angle * M_PI / 180.0));
            Vec4D temp1 = {0,0,0,0}, temp2 = {0,0,0,0};
            CollisionUtil::CollisionEnvironment env(vpMatrix, is3d, temp1, temp2, halfWidth, halfHeight, sinNegGridAngle, cosNegGridAngle, origin);


            if (lastClickHitCircle) {
                double x = ((lastClickHitCircle->x / viewport.x) * 2.0) - 1.0;
                double y = ((lastClickHitCircle->y / viewport.y) * 2.0) - 1.0;
                float aspectRatio = float(viewport.x) / float(viewport.y);
                double radius = lastClickHitCircle->radius / halfWidth;

                const size_t numCirclePoints = 8;
                std::vector<Coord> coords;
                coords.emplace_back(CoordinateSystemIdentifiers::RENDERSYSTEM(),
                                    x, y, 0.0);
                for (size_t i = 0; i < numCirclePoints; i++) {
                    float angle = i * (2 * M_PI / numCirclePoints);
                    coords.emplace_back(CoordinateSystemIdentifiers::RENDERSYSTEM(),
                                        x + radius * std::cos(angle),
                                        y + radius * aspectRatio * std::sin(angle),
                                        0.0);

                    indices.push_back(currentVertexIndex);
                    indices.push_back(currentVertexIndex + i + 1);
                    indices.push_back(currentVertexIndex + (i + 1) % numCirclePoints + 1);
                }
                vertices.push_back({coords, 2});

                currentVertexIndex += (numCirclePoints + 1);
            }

            for (const auto &object: symbolObjects) {
                if (currentVertexIndex >= std::numeric_limits<uint16_t>::max()) {
                    LogError <<= "Too many debug collision primitives for uint16 vertex indices!";
                    break;
                }

                if (!object->getIsOpaque()) continue;

                if (!object->isCoordinateOwner) continue;


#ifndef DRAW_TEXT_BOUNDING_BOX_WITH_COLLISIONS
                if (object->animationCoordinator->isColliding()) {
                    continue;
                }
#endif


                const auto &circles = object->getMapAlignedBoundingCircles(zoomIdentifier, false, true);
                if (labelRotationAlignment == SymbolAlignment::MAP && circles && !circles->empty()) {

                    for (const auto &circle: *circles) {
                        auto projectedCircle = CollisionUtil::getProjectedCircle(circle, env);
                        if (!projectedCircle) {
                            continue;
                        }

                        projectedCircle->x = ((projectedCircle->x / viewport.x) * 2.0) - 1.0;
                        projectedCircle->y = ((projectedCircle->y / viewport.y) * 2.0) - 1.0;
                        float aspectRatio = float(viewport.x) / float(viewport.y);
                        projectedCircle->radius = projectedCircle->radius / halfWidth;

                        const size_t numCirclePoints = 8;
                        std::vector<Coord> coords;
                        coords.emplace_back(CoordinateSystemIdentifiers::RENDERSYSTEM(),
                                            projectedCircle->x, projectedCircle->y, 0.0);
                        for (size_t i = 0; i < numCirclePoints; i++) {
                            float angle = i * (2 * M_PI / numCirclePoints);
                            coords.emplace_back(CoordinateSystemIdentifiers::RENDERSYSTEM(),
                                                projectedCircle->x + projectedCircle->radius * std::cos(angle),
                                                projectedCircle->y + projectedCircle->radius * aspectRatio * std::sin(angle),
                                                0.0);

                            indices.push_back(currentVertexIndex);
                            indices.push_back(currentVertexIndex + i + 1);
                            indices.push_back(currentVertexIndex + (i + 1) % numCirclePoints + 1);
                        }
                        vertices.push_back({coords, (object->animationCoordinator->isColliding()) ? 1 : 0});

                        currentVertexIndex += (numCirclePoints + 1);
                    }
                } else {
                    const auto &viewportAlignedBox = object->getViewportAlignedBoundingBox(zoomIdentifier, false, false);
                    if (viewportAlignedBox) {
                        auto projectedRectangle = CollisionUtil::getProjectedRectangle(*viewportAlignedBox, env);
                        if (projectedRectangle) {
                            projectedRectangle->x = ((projectedRectangle->x / viewport.x) * 2.0) - 1.0;
                            projectedRectangle->y = ((projectedRectangle->y / viewport.y) * 2.0) - 1.0;
                            projectedRectangle->width = (projectedRectangle->width / halfWidth);
                            projectedRectangle->height = (projectedRectangle->height / halfHeight);

                            vertices.push_back({std::vector<::Coord>{
                                    Coord(CoordinateSystemIdentifiers::RENDERSYSTEM(), projectedRectangle->x, projectedRectangle->y,0.0),
                                    Coord(CoordinateSystemIdentifiers::RENDERSYSTEM(), projectedRectangle->x + projectedRectangle->width, projectedRectangle->y, 0.0),
                                    Coord(CoordinateSystemIdentifiers::RENDERSYSTEM(), projectedRectangle->x + projectedRectangle->width, projectedRectangle->y + projectedRectangle->height, 0.0),
                                    Coord(CoordinateSystemIdentifiers::RENDERSYSTEM(), projectedRectangle->x, projectedRectangle->y + projectedRectangle->height, 0.0),
                            }, object->animationCoordinator->isColliding() ? 1 : 0});

                            indices.push_back(currentVertexIndex);
                            indices.push_back(currentVertexIndex + 1);
                            indices.push_back(currentVertexIndex + 2);

                            indices.push_back(currentVertexIndex);
                            indices.push_back(currentVertexIndex + 2);
                            indices.push_back(currentVertexIndex + 3);

                            currentVertexIndex += 4;
                        }
                    }
                }

                boundingBoxLayerObject->getPolygonObject()->clear();
                boundingBoxLayerObject->setVertices(vertices, indices, false);
                boundingBoxLayerObject->getPolygonObject()->setup(mapInterface.lock()->getRenderingContext());
            }
        }
#endif
    }
}

std::optional<Tiled2dMapVectorSymbolSubLayerPositioningWrapper>
Tiled2dMapVectorSymbolGroup::getPositioning(std::vector<::Coord>::const_iterator &iterator, const std::vector<::Coord> &collection,
                                            const double interpolationValue) {

    double distance = 10;

    Vec2D curPoint(iterator->x, iterator->y);


    auto prev = iterator;
    if (prev == collection.begin()) { return std::nullopt; }

    auto onePrev = std::prev(iterator);

    while (Vec2DHelper::distance(Vec2D(prev->x, prev->y), curPoint) < distance) {
        prev = std::prev(prev);

        if (prev == collection.begin()) {
            break;
        }
    }


    auto next = iterator;
    if (next == collection.end()) { return std::nullopt; }

    while (Vec2DHelper::distance(Vec2D(next->x, next->y), curPoint) < distance) {
        next = std::next(next);

        if (next == collection.end()) {
            next = std::prev(next);
            break;
        }
    }

    double angle = -atan2(prev->y - next->y, -(prev->x - next->x)) * (180.0 / M_PI);
    auto midpoint = Vec2D(onePrev->x * (1.0 - interpolationValue) + iterator->x * interpolationValue,
                          onePrev->y * (1.0 - interpolationValue) + iterator->y * interpolationValue);
    return Tiled2dMapVectorSymbolSubLayerPositioningWrapper(angle, Coord(next->systemIdentifier, midpoint.x, midpoint.y, next->z));
}

std::shared_ptr<Tiled2dMapVectorSymbolObject>
Tiled2dMapVectorSymbolGroup::createSymbolObject(const Tiled2dMapVersionedTileInfo &tileInfo,
                                                const std::string &layerIdentifier,
                                                const std::shared_ptr<SymbolVectorLayerDescription> &description,
                                                const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                                const std::shared_ptr<FeatureContext> &featureContext,
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
                                                const size_t symbolTileIndex,
                                                const bool hasCustomTexture) {
    auto symbolObject = std::make_shared<Tiled2dMapVectorSymbolObject>(mapInterface, layerConfig, fontProvider, tileInfo, layerIdentifier,
                                                          description, featureContext, text, fullText, coordinate, lineCoordinates,
                                                          fontList, textAnchor, angle, textJustify, textSymbolPlacement, hideIcon, animationCoordinatorMap,
                                                          featureStateManager, usedKeys, symbolTileIndex, hasCustomTexture, dpFactor, persistingSymbolPlacement, is3d, tileOrigin);
    symbolObject->setAlpha(alpha);
    const auto counts = symbolObject->getInstanceCounts();
    if (counts.icons + counts.stretchedIcons + counts.textCharacters == 0) {
        return nullptr;
    } else {
        return symbolObject;
    }
}

std::vector<SymbolObjectCollisionWrapper> Tiled2dMapVectorSymbolGroup::getSymbolObjectsForCollision() {
    std::vector<SymbolObjectCollisionWrapper> wrapperObjects;
    wrapperObjects.reserve(symbolObjects.size());
    std::transform(symbolObjects.cbegin(), symbolObjects.cend(), std::back_inserter(wrapperObjects),
                   [](const std::shared_ptr<Tiled2dMapVectorSymbolObject> &symbolObject) {
                       return SymbolObjectCollisionWrapper(symbolObject);
                   });
    return wrapperObjects;
}

std::optional<std::tuple<Coord, VectorLayerFeatureInfo>> Tiled2dMapVectorSymbolGroup::onClickConfirmed(const CircleD &clickHitCircle, double zoomIdentifier, CollisionUtil::CollisionEnvironment &collisionEnvironment) {
    if (!anyInteractable) {
        return std::nullopt;
    }
#ifdef DRAW_TEXT_BOUNDING_BOX
    lastClickHitCircle = clickHitCircle;
    mapInterface.lock()->invalidate();
#endif
    for (auto iter = symbolObjects.rbegin(); iter != symbolObjects.rend(); ++iter) {
        const auto result = (*iter)->onClickConfirmed(clickHitCircle, zoomIdentifier, collisionEnvironment);
        if (result) {
            return result;
        }
    }
    return std::nullopt;
}

void Tiled2dMapVectorSymbolGroup::setAlpha(float alpha) {
    for (auto const object: symbolObjects) {
        object->setAlpha(alpha);
    }
}

void Tiled2dMapVectorSymbolGroup::clear() {
    if (iconInstancedObject) {
        iconInstancedObject->asGraphicsObject()->clear();
    }
    if (stretchedInstancedObject) {
        stretchedInstancedObject->asGraphicsObject()->clear();
    }
    for (const auto &textInstancedObject : textInstancedObjects) {
        textInstancedObject->asGraphicsObject()->clear();
    }
    this->spriteData = nullptr;
    this->spriteTexture = nullptr;
}

void Tiled2dMapVectorSymbolGroup::placedInCache() {
    for (auto const object: symbolObjects) {
        object->placedInCache();
    }
}

void Tiled2dMapVectorSymbolGroup::removeFromCache() {
    for (auto const object: symbolObjects) {
        object->removeFromCache();
    }
}


std::vector<std::shared_ptr< ::RenderObjectInterface>> Tiled2dMapVectorSymbolGroup::getRenderObjects() {
    std::vector<std::shared_ptr< ::RenderObjectInterface>> renderObjects;

    auto iconObject = iconInstancedObject;
    if (iconObject) {
        renderObjects.push_back(std::make_shared<RenderObject>(iconObject->asGraphicsObject()));
    }

    for (const auto &descriptor: customTextures) {
        renderObjects.push_back(std::make_shared<RenderObject>(descriptor.renderObject->asGraphicsObject()));
    }

    auto stretchIconObject = stretchedInstancedObject;
    if (stretchIconObject) {
        renderObjects.push_back(std::make_shared<RenderObject>(stretchIconObject->asGraphicsObject()));
    }
    for (const auto &textInstancedObject : textInstancedObjects) {
        renderObjects.push_back(std::make_shared<RenderObject>(textInstancedObject->asGraphicsObject()));
    }

#ifdef DRAW_TEXT_BOUNDING_BOX
    auto boundingBoxLayerObject = this->boundingBoxLayerObject;
    if (boundingBoxLayerObject) {
        renderObjects.push_back(std::make_shared<RenderObject>(boundingBoxLayerObject->getPolygonObject(), true));
    }
#endif

    return renderObjects;
}
