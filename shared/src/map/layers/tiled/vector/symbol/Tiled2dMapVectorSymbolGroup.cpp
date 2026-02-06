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
                                                         const std::weak_ptr<Tiled2dMapVectorLayer> &vectorLayer,
                                                         const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                                         const WeakActor<Tiled2dMapVectorFontProvider> &fontProvider,
                                                         const Tiled2dMapVersionedTileInfo &tileInfo,
                                                         const std::string &layerIdentifier,
                                                         const std::shared_ptr<SymbolVectorLayerDescription> &layerDescription,
                                                         const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager,
                                                         const std::shared_ptr<Tiled2dMapVectorLayerSymbolDelegateInterface> &symbolDelegate)
: groupId(groupId),
mapInterface(mapInterface),
vectorLayer(vectorLayer),
layerConfig(layerConfig),
tileInfo(tileInfo),
layerIdentifier(layerIdentifier),
layerDescription(layerDescription),
fontProvider(fontProvider),
featureStateManager(featureStateManager),
symbolDelegate(symbolDelegate),
usedKeys(layerDescription->getUsedKeys()),
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
    auto strongVectorLayer = vectorLayer.lock();
    auto camera = strongMapInterface ? strongMapInterface->getCamera() : nullptr;
    if (!strongMapInterface || !camera || !strongVectorLayer) {
        symbolManagerActor.message(MFN(&Tiled2dMapVectorSourceSymbolDataManager::onSymbolGroupInitialized), false, tileInfo, layerIdentifier, selfActor);
        return;
    }

    layerBlendMode = layerDescription->style.getBlendMode(EvaluationContext(0.0, dpFactor, std::make_shared<FeatureContext>(), featureStateManager));
    auto alphaInstancedShader =
        is3d ? strongMapInterface->getShaderFactory()->createUnitSphereAlphaInstancedShader()->asShaderProgramInterface()
             : strongMapInterface->getShaderFactory()->createAlphaInstancedShader()->asShaderProgramInterface();
    alphaInstancedShader->setBlendMode(layerBlendMode);

    const double tilePixelFactor =
            (0.0254 / camera->getScreenDensityPpi()) * layerConfig->getZoomFactorAtIdentifier(tileInfo.tileInfo.zoomIdentifier - 1);

    std::unordered_map<std::string, std::vector<Vec2D>> textPositionMap;

    std::vector<VectorLayerFeatureInfo> featureInfosWithCustomAssets;

    bool warnedCustomImageWithoutSymbolDelegate = false;

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

        const bool hasImageFromCustomProviderRequested = layerDescription->style.getIconImageCustomProvider(evalContext);
        if (hasImageFromCustomProviderRequested && !symbolDelegate) {
            if (!warnedCustomImageWithoutSymbolDelegate) {
                LogError << layerIdentifier
                         <<= ": icon-image-custom-provider enabled but no symbol delegate present.";
                warnedCustomImageWithoutSymbolDelegate = true;
            }
        }
        const bool hasImageFromCustomProvider = (hasImageFromCustomProviderRequested && symbolDelegate);

        bool hasIconPotentially;
        if (hasImageFromCustomProvider) {
            hasIconPotentially = true;
        } else if (!hasImageFromCustomProviderRequested && layerDescription->style.iconImageEvaluator.getValue()) {
            // Attempt to evaluate without zoom and state
            const auto zoomAndStateIndependentEvalCtx = EvaluationContext(dpFactor, context.get(), nullptr);
            const auto value = layerDescription->style.iconImageEvaluator.getValue();
            const auto res = value->evaluate(zoomAndStateIndependentEvalCtx);
            const std::string *iconImage = std::get_if<std::string>(&res);
            // if this succeeded returns empty, there is no icon image for any zoom level.
            hasIconPotentially = (iconImage != nullptr && !iconImage->empty());
        } else {
            hasIconPotentially = false;
        }
        if(!hasIconPotentially && fullText.empty()) {
            continue;
        }

        const bool iconOptional = layerDescription->style.getIconOptional(evalContext);
        const bool textOptional = layerDescription->style.getTextOptional(evalContext);

        const auto &pointCoordinates = geometry->getPointCoordinates();

        bool wasPlaced = false;

        if (context->geomType != vtzero::GeomType::POINT) {
            double distance = 0;
            double totalDistance = 0;

            bool isLineCenter = placement == TextSymbolPlacement::LINE_CENTER;

            std::vector<Vec2D> line = {};
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
                                                                     pos->angle, justify, placement, false, animationCoordinatorMap, featureTileIndex, hasImageFromCustomProvider, featureTileIndex);
                        if (symbolObject) {
                            symbolObjects.push_back(symbolObject);
                            textPositionMap[fullText].push_back(position);
                            wasPlaced = true;
                        }

                        if (hasIconPotentially) {
                            if (textOptional) {
                                const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription,
                                                                             layerConfig, context, {}, "", position, line, fontList,
                                                                             anchor, pos->angle, justify, placement, false, animationCoordinatorMap, featureTileIndex, hasImageFromCustomProvider, featureTileIndex);

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
                                                                             true, animationCoordinatorMap, featureTileIndex, hasImageFromCustomProvider, featureTileIndex);

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
                                                                         pos->angle, justify, placement, false, animationCoordinatorMap, featureTileIndex, hasImageFromCustomProvider, featureTileIndex);
                            if (symbolObject) {
                                symbolObjects.push_back(symbolObject);
                                textPositionMap[fullText].push_back(position);
                                wasPlaced = true;
                            }

                            if (hasIconPotentially) {
                                if (textOptional) {
                                    const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription,
                                                                                 layerConfig, context, {}, "", position, line,
                                                                                 fontList, anchor, pos->angle, justify, placement,
                                                                                 false, animationCoordinatorMap, featureTileIndex, hasImageFromCustomProvider, featureTileIndex);
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
                                                                                 placement, true, animationCoordinatorMap, featureTileIndex, hasImageFromCustomProvider, featureTileIndex);
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
                                                                 placement, false, animationCoordinatorMap, featureTileIndex, hasImageFromCustomProvider, featureTileIndex);

                    if (symbolObject) {
                        symbolObjects.push_back(symbolObject);
                        wasPlaced = true;
                    }

                    if (hasIconPotentially && !fullText.empty()) {
                        if (textOptional) {
                            const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription, layerConfig,
                                                                         context, {}, "", mp, std::nullopt, fontList, anchor, angle,
                                                                         justify, placement, false, animationCoordinatorMap, featureTileIndex, hasImageFromCustomProvider, featureTileIndex);

                            if (symbolObject) {
                                symbolObjects.push_back(symbolObject);
                                wasPlaced = true;
                            }
                        }
                        if (iconOptional) {
                            const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription, layerConfig,
                                                                         context, text, fullText, mp, std::nullopt, fontList, anchor,
                                                                         angle, justify, placement, true, animationCoordinatorMap, featureTileIndex, hasImageFromCustomProvider, featureTileIndex);

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
                featureInfosWithCustomAssets.push_back(context->getFeatureInfo(strongVectorLayer->getStringInterner()));
            }
        }
    }

    if (symbolObjects.empty()) {
        symbolManagerActor.message(MFN(&Tiled2dMapVectorSourceSymbolDataManager::onSymbolGroupInitialized), false, tileInfo, layerIdentifier, selfActor);
        return;
    }

    if (!featureInfosWithCustomAssets.empty()) {
        assert(symbolDelegate != nullptr);
        auto customAssetPages = symbolDelegate->getCustomAssetsFor(featureInfosWithCustomAssets, layerDescription->identifier);
        for (const auto &page: customAssetPages) {
            auto object = strongMapInterface->getGraphicsObjectFactory()->createQuadInstanced(alphaInstancedShader);
            object->setInstanceCount((int32_t) page.featureIdentifiersUv.size());
            customTextures.push_back(CustomIconDescriptor(page.texture, object, page.featureIdentifiersUv, is3d));
        }
    }

    //Tiled2dMapVectorSymbolObject::SymbolObjectInstanceCounts instanceCounts{0, 0, 0};
    std::unordered_map<std::shared_ptr<FontLoaderResult>, std::tuple<int32_t, int32_t>> fontStylesAndCharactersCountMap;

    for (auto const object: symbolObjects) {
        const auto &counts = object->getInstanceCounts();
        if (counts.textCharacters > 0) {
            auto font = object->getFont();
            int32_t charactersCount = 0;
            auto currentEntry = fontStylesAndCharactersCountMap.find(font);
            if (currentEntry != fontStylesAndCharactersCountMap.end()) {
                charactersCount = std::get<1>(currentEntry->second);
            }
            fontStylesAndCharactersCountMap[font] = {featuresCount, charactersCount + counts.textCharacters};
        }
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
}

