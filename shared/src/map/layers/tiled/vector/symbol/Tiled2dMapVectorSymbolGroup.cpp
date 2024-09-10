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
          persistingSymbolPlacement(persistingSymbolPlacement) {
    if (auto strongMapInterface = mapInterface.lock()) {
        dpFactor = strongMapInterface->getCamera()->getScreenDensityPpi() / 160.0;
    }
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
        symbolManagerActor.message(&Tiled2dMapVectorSourceSymbolDataManager::onSymbolGroupInitialized, false, tileInfo, layerIdentifier, selfActor);
        return;
    }


    auto strongMapInterface = this->mapInterface.lock();
    auto camera = strongMapInterface ? strongMapInterface->getCamera() : nullptr;
    if (!strongMapInterface || !camera) {
        symbolManagerActor.message(&Tiled2dMapVectorSourceSymbolDataManager::onSymbolGroupInitialized, false, tileInfo, layerIdentifier, selfActor);
        return;
    }

    auto alphaInstancedShader = strongMapInterface->getShaderFactory()->createAlphaInstancedShader()->asShaderProgramInterface();

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

                    double interpolationValue;
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
                auto midP = p.begin() + p.size() / 2;
                std::optional<double> angle = std::nullopt;

                const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription, layerConfig, context,
                                                             text, fullText, *midP, std::nullopt, fontList, anchor, angle, justify,
                                                             placement, false, animationCoordinatorMap, featureTileIndex, hasImageFromCustomProvider);

                if (symbolObject) {
                    symbolObjects.push_back(symbolObject);
                    wasPlaced = true;
                }

                if (hasIcon) {
                    if (textOptional) {
                        const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription, layerConfig,
                                                                     context, {}, "", *midP, std::nullopt, fontList, anchor, angle,
                                                                     justify, placement, false, animationCoordinatorMap, featureTileIndex, hasImageFromCustomProvider);

                        if (symbolObject) {
                            symbolObjects.push_back(symbolObject);
                            wasPlaced = true;
                        }
                    }
                    if (iconOptional) {
                        const auto symbolObject = createSymbolObject(tileInfo, layerIdentifier, layerDescription, layerConfig,
                                                                     context, text, fullText, *midP, std::nullopt, fontList, anchor,
                                                                     angle, justify, placement, true, animationCoordinatorMap, featureTileIndex, hasImageFromCustomProvider);

                        if (symbolObject) {
                            symbolObjects.push_back(symbolObject);
                            wasPlaced = true;
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
        symbolManagerActor.message(&Tiled2dMapVectorSourceSymbolDataManager::onSymbolGroupInitialized, false, tileInfo, layerIdentifier, selfActor);
        return;
    }

    if (symbolDelegate && !featureInfosWithCustomAssets.empty()) {
        auto customAssetPages = symbolDelegate->getCustomAssetsFor(featureInfosWithCustomAssets, layerDescription->identifier);
        for (const auto &page: customAssetPages) {
            auto object = strongMapInterface->getGraphicsObjectFactory()->createQuadInstanced(alphaInstancedShader);
            object->setInstanceCount((int32_t) page.featureIdentifiersUv.size());
            customTextures.push_back(CustomIconDescriptor(page.texture, object, page.featureIdentifiersUv));
        }
    }

    Tiled2dMapVectorSymbolObject::SymbolObjectInstanceCounts instanceCounts{0, 0, 0};
    int textStyleCount = 0;
    for (auto const object: symbolObjects) {
        const auto &counts = object->getInstanceCounts();
        if (!object->hasCustomTexture) {
            instanceCounts.icons += counts.icons;
        }
        instanceCounts.textCharacters += counts.textCharacters;
        textStyleCount += instanceCounts.textCharacters == 0 ? 0 : 1;
        if (counts.textCharacters != 0 && !fontResult) {
            fontResult = object->getFont();
        }
        instanceCounts.stretchedIcons += counts.stretchedIcons;
    }

    if (instanceCounts.icons != 0) {
        alphaInstancedShader->setBlendMode(
                layerDescription->style.getBlendMode(EvaluationContext(0.0, dpFactor, std::make_shared<FeatureContext>(), featureStateManager)));
        iconInstancedObject = strongMapInterface->getGraphicsObjectFactory()->createQuadInstanced(alphaInstancedShader);
#if DEBUG
        iconInstancedObject->asGraphicsObject()->setDebugLabel(layerDescription->identifier + "_" + tileInfo.tileInfo.to_string_short());
#endif

        iconInstancedObject->setInstanceCount(instanceCounts.icons);

        iconAlphas.resize(instanceCounts.icons, 0.0);
        iconRotations.resize(instanceCounts.icons, 0.0);
        iconScales.resize(instanceCounts.icons * 2, 0.0);
        iconPositions.resize(instanceCounts.icons * 2, 0.0);
        iconTextureCoordinates.resize(instanceCounts.icons * 4, 0.0);
    }


    if (instanceCounts.stretchedIcons != 0) {
        auto shader = strongMapInterface->getShaderFactory()->createStretchInstancedShader()->asShaderProgramInterface();
        shader->setBlendMode(
                layerDescription->style.getBlendMode(EvaluationContext(0.0, dpFactor, std::make_shared<FeatureContext>(), featureStateManager)));
        stretchedInstancedObject = strongMapInterface->getGraphicsObjectFactory()->createQuadStretchedInstanced(shader);
#if DEBUG
        stretchedInstancedObject->asGraphicsObject()->setDebugLabel(layerDescription->identifier + "_" + tileInfo.tileInfo.to_string_short());
#endif

        stretchedInstancedObject->setInstanceCount(instanceCounts.stretchedIcons);

        stretchedIconAlphas.resize(instanceCounts.stretchedIcons, 0.0);
        stretchedIconRotations.resize(instanceCounts.stretchedIcons, 0.0);
        stretchedIconScales.resize(instanceCounts.stretchedIcons * 2, 0.0);
        stretchedIconPositions.resize(instanceCounts.stretchedIcons * 2, 0.0);
        stretchedIconStretchInfos.resize(instanceCounts.stretchedIcons * 10, 1.0);
        stretchedIconTextureCoordinates.resize(instanceCounts.stretchedIcons * 4, 0.0);
    }

    if (instanceCounts.textCharacters != 0) {
        auto shader = strongMapInterface->getShaderFactory()->createTextInstancedShader()->asShaderProgramInterface();
        shader->setBlendMode(
                layerDescription->style.getBlendMode(EvaluationContext(0.0, dpFactor, std::make_shared<FeatureContext>(), featureStateManager)));
        textInstancedObject = strongMapInterface->getGraphicsObjectFactory()->createTextInstanced(shader);
#if DEBUG
        textInstancedObject->asGraphicsObject()->setDebugLabel(layerDescription->identifier + "_" + tileInfo.tileInfo.to_string_short());
#endif

        textInstancedObject->setInstanceCount(instanceCounts.textCharacters);

        textStyles.resize(textStyleCount * 10, 0.0);
        textStyleIndices.resize(instanceCounts.textCharacters, 0);
        textRotations.resize(instanceCounts.textCharacters, 0.0);
        textScales.resize(instanceCounts.textCharacters * 2, 0.0);
        textPositions.resize(instanceCounts.textCharacters * 2, 0.0);
        textTextureCoordinates.resize(instanceCounts.textCharacters * 4, 0.0);
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
    if (instanceCounts.icons + instanceCounts.stretchedIcons + instanceCounts.textCharacters > 0) {
        auto shader = strongMapInterface->getShaderFactory()->createPolygonGroupShader(false);
        auto object = strongMapInterface->getGraphicsObjectFactory()->createPolygonGroup(shader->asShaderProgramInterface());
        boundingBoxLayerObject = std::make_shared<PolygonGroup2dLayerObject>(strongMapInterface->getCoordinateConverterHelper(),
                                                                             object, shader);
        boundingBoxLayerObject->setStyles({
                                                  PolygonStyle(Color(0, 1, 0, 0.3), 0.3),
                                                  PolygonStyle(Color(1, 0, 0, 0.3), 1.0)
                                          });
    }
#endif

    setAlpha(alpha);

    symbolManagerActor.message(&Tiled2dMapVectorSourceSymbolDataManager::onSymbolGroupInitialized, true, tileInfo, layerIdentifier, selfActor);
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
    int textOffset = 0;
    uint16_t textStyleOffset = 0;

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

        object->setupTextProperties(textTextureCoordinates, textStyleIndices, textOffset, textStyleOffset, tileInfo.tileInfo.zoomIdentifier);
    }

    for (const auto &customDescriptor: customTextures) {
        customDescriptor.renderObject->setFrame(Quad2dD(Vec2D(-0.5, 0.5), Vec2D(0.5, 0.5), Vec2D(0.5, -0.5), Vec2D(-0.5, -0.5)));
        customDescriptor.renderObject->loadTexture(context, customDescriptor.texture);
        customDescriptor.renderObject->asGraphicsObject()->setup(context);

        int32_t count = (int32_t)customDescriptor.featureIdentifiersUv.size();
        customDescriptor.renderObject->setPositions(SharedBytes((int64_t) customDescriptor.iconPositions.data(), count, 2 * (int32_t) sizeof(float)));
        customDescriptor.renderObject->setTextureCoordinates(SharedBytes((int64_t) customDescriptor.iconTextureCoordinates.data(), count, 4 * (int32_t) sizeof(float)));
    }

    if (spriteTexture && spriteData && iconInstancedObject) {
        if (this->spriteData == nullptr) {
            iconInstancedObject->setFrame(Quad2dD(Vec2D(-0.5, 0.5), Vec2D(0.5, 0.5), Vec2D(0.5, -0.5), Vec2D(-0.5, -0.5)));
            iconInstancedObject->loadTexture(context, spriteTexture);
            iconInstancedObject->asGraphicsObject()->setup(context);
        }
        iconInstancedObject->setPositions(
                SharedBytes((int64_t) iconPositions.data(), (int32_t) iconAlphas.size(), 2 * (int32_t) sizeof(float)));
        iconInstancedObject->setTextureCoordinates(
                SharedBytes((int64_t) iconTextureCoordinates.data(), (int32_t) iconAlphas.size(), 4 * (int32_t) sizeof(float)));
    }

    if (spriteTexture && spriteData && stretchedInstancedObject) {
        if (this->spriteData == nullptr) {
            stretchedInstancedObject->setFrame(Quad2dD(Vec2D(-0.5, 0.5), Vec2D(0.5, 0.5), Vec2D(0.5, -0.5), Vec2D(-0.5, -0.5)));
            stretchedInstancedObject->loadTexture(context, spriteTexture);
            stretchedInstancedObject->asGraphicsObject()->setup(context);
        }
        stretchedInstancedObject->setPositions(
                SharedBytes((int64_t) stretchedIconPositions.data(), (int32_t) stretchedIconAlphas.size(),
                            2 * (int32_t) sizeof(float)));
        stretchedInstancedObject->setTextureCoordinates(
                SharedBytes((int64_t) stretchedIconTextureCoordinates.data(), (int32_t) stretchedIconAlphas.size(),
                            4 * (int32_t) sizeof(float)));
    }

    if (textInstancedObject) {
        if (this->spriteData == nullptr) {
            textInstancedObject->setFrame(Quad2dD(Vec2D(-0.5, 0.5), Vec2D(0.5, 0.5), Vec2D(0.5, -0.5), Vec2D(-0.5, -0.5)));
            textInstancedObject->loadTexture(context, fontResult->imageData);
            textInstancedObject->asGraphicsObject()->setup(context);
        }
        textInstancedObject->setTextureCoordinates(
                SharedBytes((int64_t) textTextureCoordinates.data(), (int32_t) textRotations.size(), 4 * (int32_t) sizeof(float)));
        textInstancedObject->setStyleIndices(
                SharedBytes((int64_t) textStyleIndices.data(), (int32_t) textStyleIndices.size(), 1 * (int32_t) sizeof(uint16_t)));
    }

    this->spriteData = spriteData;
    this->spriteTexture = spriteTexture;

    if (symbolDataManager.has_value()) {
        auto selfActor = WeakActor<Tiled2dMapVectorSymbolGroup>(mailbox, shared_from_this());
        symbolDataManager->message(&Tiled2dMapVectorSourceSymbolDataManager::onSymbolGroupInitialized, true, tileInfo,
                                   layerIdentifier, selfActor);
    }

    isInitialized = true;
}