void Tiled2dMapVectorSymbolGroup::setupObjects(const std::vector<std::pair<std::shared_ptr<SpriteData>, std::shared_ptr<::TextureHolderInterface>>> &sprites, const std::optional<WeakActor<Tiled2dMapVectorSourceSymbolDataManager>> &symbolDataManager) {
    if(isInitialized) {
        // TODO: it appears that we land here (at least) twice
        // - during resume
        // - onSymbolGroupInitialized
        // -> figure out if/how we can avoid this
        return;
    }
    const auto context = mapInterface.lock()->getRenderingContext();

    std::unordered_map<std::string, int32_t> textOffsets;

    int positionSize = is3d ? 3 : 2;

    for (auto const &object: symbolObjects) {
        if (object->hasCustomTexture) {
            for (size_t i = 0; i != customTextures.size(); i++) {
                auto identifier = object->stringIdentifier;
                auto uvIt = customTextures[i].featureIdentifiersUv.find(identifier);
                if (uvIt != customTextures[i].featureIdentifiersUv.end()) {
                    object->customTexturePage = i;
                    object->customTextureOffset = (int)std::distance(customTextures[i].featureIdentifiersUv.begin(),uvIt);

                    [[ maybe_unused ]] int offset = object->customTextureOffset;
                    assert(offset < customTextures[i].iconRotations.size());

                    object->setupCustomIconInfo(uvIt->second);
                    break;
                }
            }
        }

        auto font = object->getFont();
        if (font) {
            const auto &textDescriptor = std::find_if(textDescriptors.begin(), textDescriptors.end(),
                                                      [&font](const auto &textDescriptor) {
                                                          return font->fontData->info.name ==
                                                                 textDescriptor->fontResult->fontData->info.name;
                                                      });
            if (textDescriptor != textDescriptors.end()) {
                int32_t currentTextOffset = textOffsets[font->fontData->info.name];
                object->setupTextProperties((*textDescriptor)->textTextureCoordinates,
                                            (*textDescriptor)->textStyleIndices,
                                            currentTextOffset,
                                            tileInfo.tileInfo.zoomIdentifier);
                textOffsets[font->fontData->info.name] = currentTextOffset;
            }
        }
    }

    for (auto &customDescriptor: customTextures) {
        customDescriptor.renderObject->setFrame(Quad2dD(Vec2D(-0.5, 0.5), Vec2D(0.5, 0.5), Vec2D(0.5, -0.5), Vec2D(-0.5, -0.5)), tileOrigin, is3d);
        customDescriptor.renderObject->loadTexture(context, customDescriptor.texture);
        customDescriptor.renderObject->asGraphicsObject()->setup(context);

        int32_t count = (int32_t)customDescriptor.featureIdentifiersUv.size();
        customDescriptor.renderObject->setPositions(SharedBytes((int64_t) customDescriptor.iconPositions.data(), count, positionSize * (int32_t) sizeof(float)));
        customDescriptor.renderObject->setTextureCoordinates(SharedBytes((int64_t) customDescriptor.iconTextureCoordinates.data(), count, 4 * (int32_t) sizeof(float)));
        customDescriptor.iconAlphas.setModified();
        customDescriptor.iconScales.setModified();
        customDescriptor.iconRotations.setModified();
        customDescriptor.iconOffsets.setModified();

    }

    for (size_t i = 0; i < textDescriptors.size(); i++) {
        const auto &textDescriptor = textDescriptors[i];
        const auto &textInstancedObject = textInstancedObjects[i];
        textInstancedObject->setFrame(Quad2dD(Vec2D(-0.5, 0.5), Vec2D(0.5, 0.5), Vec2D(0.5, -0.5), Vec2D(-0.5, -0.5)), tileOrigin, is3d);
        textInstancedObject->loadFont(context, *textDescriptor->fontResult->fontData, textDescriptor->fontResult->imageData);
        textInstancedObject->asGraphicsObject()->setup(context);
        textInstancedObject->setTextureCoordinates(
                SharedBytes((int64_t) textDescriptor->textTextureCoordinates.data(), (int32_t) textDescriptor->textRotations.size(), 4 * (int32_t) sizeof(float)));
        textInstancedObject->setStyleIndices(
                SharedBytes((int64_t) textDescriptor->textStyleIndices.data(), (int32_t) textDescriptor->textStyleIndices.size(), 1 * (int32_t) sizeof(uint16_t)));
        textDescriptor->textReferencePositions.setModified();
        textDescriptor->textStyles.setModified();
        textDescriptor->textScales.setModified();
        textDescriptor->textRotations.setModified();
    }

    for(auto &[spriteData, spriteTexture] : sprites) {
        addSprite(spriteData, spriteTexture);
    }


    if (symbolDataManager.has_value()) {
        auto selfActor = WeakActor<Tiled2dMapVectorSymbolGroup>(mailbox, shared_from_this());
        symbolDataManager->message(MFN(&Tiled2dMapVectorSourceSymbolDataManager::onSymbolGroupInitialized), true, tileInfo,
                                   layerIdentifier, selfActor);
    }

    isInitialized = true;
}

void Tiled2dMapVectorSymbolGroup::addSprite(const std::shared_ptr<SpriteData> &spriteData, const std::shared_ptr<TextureHolderInterface> &spriteTexture) {
   
    // Skip if sprite already added.
    // This can occur because it is called when a sprite is done loading via
    // setupExistingSymbolWithSprite; this can race with the group being setup,
    // and the list passed to setupObjects may or may not contain that sprite.
    auto it = std::find_if(sprites.begin(), sprites.end(), [&spriteData](const SpriteIconDescriptor &spriteDescriptor) {
        return spriteData->identifier == spriteDescriptor.identifier;
    });
    if (it != sprites.end()) {
        return;
    }

    SpriteIconDescriptor &spriteDescriptor = sprites.emplace_back();
    spriteDescriptor.identifier = spriteData->identifier;
    spriteDescriptor.spriteTexture = spriteTexture;

    uint32_t sheetIndex = (uint32_t)sprites.size() - 1;
    std::string sheetName = spriteData->identifier;
    if (sheetName == "default") {
        sheetName = "";
    }
    spriteLookup.reserve(spriteLookup.size() + spriteData->sprites.size());
    spriteIconData.reserve(spriteLookup.size() + spriteData->sprites.size());
    for (auto &[iconName, spriteDesc] : spriteData->sprites) {
        SpriteIconId iconId{sheetName, iconName};
        assert(spriteLookup.count(iconId) == 0);
        uint32_t iconIndex = (uint32_t)spriteIconData.size();
        spriteLookup[iconId] = ResolvedSpriteIconId{sheetIndex, iconIndex};
        spriteIconData.push_back(spriteDesc);
    }
}