void Tiled2dMapVectorSymbolGroup::update(const double zoomIdentifier, const double rotation, const double scaleFactor, long long now) {
    if (!isInitialized) {
        return;
    }
    // icons
    if (!symbolObjects.empty()) {

        int iconOffset = 0;
        int stretchedIconOffset = 0;
        int textOffset = 0;
        uint16_t textStyleOffset = 0;

        for (auto const &object: symbolObjects) {
            if (object->hasCustomTexture) {
                auto &page = customTextures[object->customTexturePage];
                int offset = (int)object->customTextureOffset;
                object->updateIconProperties(page.iconPositions, page.iconScales, page.iconRotations, page.iconAlphas, offset, zoomIdentifier,scaleFactor, rotation, now);

            } else {
                object->updateIconProperties(iconPositions, iconScales, iconRotations, iconAlphas, iconOffset, zoomIdentifier,
                                             scaleFactor, rotation, now);
            }
            object->updateStretchIconProperties(stretchedIconPositions, stretchedIconScales, stretchedIconRotations,
                                                stretchedIconAlphas, stretchedIconStretchInfos, stretchedIconOffset, zoomIdentifier,
                                                scaleFactor, rotation, now);
            object->updateTextProperties(textPositions, textScales, textRotations, textStyles, textOffset, textStyleOffset,
                                         zoomIdentifier, scaleFactor, rotation, now);
        }

        for (const auto &customDescriptor: customTextures) {
            int32_t count = (int32_t)customDescriptor.featureIdentifiersUv.size();
            customDescriptor.renderObject->setPositions(
                    SharedBytes((int64_t) customDescriptor.iconPositions.data(), (int32_t) count, 2 * (int32_t) sizeof(float)));
            customDescriptor.renderObject->setAlphas(
                    SharedBytes((int64_t) customDescriptor.iconAlphas.data(), (int32_t) count, (int32_t) sizeof(float)));
            customDescriptor.renderObject->setScales(
                    SharedBytes((int64_t) customDescriptor.iconScales.data(), (int32_t) count, 2 * (int32_t) sizeof(float)));
            customDescriptor.renderObject->setRotations(
                    SharedBytes((int64_t) customDescriptor.iconRotations.data(), (int32_t) count, 1 * (int32_t) sizeof(float)));
        }

        if (iconInstancedObject) {
            iconInstancedObject->setPositions(
                    SharedBytes((int64_t) iconPositions.data(), (int32_t) iconAlphas.size(), 2 * (int32_t) sizeof(float)));
            iconInstancedObject->setAlphas(
                    SharedBytes((int64_t) iconAlphas.data(), (int32_t) iconAlphas.size(), (int32_t) sizeof(float)));
            iconInstancedObject->setScales(
                    SharedBytes((int64_t) iconScales.data(), (int32_t) iconAlphas.size(), 2 * (int32_t) sizeof(float)));
            iconInstancedObject->setRotations(
                    SharedBytes((int64_t) iconRotations.data(), (int32_t) iconAlphas.size(), 1 * (int32_t) sizeof(float)));
        }

        if (stretchedInstancedObject) {
            stretchedInstancedObject->setPositions(
                    SharedBytes((int64_t) stretchedIconPositions.data(), (int32_t) stretchedIconAlphas.size(),
                                2 * (int32_t) sizeof(float)));
            stretchedInstancedObject->setAlphas(
                    SharedBytes((int64_t) stretchedIconAlphas.data(), (int32_t) stretchedIconAlphas.size(),
                                (int32_t) sizeof(float)));
            stretchedInstancedObject->setScales(
                    SharedBytes((int64_t) stretchedIconScales.data(), (int32_t) stretchedIconAlphas.size(),
                                2 * (int32_t) sizeof(float)));
            stretchedInstancedObject->setRotations(
                    SharedBytes((int64_t) stretchedIconRotations.data(), (int32_t) stretchedIconAlphas.size(),
                                1 * (int32_t) sizeof(float)));
            stretchedInstancedObject->setStretchInfos(
                    SharedBytes((int64_t) stretchedIconStretchInfos.data(), (int32_t) stretchedIconAlphas.size(),
                                10 * (int32_t) sizeof(float)));
        }

        if (textInstancedObject) {
            textInstancedObject->setPositions(
                    SharedBytes((int64_t) textPositions.data(), (int32_t) textRotations.size(), 2 * (int32_t) sizeof(float)));
            textInstancedObject->setStyles(
                    SharedBytes((int64_t) textStyles.data(), (int32_t) textStyles.size() / 10, 10 * (int32_t) sizeof(float)));
            textInstancedObject->setScales(
                    SharedBytes((int64_t) textScales.data(), (int32_t) textRotations.size(), 2 * (int32_t) sizeof(float)));
            textInstancedObject->setRotations(
                    SharedBytes((int64_t) textRotations.data(), (int32_t) textRotations.size(), 1 * (int32_t) sizeof(float)));
        }

#ifdef DRAW_TEXT_BOUNDING_BOX

        if (boundingBoxLayerObject) {
            std::vector<std::tuple<std::vector<::Coord>, int>> vertices;
            std::vector<uint16_t> indices;

            int32_t currentVertexIndex = 0;
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
                        const size_t numCirclePoints = 8;
                        std::vector<Coord> coords;
                        coords.emplace_back(CoordinateSystemIdentifiers::RENDERSYSTEM(),
                                            circle.x, circle.y, 0.0);
                        for (size_t i = 0; i < numCirclePoints; i++) {
                            float angle = i * (2 * M_PI / numCirclePoints);
                            coords.emplace_back(CoordinateSystemIdentifiers::RENDERSYSTEM(),
                                                circle.x + circle.radius * std::cos(angle),
                                                circle.y + circle.radius * std::sin(angle),
                                                0.0);

                            indices.push_back(currentVertexIndex);
                            indices.push_back(currentVertexIndex + i + 1);
                            indices.push_back(currentVertexIndex + (i + 1) % numCirclePoints + 1);
                        }
                        vertices.push_back({coords, (object->animationCoordinator->isColliding()) ? 1 : 0});

                        currentVertexIndex += (numCirclePoints + 1);
                    }
                } else {
                    const auto &viewportAlignedBox = object->getViewportAlignedBoundingBox(zoomIdentifier, false, true);
                    if (viewportAlignedBox) {
                        // Align rectangle to viewport
                        const double sinAngle = sin(-rotation * M_PI / 180.0);
                        const double cosAngle = cos(-rotation * M_PI / 180.0);
                        Vec2D rotWidth = Vec2D(viewportAlignedBox->width * cosAngle, viewportAlignedBox->width * sinAngle);
                        Vec2D rotHeight = Vec2D(-viewportAlignedBox->height * sinAngle, viewportAlignedBox->height * cosAngle);
                        Vec2D rotOrigin = Vec2DHelper::rotate(Vec2D(viewportAlignedBox->x, viewportAlignedBox->y),
                                                              Vec2D(viewportAlignedBox->anchorX, viewportAlignedBox->anchorY),
                                                              -rotation);
                        vertices.push_back({std::vector<::Coord>{
                                Coord(CoordinateSystemIdentifiers::RENDERSYSTEM(),
                                      rotOrigin.x, rotOrigin.y,0.0),
                                Coord(CoordinateSystemIdentifiers::RENDERSYSTEM(),
                                      rotOrigin.x + rotWidth.x,rotOrigin.y + rotWidth.y, 0.0),
                                Coord(CoordinateSystemIdentifiers::RENDERSYSTEM(),
                                      rotOrigin.x + rotWidth.x + rotHeight.x,rotOrigin.y + rotWidth.y + rotHeight.y, 0.0),
                                Coord(CoordinateSystemIdentifiers::RENDERSYSTEM(),
                                      rotOrigin.x + rotHeight.x,rotOrigin.y + rotHeight.y, 0.0),
                        }, object->animationCoordinator->isColliding() ? 1 : 0});
                        indices.push_back(currentVertexIndex);
                        indices.push_back(currentVertexIndex + 1);
                        indices.push_back(currentVertexIndex + 2);

                        indices.push_back(currentVertexIndex);
                        indices.push_back(currentVertexIndex + 3);
                        indices.push_back(currentVertexIndex + 2);

                        currentVertexIndex += 4;
                    }
                }

                if (currentVertexIndex == 0) {
                    return;
                }

                boundingBoxLayerObject->getPolygonObject()->clear();
                boundingBoxLayerObject->setVertices(vertices, indices);
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

    double angle = -atan2(next->y - prev->y, next->x - prev->x) * (180.0 / M_PI);
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
                                                          featureStateManager, usedKeys, symbolTileIndex, hasCustomTexture, dpFactor, persistingSymbolPlacement);
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

std::optional<std::tuple<Coord, VectorLayerFeatureInfo>> Tiled2dMapVectorSymbolGroup::onClickConfirmed(const CircleD &clickHitCircle) {
    if (!anyInteractable) {
        return std::nullopt;
    }
    for (const auto object: symbolObjects) {
        const auto result = object->onClickConfirmed(clickHitCircle);
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
    if (textInstancedObject) {
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
    auto textObject = textInstancedObject;
    if (textObject) {
        renderObjects.push_back(std::make_shared<RenderObject>(textObject->asGraphicsObject()));
    }

    auto boundingBoxLayerObject = this->boundingBoxLayerObject;
    if (boundingBoxLayerObject) {
        renderObjects.push_back(std::make_shared<RenderObject>(boundingBoxLayerObject->getPolygonObject()));
    }

    return renderObjects;
}