// Initialize or update sprite icon instanced object and buffers. Adapt the graphics objects to changed icon counts, initializing it only if icons should be visible.
// Returns true if a different render objects will need to be displayed (i.e. the render passes list needs to be updated).
bool Tiled2dMapVectorSymbolGroup::prepareIconObject(SpriteIconDescriptor &spriteDescriptor, size_t iconCount, size_t stretchedIconCount) {
    bool renderObjectsChanged = false;
    if(iconCount != 0 && spriteDescriptor.iconInstancedObject == nullptr) {
        auto strongMapInterface = this->mapInterface.lock();
        const auto context = strongMapInterface->getRenderingContext();

        auto alphaInstancedShader =
            is3d ? strongMapInterface->getShaderFactory()->createUnitSphereAlphaInstancedShader()->asShaderProgramInterface()
                 : strongMapInterface->getShaderFactory()->createAlphaInstancedShader()->asShaderProgramInterface();
        alphaInstancedShader->setBlendMode(layerBlendMode);
        spriteDescriptor.iconInstancedObject = strongMapInterface->getGraphicsObjectFactory()->createQuadInstanced(alphaInstancedShader);
#if DEBUG
        spriteDescriptor.iconInstancedObject->asGraphicsObject()->setDebugLabel(layerDescription->identifier + "_" + tileInfo.tileInfo.to_string_short());
#endif
        spriteDescriptor.iconInstancedObject->setFrame(Quad2dD(Vec2D(-0.5, 0.5), Vec2D(0.5, 0.5), Vec2D(0.5, -0.5), Vec2D(-0.5, -0.5)), tileOrigin, is3d);
        spriteDescriptor.iconInstancedObject->loadTexture(context, spriteDescriptor.spriteTexture);
        spriteDescriptor.iconInstancedObject->asGraphicsObject()->setup(context);
        renderObjectsChanged = true;
    }

    if (stretchedIconCount != 0 && spriteDescriptor.stretchedInstancedObject == nullptr) {
        auto strongMapInterface = this->mapInterface.lock();
        const auto context = strongMapInterface->getRenderingContext();

        auto shader = strongMapInterface->getShaderFactory()->createStretchInstancedShader(is3d)->asShaderProgramInterface();
        shader->setBlendMode(layerBlendMode);
        spriteDescriptor.stretchedInstancedObject = strongMapInterface->getGraphicsObjectFactory()->createQuadStretchedInstanced(shader);
#if DEBUG
        spriteDescriptor.stretchedInstancedObject->asGraphicsObject()->setDebugLabel(layerDescription->identifier + "_" + tileInfo.tileInfo.to_string_short());
#endif

        spriteDescriptor.stretchedInstancedObject->setFrame(Quad2dD(Vec2D(-0.5, 0.5), Vec2D(0.5, 0.5), Vec2D(0.5, -0.5), Vec2D(-0.5, -0.5)), tileOrigin, is3d);
        spriteDescriptor.stretchedInstancedObject->loadTexture(context, spriteDescriptor.spriteTexture);
        spriteDescriptor.stretchedInstancedObject->asGraphicsObject()->setup(context);
        renderObjectsChanged = true;
    }

    const int positionSize = is3d ? 3 : 2;

    if(spriteDescriptor.iconInstancedObject && spriteDescriptor.iconAlphas.size() != iconCount) {
        spriteDescriptor.iconInstancedObject->setInstanceCount(iconCount);
        
        spriteDescriptor.iconAlphas.resize(iconCount, 0.0);
        spriteDescriptor.iconRotations.resize(iconCount, 0.0);
        spriteDescriptor.iconScales.resize(iconCount * 2, 0.0);
        spriteDescriptor.iconPositions.resize(iconCount * positionSize, 0.0);
        spriteDescriptor.iconTextureCoordinates.resize(iconCount * 4, 0.0);
        spriteDescriptor.iconOffsets.resize(iconCount * 2, 0.0);
    }


    if(spriteDescriptor.stretchedInstancedObject && spriteDescriptor.stretchedIconAlphas.size() != stretchedIconCount) {
        spriteDescriptor.stretchedInstancedObject->setInstanceCount(stretchedIconCount);
        
        spriteDescriptor.stretchedIconAlphas.resize(stretchedIconCount, 0.0);
        spriteDescriptor.stretchedIconRotations.resize(stretchedIconCount, 0.0);
        spriteDescriptor.stretchedIconScales.resize(stretchedIconCount * 2, 0.0);
        spriteDescriptor.stretchedIconPositions.resize(stretchedIconCount * positionSize, 0.0);
        spriteDescriptor.stretchedIconStretchInfos.resize(stretchedIconCount * 10, 1.0);
        spriteDescriptor.stretchedIconTextureCoordinates.resize(stretchedIconCount * 4, 0.0);
    }
    return renderObjectsChanged;
}

bool Tiled2dMapVectorSymbolGroup::update(const double zoomIdentifier, const double rotation, const double scaleFactor, int64_t now, const Vec2I viewPortSize, const std::vector<float>& vpMatrix, const Vec3D& origin) {
    bool renderObjectsChanged = false;
    if (!isInitialized) {
        return renderObjectsChanged;
    }
    if (symbolObjects.empty()) {
        return renderObjectsChanged;
    }

    // Count the number of icons per sheet to update instance counts and buffer sizes.
    for(auto &spriteDescriptor : sprites) {
        spriteDescriptor.tmpIconCounter = 0;
        spriteDescriptor.tmpStretchedIconCounter = 0;
    }
    for (auto const &object: symbolObjects) {
        auto spriteIconRef = object->getUpdatedSpriteIconRef(zoomIdentifier, spriteLookup);
        if(spriteIconRef && !object->hasCustomTexture) {
            const auto objectInstanceCounts = object->getInstanceCounts();
            
            auto &spriteDescriptor = sprites.at(spriteIconRef->sheet);
            spriteDescriptor.tmpIconCounter += objectInstanceCounts.icons;
            spriteDescriptor.tmpStretchedIconCounter += objectInstanceCounts.stretchedIcons;
        }
    }
    for(auto &spriteDescriptor : sprites) {
        renderObjectsChanged |= prepareIconObject(spriteDescriptor, spriteDescriptor.tmpIconCounter, spriteDescriptor.tmpStretchedIconCounter);
        spriteDescriptor.tmpIconCounter = 0;
        spriteDescriptor.tmpStretchedIconCounter = 0;
    }
    

        std::unordered_map<std::string, int32_t> textOffsets;
        int32_t singleTextOffset = 0;

        for (auto &object : symbolObjects) {
            auto spriteIconRef = object->getSpriteIconRef();

            if (object->hasCustomTexture) {
                auto &page = customTextures[object->customTexturePage];
                uint32_t offset = object->customTextureOffset;
                object->updateIconProperties(page.iconPositions, page.iconScales, page.iconRotations, page.iconAlphas, page.iconOffsets, page.iconTextureCoordinates, offset, zoomIdentifier,scaleFactor, rotation, now, viewPortSize, page.texture, spriteIconData);
            } else if (spriteIconRef) {
                SpriteIconDescriptor &spriteDescriptor = sprites.at(spriteIconRef->sheet);
                if(object->getInstanceCounts().icons) {
                    uint32_t &iconOffset = spriteDescriptor.tmpIconCounter;
                    object->updateIconProperties(spriteDescriptor.iconPositions, spriteDescriptor.iconScales, spriteDescriptor.iconRotations, spriteDescriptor.iconAlphas, spriteDescriptor.iconOffsets, spriteDescriptor.iconTextureCoordinates, iconOffset, zoomIdentifier,
                                                 scaleFactor, rotation, now, viewPortSize, spriteDescriptor.spriteTexture, spriteIconData);
                } else {
                    uint32_t &stretchedIconOffset = spriteDescriptor.tmpStretchedIconCounter;
                    object->updateStretchIconProperties(spriteDescriptor.stretchedIconPositions, spriteDescriptor.stretchedIconScales, spriteDescriptor.stretchedIconRotations,
                                                        spriteDescriptor.stretchedIconAlphas, spriteDescriptor.stretchedIconStretchInfos, spriteDescriptor.stretchedIconTextureCoordinates, stretchedIconOffset, zoomIdentifier,
                                                        scaleFactor, rotation, now, viewPortSize, spriteDescriptor.spriteTexture, spriteIconData);
                }
            } else {
                object->resetLastIconProperties();
            }

            auto font = object->getFont();
            if (font) {
                auto n = textDescriptors.size();

                if(n > 1) {
                    auto &name = font->fontData->info.name;

                    const auto &textDescriptor = std::find_if(textDescriptors.begin(), textDescriptors.end(),
                                                              [&name](const auto &textDescriptor) {
                        return name == textDescriptor->fontResult->fontData->info.name;
                    });

                    if (textDescriptor != textDescriptors.end()) {
                        int32_t currentTextOffset = textOffsets[name];
                        object->updateTextProperties((*textDescriptor)->textPositions, (*textDescriptor)->textReferencePositions,
                                                     (*textDescriptor)->textScales, (*textDescriptor)->textRotations,
                                                     (*textDescriptor)->textAlphas, (*textDescriptor)->textStyles,
                                                     currentTextOffset,
                                                     zoomIdentifier, scaleFactor, rotation, now, viewPortSize, vpMatrix, origin);
                        textOffsets[name] = currentTextOffset;
                    }
                } else if(n == 1) {
                    // use first, as it has to be this one, otherwise multiple fonts
                    // would have been registered
                    const auto &td = *(textDescriptors.begin());
                    object->updateTextProperties(td->textPositions, td->textReferencePositions,
                                                 td->textScales, td->textRotations,
                                                 td->textAlphas, td->textStyles,
                                                 singleTextOffset,
                                                 zoomIdentifier, scaleFactor, rotation, now, viewPortSize, vpMatrix, origin);
                }
            }
        }

        const int positionSize = is3d ? 3 : 2;
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

        for (auto &spriteDescriptor: sprites) {
            if(spriteDescriptor.iconInstancedObject) {
                int32_t iconCount = (int32_t) spriteDescriptor.iconAlphas.size();
                if (spriteDescriptor.iconPositions.wasModified()) {
                    spriteDescriptor.iconInstancedObject->setPositions(
                                                      SharedBytes((int64_t) spriteDescriptor.iconPositions.data(), iconCount, positionSize * (int32_t) sizeof(float)));
                    spriteDescriptor.iconPositions.resetModificationFlag();
                }

                if (spriteDescriptor.iconAlphas.wasModified()) {
                    spriteDescriptor.iconInstancedObject->setAlphas(
                                                   SharedBytes((int64_t) spriteDescriptor.iconAlphas.data(), iconCount, (int32_t) sizeof(float)));
                    spriteDescriptor.iconAlphas.resetModificationFlag();
                }

                if (spriteDescriptor.iconScales.wasModified()) {
                    spriteDescriptor.iconInstancedObject->setScales(
                                                   SharedBytes((int64_t) spriteDescriptor.iconScales.data(), iconCount, 2 * (int32_t) sizeof(float)));
                    spriteDescriptor.iconScales.resetModificationFlag();
                }

                if (spriteDescriptor.iconRotations.wasModified()) {
                    spriteDescriptor.iconInstancedObject->setRotations(
                                                      SharedBytes((int64_t) spriteDescriptor.iconRotations.data(), iconCount, 1 * (int32_t) sizeof(float)));
                    spriteDescriptor.iconRotations.resetModificationFlag();
                }

                if (spriteDescriptor.iconOffsets.wasModified()) {
                    spriteDescriptor.iconInstancedObject->setPositionOffset(
                                                           SharedBytes((int64_t) spriteDescriptor.iconOffsets.data(), iconCount, 2 * (int32_t) sizeof(float)));
                    spriteDescriptor.iconOffsets.resetModificationFlag();
                }

                if (spriteDescriptor.iconTextureCoordinates.wasModified()) {
                    spriteDescriptor.iconInstancedObject->setTextureCoordinates(
                                                           SharedBytes((int64_t) spriteDescriptor.iconTextureCoordinates.data(), iconCount, 4 * (int32_t) sizeof(float)));
                    spriteDescriptor.iconTextureCoordinates.resetModificationFlag();
                }
            }

            if(spriteDescriptor.stretchedInstancedObject) {
                int32_t iconCount = (int32_t) spriteDescriptor.stretchedIconAlphas.size();
                if (spriteDescriptor.stretchedIconPositions.wasModified()) {
                    spriteDescriptor.stretchedInstancedObject->setPositions(SharedBytes((int64_t) spriteDescriptor.stretchedIconPositions.data(), iconCount,
                                                                       positionSize * (int32_t) sizeof(float)));
                    spriteDescriptor.stretchedIconPositions.resetModificationFlag();
                }

                if (spriteDescriptor.stretchedIconAlphas.wasModified()) {
                    spriteDescriptor.stretchedInstancedObject->setAlphas(SharedBytes((int64_t) spriteDescriptor.stretchedIconAlphas.data(), iconCount,
                                                                    (int32_t) sizeof(float)));
                    spriteDescriptor.stretchedIconAlphas.resetModificationFlag();
                }

                if (spriteDescriptor.stretchedIconScales.wasModified()) {
                    spriteDescriptor.stretchedInstancedObject->setScales(SharedBytes((int64_t) spriteDescriptor.stretchedIconScales.data(), iconCount,
                                                                    2 * (int32_t) sizeof(float)));
                    spriteDescriptor.stretchedIconScales.resetModificationFlag();
                }

                if (spriteDescriptor.stretchedIconRotations.wasModified()) {
                    spriteDescriptor.stretchedInstancedObject->setRotations(SharedBytes((int64_t) spriteDescriptor.stretchedIconRotations.data(), iconCount,
                                                                       1 * (int32_t) sizeof(float)));
                    spriteDescriptor.stretchedIconRotations.resetModificationFlag();
                }

                if (spriteDescriptor.stretchedIconStretchInfos.wasModified()) {
                    spriteDescriptor.stretchedInstancedObject->setStretchInfos(SharedBytes((int64_t) spriteDescriptor.stretchedIconStretchInfos.data(), iconCount,
                                                                          10 * (int32_t) sizeof(float)));
                    spriteDescriptor.stretchedIconStretchInfos.resetModificationFlag();
                }

                if (spriteDescriptor.stretchedIconTextureCoordinates.wasModified()) {
                    spriteDescriptor.stretchedInstancedObject->setTextureCoordinates(SharedBytes((int64_t) spriteDescriptor.stretchedIconTextureCoordinates.data(), iconCount,
                                                                          4 * (int32_t) sizeof(float)));
                    spriteDescriptor.stretchedIconTextureCoordinates.resetModificationFlag();
                }
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
                textInstancedObject->setStyles(SharedBytes((int64_t) textDescriptor->textStyles.data(), (int32_t) textDescriptor->textStyles.size() / 4, 4 * (int32_t) sizeof(float)));
                textDescriptor->textStyles.resetModificationFlag();
            }

            if (textDescriptor->textScales.wasModified()) {
                textInstancedObject->setScales(SharedBytes((int64_t) textDescriptor->textScales.data(), (int32_t) textDescriptor->textRotations.size(), 2 * (int32_t) sizeof(float)));
                textDescriptor->textScales.resetModificationFlag();
            }

            if (textDescriptor->textAlphas.wasModified()) {
                textInstancedObject->setAlphas(SharedBytes((int64_t) textDescriptor->textAlphas.data(), (int32_t) textDescriptor->textAlphas.size(), 1 * (int32_t) sizeof(float)));
                textDescriptor->textAlphas.resetModificationFlag();
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
    return renderObjectsChanged;
}

std::optional<Tiled2dMapVectorSymbolSubLayerPositioningWrapper>
Tiled2dMapVectorSymbolGroup::getPositioning(std::vector<::Vec2D>::const_iterator &iterator, const std::vector<::Vec2D> &collection,
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
    return Tiled2dMapVectorSymbolSubLayerPositioningWrapper(angle, midpoint);
}

std::shared_ptr<Tiled2dMapVectorSymbolObject>
Tiled2dMapVectorSymbolGroup::createSymbolObject(const Tiled2dMapVersionedTileInfo &tileInfo,
                                                const std::string &layerIdentifier,
                                                const std::shared_ptr<SymbolVectorLayerDescription> &description,
                                                const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                                const std::shared_ptr<FeatureContext> &featureContext,
                                                const std::vector<FormattedStringEntry> &text,
                                                const std::string &fullText,
                                                const ::Vec2D &coordinate,
                                                const std::optional<std::vector<Vec2D>> &lineCoordinates,
                                                const std::vector<std::string> &fontList,
                                                const Anchor &textAnchor,
                                                const std::optional<double> &angle,
                                                const TextJustify &textJustify,
                                                const TextSymbolPlacement &textSymbolPlacement,
                                                const bool hideIcon,
                                                std::shared_ptr<SymbolAnimationCoordinatorMap> animationCoordinatorMap,
                                                const size_t symbolTileIndex,
                                                const bool hasCustomTexture,
                                                const uint16_t styleIndex) {
    auto symbolObject = std::make_shared<Tiled2dMapVectorSymbolObject>(mapInterface, layerConfig, fontProvider, tileInfo, layerIdentifier,
                                                          description, featureContext, text, fullText, coordinate, lineCoordinates,
                                                          fontList, textAnchor, angle, textJustify, textSymbolPlacement, hideIcon, animationCoordinatorMap,
                                                          featureStateManager, usedKeys, symbolTileIndex, hasCustomTexture, dpFactor,
                                                          is3d, tileOrigin, styleIndex);
    symbolObject->setAlpha(alpha);
    const auto counts = symbolObject->getInstanceCounts();
    if (counts.icons + counts.stretchedIcons + counts.textCharacters == 0) {
        return nullptr;
    } else {
        return symbolObject;
    }
}

const std::vector<std::shared_ptr<Tiled2dMapVectorSymbolObject>>& Tiled2dMapVectorSymbolGroup::getSymbolObjectsForCollision() const {
    return symbolObjects;
}

std::optional<std::tuple<Coord, VectorLayerFeatureInfo>> Tiled2dMapVectorSymbolGroup::onClickConfirmed(const CircleD &clickHitCircle, double zoomIdentifier, CollisionUtil::CollisionEnvironment &collisionEnvironment) {
    if (!anyInteractable) {
        return std::nullopt;
    }
    auto strongVectorLayer = vectorLayer.lock();
    if (!strongVectorLayer) {
        return std::nullopt;
    }
    const StringInterner& stringTable = strongVectorLayer->getStringInterner();
#ifdef DRAW_TEXT_BOUNDING_BOX
    lastClickHitCircle = clickHitCircle;
    mapInterface.lock()->invalidate();
#endif
    for (auto iter = symbolObjects.rbegin(); iter != symbolObjects.rend(); ++iter) {
        const auto result = (*iter)->onClickConfirmed(clickHitCircle, zoomIdentifier, collisionEnvironment, stringTable);
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
    for(auto &spriteDescriptor : sprites) {
        if (spriteDescriptor.iconInstancedObject) {
            spriteDescriptor.iconInstancedObject->asGraphicsObject()->clear();
        }
        if (spriteDescriptor.stretchedInstancedObject) {
            spriteDescriptor.stretchedInstancedObject->asGraphicsObject()->clear();
        }
    }
    sprites.clear();
    spriteLookup.clear();
    spriteIconData.clear();
    for (auto const &object: symbolObjects) {
        object->resetLastIconProperties();
    }
    for (const auto &textInstancedObject : textInstancedObjects) {
        textInstancedObject->asGraphicsObject()->clear();
    }
    isInitialized = false;
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

    for(auto &spriteDescriptor : sprites) {
        auto iconObject = spriteDescriptor.iconInstancedObject;
        if (iconObject) {
            renderObjects.push_back(std::make_shared<RenderObject>(iconObject->asGraphicsObject()));
        }
        auto stretchIconObject = spriteDescriptor.stretchedInstancedObject;
        if (stretchIconObject) {
            renderObjects.push_back(std::make_shared<RenderObject>(stretchIconObject->asGraphicsObject()));
        }
    }

    for (const auto &descriptor: customTextures) {
        renderObjects.push_back(std::make_shared<RenderObject>(descriptor.renderObject->asGraphicsObject()));
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
