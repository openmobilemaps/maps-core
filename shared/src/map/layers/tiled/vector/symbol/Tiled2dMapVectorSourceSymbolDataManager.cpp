/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorSourceSymbolDataManager.h"
#include "Tiled2dMapVectorLayer.h"
#include "Tiled2dMapVectorRasterSubLayerConfig.h"
#include "RenderPass.h"
#include "Matrix.h"
#include "StretchShaderInfo.h"

Tiled2dMapVectorSourceSymbolDataManager::Tiled2dMapVectorSourceSymbolDataManager(const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                                                                 const std::shared_ptr<VectorMapDescription> &mapDescription,
                                                                                 const std::string &source,
                                                                                 const std::shared_ptr<FontLoaderInterface> &fontLoader) :
Tiled2dMapVectorSourceDataManager(vectorLayer, mapDescription, source), fontLoader(fontLoader)
{
    for (const auto &layer: mapDescription->layers) {
        if (layer->getType() == VectorLayerType::symbol && layer->source == source) {
            layerDescriptions.insert({layer->identifier, std::static_pointer_cast<SymbolVectorLayerDescription>(layer)});
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::onAdded(const std::weak_ptr< ::MapInterface> &mapInterface) {
    Tiled2dMapVectorSourceDataManager::onAdded(mapInterface);
    textHelper.setMapInterface(mapInterface);

    auto mapInterfaceS = this->mapInterface.lock();
    auto graphicsFactory = mapInterfaceS ? mapInterfaceS->getGraphicsObjectFactory() : nullptr;
    auto shaderFactory = mapInterfaceS ? mapInterfaceS->getShaderFactory() : nullptr;
    const auto &context = mapInterfaceS ? mapInterfaceS->getRenderingContext() : nullptr;
    if (!graphicsFactory || !shaderFactory) {
        return;
    }

    instancedShader = shaderFactory->createTextInstancedShader();
    instancedObject = graphicsFactory->createTextInstanced(instancedShader->asShaderProgramInterface());
}

void Tiled2dMapVectorSourceSymbolDataManager::pause() {
    for (const auto &[tileInfo, tileSymbols]: tileSymbolMap) {
        for (const auto &[s, wrappers]: tileSymbols) {
            for (const auto &wrapper: wrappers) {
                if (wrapper->symbolObject) {
                    wrapper->symbolObject->asGraphicsObject()->clear();
                }
                if (wrapper->textObject) {
                    const auto textObject = wrapper->textObject->getTextObject();
                    if (textObject) {
                        textObject->asGraphicsObject()->clear();
                    }
                }
            }
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::resume() {
    auto mapInterface = this->mapInterface.lock();
    const auto &context = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!context) {
        return;
    }

    for (const auto &[tileInfo, tileSymbols]: tileSymbolMap) {
        for (const auto &[s, wrappers]: tileSymbols) {
            for (const auto &wrapper: wrappers) {
                if (wrapper->symbolObject && !wrapper->symbolObject->asGraphicsObject()->isReady()) {
                    wrapper->symbolObject->asGraphicsObject()->setup(context);
                    wrapper->symbolObject->loadTexture(mapInterface->getRenderingContext(), spriteTexture);
                }
                if (wrapper->textObject) {
                    const auto &textObject = wrapper->textObject->getTextObject();
                    if (textObject && !textObject->asGraphicsObject()->isReady()) {
                        textObject->asGraphicsObject()->setup(context);
                        auto fontResult = loadFont(wrapper->textInfo->getFont());
                        if (fontResult.imageData) {
                            textObject->loadTexture(context, fontResult.imageData);
                        }
                    }
                }
            }
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::setAlpha(float alpha) {
    this->alpha = alpha;
}

void Tiled2dMapVectorSourceSymbolDataManager::updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription,
                                                                     int32_t legacyIndex,
                                                                     bool needsTileReplace) {
    // TODO: update layer description of symbols
}

void Tiled2dMapVectorSourceSymbolDataManager::onVectorTilesUpdated(const std::string &sourceName,
                                                                   std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) {
    auto mapInterface = this->mapInterface.lock();
    auto graphicsFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
    auto coordinateConverterHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    auto shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
    if (!graphicsFactory || !shaderFactory) {
        return;
    }

    std::thread::id this_id = std::this_thread::get_id();

  //  LogError << "thread " << this_id <<= " sleeping...\n";

    // Just insert pointers here since we will only access the objects inside this method where we know that currentTileInfos is retained
    std::vector<const Tiled2dMapVectorTileInfo*> tilesToAdd;
    std::vector<const Tiled2dMapVectorTileInfo*> tilesToKeep;

    std::unordered_set<Tiled2dMapTileInfo> tilesToRemove;

    for (const auto &vectorTileInfo: currentTileInfos) {
        if (tileSymbolMap.count(vectorTileInfo.tileInfo) == 0) {
            tilesToAdd.push_back(&vectorTileInfo);
        } else {
            tilesToKeep.push_back(&vectorTileInfo);
        }
    }

    for (const auto &[tileInfo, _]: tileSymbolMap) {
        bool found = false;
        for (const auto &currentTile: currentTileInfos) {
            if (tileInfo == currentTile.tileInfo) {
                found = true;
                break;
            }
        }
        if (!found) {
            tilesToRemove.insert(tileInfo);
        }
    }

    if (tilesToAdd.empty() && tilesToRemove.empty()) {
        return;
    }

    std::vector<std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper>> toSetup;

    for (const auto &tile : tilesToAdd) {
        tileSymbolMap[tile->tileInfo] = {};

        for (const auto &[layerIdentifier, layer]: layerDescriptions) {

            if (!(layerDescriptions[layerIdentifier]->minZoom <= tile->tileInfo.zoomIdentifier &&
                  layerDescriptions[layerIdentifier]->maxZoom >= tile->tileInfo.zoomIdentifier)) {
                continue;
            }

            const auto &dataIt = tile->layerFeatureMaps->find(layer->sourceId);

            std::vector<std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper>> layerSymbols;

            if (dataIt != tile->layerFeatureMaps->end()) {
                for (const auto feature: *dataIt->second) {
                    // there is something in this layer to display
                    const auto &newSymbols = createSymbols(tile->tileInfo, layerIdentifier, feature);
                    if(!newSymbols.empty()) {
                        tileSymbolMap.at(tile->tileInfo)[layerIdentifier].insert(tileSymbolMap.at(tile->tileInfo)[layerIdentifier].end(), newSymbols.begin(), newSymbols.end());
                        toSetup.insert(toSetup.end(), newSymbols.begin(), newSymbols.end());
                    }
                }
            }
        }
    }

#ifdef __APPLE__
    setupTexts(toSetup, tilesToRemove);
#else
    auto selfActor = WeakActor(mailbox, weak_from_this());
    selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSourceSymbolDataManager::setupTexts, toSetup, tilesToRemove);
#endif


    mapInterface->invalidate();
}

std::vector<std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper>> Tiled2dMapVectorSourceSymbolDataManager::createSymbols(const Tiled2dMapTileInfo &tileInfo,
                                                                                                                          const LayerIndentifier &identifier,
                                                                                                                          const Tiled2dMapVectorTileInfo::FeatureTuple &feature) {
    const auto &[context, geometry] = feature;

    const auto evalContext = EvaluationContext(tileInfo.zoomIdentifier, context);

    const auto &description = layerDescriptions.at(identifier);


    std::vector<std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper>> symbols;

    if ((description->filter != nullptr && !description->filter->evaluateOr(evalContext, true))) {
        return symbols;
    }

    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!mapInterface || !camera) {
        return symbols;
    }

    std::vector<FormattedStringEntry> text = description->style.getTextField(evalContext);

    if(description->style.getTextTransform(evalContext) == TextTransform::UPPERCASE && text.size() > 2) {
        for (auto &e: text) {
            e.text = TextHelper::uppercase(e.text);
        }
    }

    std::string fullText;
    for (const auto &textEntry: text) {
        fullText += textEntry.text;
    }

    auto anchor = description->style.getTextAnchor(evalContext);
    const auto &justify = description->style.getTextJustify(evalContext);
    const auto &placement = description->style.getTextSymbolPlacement(evalContext);

    const auto &variableTextAnchor = description->style.getTextVariableAnchor(evalContext);

    if (!variableTextAnchor.empty()) {
        // TODO: evaluate all anchors correctly and choose best one
        // for now the first one is simply used
        anchor = *variableTextAnchor.begin();
    }

    const auto &fontList = description->style.getTextFont(evalContext);

    const double symbolSpacingPx = description->style.getSymbolSpacing(evalContext);
    const double tilePixelFactor = (0.0254 / camera->getScreenDensityPpi()) * tileInfo.zoomLevel;
    const double symbolSpacingMeters = symbolSpacingPx * tilePixelFactor;


    if(context.geomType != vtzero::GeomType::POINT) {

        // TODO: find meaningful way
        // to distribute along line
        double distance = 0;
        double totalDistance = 0;

        bool wasPlaced = false;

        std::vector<Coord> line = {};
        for (const auto &points: geometry.getPointCoordinates()) {
            for(auto& p : points) {
                line.push_back(p);
            }
        }

        for (const auto &points: geometry.getPointCoordinates()) {

            for (auto pointIt = points.begin(); pointIt != points.end(); pointIt++) {
                auto point = *pointIt;

                if (pointIt != points.begin()) {
                    auto last = std::prev(pointIt);
                    double addDistance = Vec2DHelper::distance(Vec2D(last->x, last->y), Vec2D(point.x, point.y));
                    distance += addDistance;
                    totalDistance += addDistance;
                }


                auto pos = getPositioning(pointIt, points);

                if (distance > symbolSpacingMeters && pos) {

                    auto position = pos->centerPosition;

                    tileTextPositionMap[tileInfo][fullText].push_back(position);

                    wasPlaced = true;

                    const auto symbol = createSymbolWrapper(tileInfo, identifier, description, {
                        context,
                        std::make_shared<SymbolInfo>(text,
                                                     position,
                                                     line,
                                                     Font(*fontList.begin()),
                                                     anchor,
                                                     pos->angle,
                                                     justify,
                                                     placement
                                                     )});
                    if (symbol) {
                        symbols.push_back(symbol);
                    }


                    distance = 0;
                }
            }
        }

        // if no label was placed, place it in the middle of the line
        if (!wasPlaced) {
            distance = 0;
            for (const auto &points: geometry.getPointCoordinates()) {

                for (auto pointIt = points.begin(); pointIt != points.end(); pointIt++) {
                    auto point = *pointIt;

                    if (pointIt != points.begin()) {
                        auto last = std::prev(pointIt);
                        double addDistance = Vec2DHelper::distance(Vec2D(last->x, last->y), Vec2D(point.x, point.y));
                        distance += addDistance;
                    }

                    auto pos = getPositioning(pointIt, points);

                    if (distance > (totalDistance / 2.0) && !wasPlaced && pos) {

                        auto position = pos->centerPosition;

                        wasPlaced = true;
                        const auto symbol = createSymbolWrapper(tileInfo, identifier, description, {
                            context,
                            std::make_shared<SymbolInfo>(text,
                                                         position,
                                                         std::nullopt,
                                                         Font(*fontList.begin()),
                                                         anchor,
                                                         pos->angle,
                                                         justify,
                                                         placement)
                        });
                        if (symbol) {
                            symbols.push_back(symbol);
                        }


                        distance = 0;
                    }
                }
            }
        }
    } else {
        for (const auto &p: geometry.getPointCoordinates()) {
            auto midP = p.begin() + p.size() / 2;
            std::optional<double> angle = std::nullopt;

            tileTextPositionMap[tileInfo][fullText].push_back(*midP);

            const auto symbol = createSymbolWrapper(tileInfo, identifier, description, {
                context,
                std::make_shared<SymbolInfo>(text,
                                             *midP,
                                             std::nullopt,
                                             Font(*fontList.begin()),
                                             anchor,
                                             angle,
                                             justify,
                                             placement)
            });
                              if (symbol) {
                                  symbols.push_back(symbol);
                              }

        }
    }
    return symbols;
}

std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper> Tiled2dMapVectorSourceSymbolDataManager::createSymbolWrapper(const Tiled2dMapTileInfo &tileInfo, const LayerIndentifier &identifier, const std::shared_ptr<SymbolVectorLayerDescription> &description, const std::tuple<const FeatureContext, std::shared_ptr<SymbolInfo>> &symbolInfo) {

    auto mapInterface = this->mapInterface.lock();
    auto objectFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;

    if (!objectFactory || !camera) {
        return nullptr;
    }

    std::vector<std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper>> textObjects;

    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(camera->getZoom());

    const auto &[context, symbol] = symbolInfo;

    auto fontResult = loadFont(symbol->getFont());
    if (fontResult.status != LoaderStatus::OK) {
        return nullptr;
    }

    const auto evalContext = EvaluationContext(zoomIdentifier, context);

    auto textOffset = description->style.getTextOffset(evalContext);

    const auto textRadialOffset = description->style.getTextRadialOffset(evalContext);
    // TODO: currently only shifting to top right
    textOffset.x += textRadialOffset;
    textOffset.y -= textRadialOffset;

    const auto letterSpacing = description->style.getTextLetterSpacing(evalContext);

    const auto textObject = textHelper.textLayerObject(symbol,
                                                       fontResult.fontData,
                                                       textOffset,
                                                       description->style.getTextLineHeight(evalContext),
                                                       letterSpacing,
                                                       description->style.getTextMaxWidth(evalContext),
                                                       description->style.getTextMaxAngle(evalContext),
                                                       description->style.getTextRotationAlignment(evalContext));

    if(textObject) {
        int64_t const symbolSortKey = description->style.getSymbolSortKey(evalContext);
        std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper> wrapper = std::make_shared<Tiled2dMapVectorSymbolFeatureWrapper>(context, symbol, textObject, symbolSortKey);

#ifdef DRAW_TEXT_BOUNDING_BOXES
        wrapper->boundingBoxShader = mapInterface->getShaderFactory()->createColorShader();
        wrapper->boundingBoxShader->setColor(0.0, 1.0, 0.0, 0.5);
        std::shared_ptr<Quad2dInterface> quadObject = mapInterface->getGraphicsObjectFactory()->createQuad(wrapper->boundingBoxShader->asShaderProgramInterface());
        quadObject->asGraphicsObject()->setup(mapInterface->getRenderingContext());

        wrapper->boundingBox = quadObject;
#endif

#ifdef DRAW_TEXT_LINES
        if(text->getLineCoordinates()) {
            auto list = ColorStateList(Color(0.0, 1.0, 0.0, 1.0), Color(0.0, 1.0, 0.0, 1.0));

            std::shared_ptr<LineInfoInterface> lineInfo = LineFactory::createLine("draw_text_lines", *text->getLineCoordinates(), LineStyle(list, list, 1.0, 1.0, SizeType::SCREEN_PIXEL, 3.0, {}, LineCapType::ROUND));

            auto objectFactory = mapInterface->getGraphicsObjectFactory();
            auto shaderFactory = mapInterface->getShaderFactory();

            auto shader = shaderFactory->createColorLineShader();
            auto lineGraphicsObject = objectFactory->createLine(shader->asShaderProgramInterface());

            auto lineObject = std::make_shared<Line2dLayerObject>(mapInterface->getCoordinateConverterHelper(), lineGraphicsObject, shader);

            lineObject->setStyle(lineInfo->getStyle());
            lineObject->setPositions(lineInfo->getCoordinates());
            lineObject->getLineObject()->setup(mapInterface->getRenderingContext());

            wrapper->lineObject = lineObject;
        }
#endif
        if (description->isInteractable(evalContext)) {
            interactableSet.insert({wrapper, identifier});
        }
        
        return wrapper;
    }
    return nullptr;
}


void Tiled2dMapVectorSourceSymbolDataManager::setupTexts(const std::vector<std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper>> toSetup, const std::unordered_set<Tiled2dMapTileInfo> tilesToRemove) {
    auto mapInterface = this->mapInterface.lock();
    if (!mapInterface) { return; }

    for (const auto &tile: tilesToRemove) {
        auto tileIt = tileSymbolMap.find(tile);
        if (tileIt != tileSymbolMap.end()) {
            for (const auto &[s, wrappers] : tileIt->second) {
                for (const auto &wrapper : wrappers) {
                    auto symbolObject = wrapper->symbolObject;
                    if (symbolObject) {
                        symbolObject->asGraphicsObject()->clear();
                    }
                    auto textObject = wrapper->textObject ? wrapper->textObject->getTextObject() : nullptr;
                    if (textObject) {
                        textObject->asGraphicsObject()->clear();
                    }
                    interactableSet.erase(wrapper);
                }
            }
            tileSymbolMap.erase(tileIt);
        }
    }

    for (const auto &symbol: toSetup) {
        const auto &textInfo = symbol->textInfo;

        const auto &textObject = symbol->textObject->getTextObject();

        if (textObject) {
            auto renderingContext = mapInterface->getRenderingContext();
            textObject->asGraphicsObject()->setup(renderingContext);

            auto fontResult = loadFont(textInfo->getFont());
            if(fontResult.imageData) {
                if (!instanceFontName) {
                    instanceFontName = fontResult.fontData->info.name;
                    instancedObject->loadTexture(renderingContext, fontResult.imageData);

                    instancedObject->setFrame(Quad2dD(Vec2D(-0.5, 0.5), Vec2D(0.5, 0.5), Vec2D(0.5, -0.5), Vec2D(-0.5, -0.5)));
                    instancedObject->asGraphicsObject()->setup(renderingContext);
                }
                textObject->loadTexture(renderingContext, fontResult.imageData);
            }
        }
    }
    
    pregenerateRenderPasses();
}

FontLoaderResult Tiled2dMapVectorSourceSymbolDataManager::loadFont(const Font &font) {
    if (fontLoaderResults.count(font.name) > 0) {
        return fontLoaderResults.at(font.name);
    } else {
        auto fontResult = fontLoader->loadFont(font);
        if (fontResult.status == LoaderStatus::OK && fontResult.fontData && fontResult.imageData) {
            fontLoaderResults.insert({font.name, fontResult});
        }
        return fontResult;
    }
}

std::optional<Tiled2dMapVectorSymbolSubLayerPositioningWrapper> Tiled2dMapVectorSourceSymbolDataManager::getPositioning(std::vector<::Coord>::const_iterator &iterator, const std::vector<::Coord> & collection) {

    double distance = 10;

    Vec2D curPoint(iterator->x, iterator->y);

    auto prev = iterator;
    if (prev == collection.begin()) { return std::nullopt; }

    while (Vec2DHelper::distance(Vec2D(prev->x, prev->y), curPoint) < distance) {
        prev = std::prev(prev);

        if (prev == collection.begin()) { return std::nullopt; }
    }

    auto next = iterator;
    if (next == collection.end()) { return std::nullopt; }

    while (Vec2DHelper::distance(Vec2D(next->x, next->y), curPoint) < distance) {
        next = std::next(next);

        if (next == collection.end()) { return std::nullopt; }
    }

    double angle = -atan2( next->y - prev->y, next->x - prev->x ) * ( 180.0 / M_PI );
    auto midpoint = Vec2DHelper::midpoint(Vec2D(prev->x, prev->y), Vec2D(next->x, next->y));
    return Tiled2dMapVectorSymbolSubLayerPositioningWrapper(angle, Coord(next->systemIdentifier, midpoint.x, midpoint.y, next->z));
}


Quad2dD Tiled2dMapVectorSourceSymbolDataManager::getProjectedFrame(const RectCoord &boundingBox, const float &padding, const std::vector<float> &modelMatrix) {

    auto topLeft = Vec2D(boundingBox.topLeft.x, boundingBox.topLeft.y);
    auto topRight = Vec2D(boundingBox.bottomRight.x, topLeft.y);
    auto bottomRight = Vec2D(boundingBox.bottomRight.x, boundingBox.bottomRight.y);
    auto bottomLeft = Vec2D(topLeft.x, bottomRight.y);

    topLeft.x -= padding;
    topLeft.y -= padding;

    topRight.x += padding;
    topRight.y -= padding;

    bottomLeft.x -= padding;
    bottomLeft.y += padding;

    bottomRight.x += padding;
    bottomRight.y += padding;

    Matrix::multiply(modelMatrix, std::vector<float>{(float)topLeft.x, (float)topLeft.y, 0.0, 1.0}, topLeftProj);
    Matrix::multiply(modelMatrix, std::vector<float>{(float)topRight.x, (float)topRight.y, 0.0, 1.0}, topRightProj);
    Matrix::multiply(modelMatrix, std::vector<float>{(float)bottomRight.x, (float)bottomRight.y, 0.0, 1.0}, bottomRightProj);
    Matrix::multiply(modelMatrix, std::vector<float>{(float)bottomLeft.x, (float)bottomLeft.y, 0.0, 1.0}, bottomLeftProj);

    return Quad2dD(Vec2D(topLeftProj[0], topLeftProj[1]), Vec2D(topRightProj[0], topRightProj[1]), Vec2D(bottomRightProj[0], bottomRightProj[1]), Vec2D(bottomLeftProj[0], bottomLeftProj[1]));
}


void Tiled2dMapVectorSourceSymbolDataManager::setSprites(std::shared_ptr<SpriteData> spriteData, std::shared_ptr<TextureHolderInterface> spriteTexture) {
    this->spriteData = spriteData;
    this->spriteTexture = spriteTexture;

    if (!tileSymbolMap.empty()) {
        auto selfActor = WeakActor(mailbox, weak_from_this());
        selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSourceSymbolDataManager::setupExistingSymbolWithSprite);
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::setupExistingSymbolWithSprite() {
    auto mapInterface = this->mapInterface.lock();

    if (!mapInterface) {
        return;
    }

    for (const auto &[tile, layers]: tileSymbolMap) {
        for (const auto &[layerIdentifier, objects]: layers) {
            for (const auto &wrapper: objects) {
                if (wrapper->symbolObject) {
                    wrapper->symbolObject->loadTexture(mapInterface->getRenderingContext(), spriteTexture);
                    wrapper->symbolGraphicsObject->setup(mapInterface->getRenderingContext());
                }
            }
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::collisionDetection(std::vector<std::string> layerIdentifiers, std::shared_ptr<std::vector<OBB2D>> placements) {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    auto converter = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    if (!camera || !converter || !renderingContext) {
        return;
    }

    double zoom = camera->getZoom();
    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(zoom);
    double rotation = -camera->getRotation();

    auto scaleFactor = camera->mapUnitsFromPixels(1.0);

    auto collisionDetectionLambda = [&](std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper> wrapper, const std::string &layerIdentifier){
        const auto evalContext = EvaluationContext(zoomIdentifier, wrapper->featureContext);

        const auto &object = wrapper->textObject;

        auto ref = object->getReferenceSize();

        const auto& refP = object->getReferencePoint();

        const auto &description = layerDescriptions.at(layerIdentifier);

        auto scale = scaleFactor * description->style.getTextSize(evalContext) / ref;

        wrapper->textObject->layout(scale);

        const bool hasText = object->getTextObject() != nullptr;

        const Coord renderCoord = converter->convertToRenderSystem(refP);

        double rotation = -camera->getRotation();

        bool collides = false;

        std::optional<Quad2dD> projectedTextQuad;
        std::optional<Quad2dD> projectedSymbolQuad;

        if (hasText) {
            Matrix::setIdentityM(wrapper->modelMatrix, 0);
            Matrix::translateM(wrapper->modelMatrix, 0, renderCoord.x, renderCoord.y, renderCoord.z);

            rotation += description->style.getTextRotate(evalContext);

            if(true) {
                if (wrapper->textInfo->angle) {
                    double rotateSum = fmod(rotation - (*wrapper->textInfo->angle) + 720.0, 360);
                    if (rotateSum > 270 || rotateSum < 90) {
                        rotation = *wrapper->textInfo->angle;
                    } else {
                        rotation = *wrapper->textInfo->angle + 180;
                    }
                }
                Matrix::rotateM(wrapper->modelMatrix, 0.0, rotation, 0.0, 0.0, 1.0);
            }

            Matrix::translateM(wrapper->modelMatrix, 0, -renderCoord.x, -renderCoord.y, -renderCoord.z);

            const float padding = description->style.getTextPadding(evalContext);
            projectedTextQuad = getProjectedFrame(object->boundingBox, padding, wrapper->modelMatrix);
        }

        if (spriteData && spriteTexture) {
            auto iconImage = description->style.getIconImage(evalContext);
            if (iconImage != "") {
                Matrix::setIdentityM(wrapper->iconModelMatrix, 0);
                Matrix::translateM(wrapper->iconModelMatrix, 0, renderCoord.x, renderCoord.y, renderCoord.z);

                auto iconSize = description->style.getIconSize(evalContext) * scaleFactor;

                Matrix::scaleM(wrapper->iconModelMatrix, 0, iconSize, iconSize, 1.0);
                Matrix::rotateM(wrapper->iconModelMatrix, 0.0, rotation, 0.0, 0.0, 1.0);
                Matrix::translateM(wrapper->iconModelMatrix, 0, -renderCoord.x, -renderCoord.y, -renderCoord.z);

                const auto &spriteInfo = spriteData->sprites.at(iconImage);

                if (!wrapper->symbolShader) {
                    wrapper->symbolShader = mapInterface->getShaderFactory()->createStretchShader();
                }

                if (!wrapper->symbolObject) {
                    wrapper->symbolObject = mapInterface->getGraphicsObjectFactory()->createQuad(wrapper->symbolShader->asShaderProgramInterface());
                    wrapper->symbolGraphicsObject = wrapper->symbolObject->asGraphicsObject();
                }

                Coord renderPos = converter->convertToRenderSystem(wrapper->textInfo->getCoordinate());

                const double densityOffset = (camera->getScreenDensityPpi() / 160.0) / spriteInfo.pixelRatio;

                auto iconOffset = description->style.getIconOffset(evalContext);
                renderPos.y -= iconOffset.y;
                renderPos.x += iconOffset.x;

                const auto textureWidth = (double) spriteTexture->getImageWidth();
                const auto textureHeight = (double) spriteTexture->getImageHeight();

                auto spriteWidth = spriteInfo.width * densityOffset;
                auto spriteHeight = spriteInfo.height * densityOffset;

                auto padding = description->style.getIconTextFitPadding(evalContext);

                const float topPadding = padding[0];
                const float rightPadding = padding[1];
                const float bottomPadding = padding[2];
                const float leftPadding = padding[3];

                const auto textWidth = (leftPadding + rightPadding) * scaleFactor + (object->boundingBox.bottomRight.x - object->boundingBox.topLeft.x) / scaleFactor;
                const auto textHeight = (topPadding + bottomPadding) * scaleFactor + (object->boundingBox.bottomRight.y - object->boundingBox.topLeft.y) / scaleFactor;

                auto scaleX = std::max(1.0, textWidth / spriteWidth);
                auto scaleY = std::max(1.0, textHeight / spriteHeight);

                auto textFit = description->style.getIconTextFit(evalContext);
                if (textFit == IconTextFit::NONE) {
                    scaleX = 1;
                    scaleY = 1;
                } else if (textFit == IconTextFit::WIDTH) {
                    scaleY = 1;
                } else if (textFit == IconTextFit::HEIGHT) {
                    scaleX = 1;
                }

                spriteWidth *= scaleX;
                spriteHeight *= scaleY;

                auto x = renderPos.x - spriteWidth / 2;
                auto y = renderPos.y + spriteHeight / 2;
                auto xw = renderPos.x + spriteWidth / 2;
                auto yh = renderPos.y - spriteHeight / 2;

                Quad2dD quad = Quad2dD(Vec2D(x, yh), Vec2D(xw, yh), Vec2D(xw, y), Vec2D(x, y));

                const double iconPadding = description->style.getIconPadding(evalContext);
                projectedSymbolQuad = getProjectedFrame(RectCoord(Coord(renderPos.systemIdentifier, quad.topLeft.x, quad.topLeft.y, renderPos.z), Coord(renderPos.systemIdentifier, quad.bottomRight.x, quad.bottomRight.y, renderPos.z)), iconPadding, wrapper->iconModelMatrix);

                auto symbolGraphicsObject = wrapper->symbolGraphicsObject;
                if (spriteTexture && !symbolGraphicsObject->isReady()) {
                    symbolGraphicsObject->setup(renderingContext);
                    wrapper->symbolObject->loadTexture(renderingContext, spriteTexture);
                }
            }
        }

        std::optional<Quad2dD> combinedQuad;
        if (projectedTextQuad && projectedSymbolQuad) {
            combinedQuad = Quad2dD(Vec2D(std::min(projectedTextQuad->topLeft.x, projectedSymbolQuad->topLeft.x), std::min(projectedTextQuad->topLeft.y, projectedSymbolQuad->topLeft.y)),
                                   Vec2D(std::max(projectedTextQuad->topRight.x, projectedSymbolQuad->topRight.x), std::min(projectedTextQuad->topRight.y, projectedSymbolQuad->topRight.y)),
                                   Vec2D(std::max(projectedTextQuad->bottomRight.x, projectedSymbolQuad->bottomRight.x), std::max(projectedTextQuad->bottomRight.y, projectedSymbolQuad->bottomRight.y)),
                                   Vec2D(std::min(projectedTextQuad->bottomLeft.x, projectedSymbolQuad->bottomLeft.x), std::max(projectedTextQuad->bottomLeft.y, projectedSymbolQuad->bottomLeft.y)));
        } else if (projectedTextQuad) {
            combinedQuad = projectedTextQuad;
        } else if (projectedSymbolQuad) {
            combinedQuad = projectedSymbolQuad;
        }

        if (!combinedQuad) {
            // The symbol doesnt have a text nor a icon
            //assert(false);
            collides = true;
        }

        wrapper->orientedBoundingBox = OBB2D(*combinedQuad);

        for ( const auto &otherB: *placements ) {
            if (otherB.overlaps(wrapper->orientedBoundingBox)) {
                collides = true;
                break;
            }
        }
        if (!collides) {
            placements->push_back(wrapper->orientedBoundingBox);
        }
        wrapper->setCollisionAt(zoom, collides);
    };

    for (const auto layerIdentifier: layerIdentifiers) {
        const auto &description = layerDescriptions.at(layerIdentifier);
        if (!(description->minZoom <= zoomIdentifier &&
              description->maxZoom >= zoomIdentifier)) {
            continue;
        }
        for (const auto &[tile, layers]: tileSymbolMap) {
            const auto objectsIt = layers.find(layerIdentifier);
            if (objectsIt != layers.end()) {
                for (auto &wrapper: objectsIt->second) {
                    collisionDetectionLambda(wrapper, layerIdentifier);
                }
            }
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::update() {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    auto converter = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    if (!camera || !converter || !renderingContext) {
        return;
    }

    const double zoom = camera->getZoom();
    const double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(zoom);
    const double rotation = -camera->getRotation();

    const auto scaleFactor = camera->mapUnitsFromPixels(1.0);

    std::vector<float> positions;
    std::vector<float> scales;
    std::vector<float> textureCoordinates;
    std::vector<float> rotations;
    std::vector<uint16_t> stylesIndex;
    std::vector<float> styles;

    for (const auto &[tile, layers]: tileSymbolMap) {
        for (const auto &[layerIdentifier, objects]: layers) {
            const auto &description = layerDescriptions.at(layerIdentifier);
            if (!(description->minZoom <= zoomIdentifier &&
                  description->maxZoom >= zoomIdentifier)) {
                continue;
            }
            for (auto &wrapper: objects) {

                const auto evalContext = EvaluationContext(zoomIdentifier, wrapper->featureContext);

                auto &object = wrapper->textObject;
                wrapper->collides = wrapper->hasCollision(zoom);

                if(wrapper->collides) {
                    continue;
                }

                const auto ref = object->getReferenceSize();

                const auto &refP = object->getReferencePoint();

                const auto scale = scaleFactor * description->style.getTextSize(evalContext) / ref;

                object->update(scale);

                const bool hasText = object->getTextObject() != nullptr;

                const Coord renderCoord = converter->convertToRenderSystem(refP);

                double rotation = -camera->getRotation();

                std::optional<Quad2dD> projectedTextQuad;
                std::optional<Quad2dD> projectedSymbolQuad;

                if (hasText) {
                    Matrix::setIdentityM(wrapper->modelMatrix, 0);
                    Matrix::translateM(wrapper->modelMatrix, 0, renderCoord.x, renderCoord.y, renderCoord.z);

                    rotation += description->style.getTextRotate(evalContext);

                    if(wrapper->textInfo->getSymbolPlacement() == TextSymbolPlacement::POINT) {
                        if (wrapper->textInfo->angle) {
                            double rotateSum = fmod(rotation - (*wrapper->textInfo->angle) + 720.0, 360);
                            if (rotateSum > 270 || rotateSum < 90) {
                                rotation = *wrapper->textInfo->angle;
                            } else {
                                rotation = *wrapper->textInfo->angle + 180;
                            }
                        }

                        Matrix::rotateM(wrapper->modelMatrix, 0.0, rotation, 0.0, 0.0, 1.0);
                    }

                    Matrix::translateM(wrapper->modelMatrix, 0, -renderCoord.x, -renderCoord.y, -renderCoord.z);

                    const float padding = description->style.getTextPadding(evalContext);
                    projectedTextQuad = getProjectedFrame(object->boundingBox, padding, wrapper->modelMatrix);


                    if (object->fontData.info.name == instanceFontName){
                        positions.insert(positions.begin(), object->positions.begin(), object->positions.end());
                        scales.insert(scales.begin(), object->scales.begin(), object->scales.end());
                        textureCoordinates.insert(textureCoordinates.begin(), object->textureCoordinates.begin(), object->textureCoordinates.end());
                        rotations.insert(rotations.begin(), object->rotations.begin(), object->rotations.end());
                        uint16_t styleIndex = (uint16_t)(styles.size() / 8.0);
                        for(int i = 0; i != object->rotations.size(); i++) {
                            stylesIndex.push_back(styleIndex);
                        }
                        auto opacity = description->style.getTextOpacity(evalContext) * alpha;
                        auto textColor = description->style.getTextColor(evalContext);
                        auto haloColor = description->style.getTextHaloColor(evalContext);

                        styles.push_back(textColor.r); //R
                        styles.push_back(textColor.g); //G
                        styles.push_back(textColor.b); //B
                        styles.push_back(textColor.a * opacity); //A

                        styles.push_back(haloColor.r); //R
                        styles.push_back(haloColor.g); //G
                        styles.push_back(haloColor.b); //B
                        styles.push_back(haloColor.a * opacity); //A
                    }
                }

                if (spriteData && spriteTexture) {
                    auto iconImage = description->style.getIconImage(evalContext);
                    if (iconImage != "") {
                        Matrix::setIdentityM(wrapper->iconModelMatrix, 0);
                        Matrix::translateM(wrapper->iconModelMatrix, 0, renderCoord.x, renderCoord.y, renderCoord.z);

                        auto iconSize = description->style.getIconSize(evalContext) * scaleFactor;

                        Matrix::scaleM(wrapper->iconModelMatrix, 0, iconSize, iconSize, 1.0);
                        Matrix::rotateM(wrapper->iconModelMatrix, 0.0, rotation, 0.0, 0.0, 1.0);
                        Matrix::translateM(wrapper->iconModelMatrix, 0, -renderCoord.x, -renderCoord.y, -renderCoord.z);

                        const auto &spriteInfo = spriteData->sprites.at(iconImage);

                        if (!wrapper->symbolShader) {
                            wrapper->symbolShader = mapInterface->getShaderFactory()->createStretchShader();
                        }

                        if (!wrapper->symbolObject) {
                            wrapper->symbolObject = mapInterface->getGraphicsObjectFactory()->createQuad(wrapper->symbolShader->asShaderProgramInterface());
                            wrapper->symbolGraphicsObject = wrapper->symbolObject->asGraphicsObject();
                        }

                        Coord renderPos = converter->convertToRenderSystem(wrapper->textInfo->getCoordinate());

                        const double densityOffset = (camera->getScreenDensityPpi() / 160.0) / spriteInfo.pixelRatio;

                        auto iconOffset = description->style.getIconOffset(evalContext);
                        renderPos.y -= iconOffset.y;
                        renderPos.x += iconOffset.x;

                        const auto textureWidth = (double) spriteTexture->getImageWidth();
                        const auto textureHeight = (double) spriteTexture->getImageHeight();
#ifdef __ANDROID__
                        const auto textureWidthFactor = (double) spriteTexture->getImageWidth() / spriteTexture->getTextureWidth();
                        const auto textureHeightFactor = (double) spriteTexture->getImageHeight() / spriteTexture->getTextureHeight();
#endif

                        auto spriteWidth = spriteInfo.width * densityOffset;
                        auto spriteHeight = spriteInfo.height * densityOffset;

                        auto padding = description->style.getIconTextFitPadding(evalContext);

                        const float topPadding = padding[0] * spriteInfo.pixelRatio * densityOffset;
                        const float rightPadding = padding[1] * spriteInfo.pixelRatio * densityOffset;
                        const float bottomPadding = padding[2] * spriteInfo.pixelRatio * densityOffset;
                        const float leftPadding = padding[3] * spriteInfo.pixelRatio * densityOffset;

                        auto textWidth = (object->boundingBox.bottomRight.x - object->boundingBox.topLeft.x) / scaleFactor;
                        textWidth += (leftPadding + rightPadding);

                        auto textHeight = (object->boundingBox.bottomRight.y - object->boundingBox.topLeft.y) / scaleFactor;
                        textHeight+= (topPadding + bottomPadding);

                        auto scaleX = std::max(1.0, textWidth / spriteWidth);
                        auto scaleY = std::max(1.0, textHeight / spriteHeight);

                        auto textFit = description->style.getIconTextFit(evalContext);

                        if (textFit == IconTextFit::WIDTH || textFit == IconTextFit::BOTH) {
                            spriteWidth *= scaleX;
                        }
                        if (textFit == IconTextFit::HEIGHT || textFit == IconTextFit::BOTH) {
                            spriteHeight *= scaleY;
                        }

                        auto x = renderPos.x - (leftPadding + (spriteWidth - leftPadding - rightPadding) / 2);
                        auto y = renderPos.y + (topPadding + (spriteHeight - topPadding - bottomPadding) / 2);
                        auto xw = x + spriteWidth;
                        auto yh = y - spriteHeight;

                        Quad2dD quad = Quad2dD(Vec2D(x, yh), Vec2D(xw, yh), Vec2D(xw, y), Vec2D(x, y));

                        if (object->getCurrentSymbolName() != iconImage && spriteData->sprites.count(iconImage) != 0) {
                            auto uvRect = RectD( ((double) spriteInfo.x) / textureWidth,
                                                ((double) spriteInfo.y) / textureHeight,
                                                ((double) spriteInfo.width) / textureWidth,
                                                ((double) spriteInfo.height) / textureHeight);

                            auto stretchinfo = StretchShaderInfo(scaleX, 1, 1, 1, 1, scaleY, 1, 1, 1, 1, uvRect);
#ifdef __ANDROID__
                            stretchinfo.uv.y -= stretchinfo.uv.height; // OpenGL uv coordinates are bottom to top
                            stretchinfo.uv.width *= textureWidthFactor; // Android textures are scaled to the size of a power of 2
                            stretchinfo.uv.height *= textureHeightFactor;
#endif


                            if (spriteInfo.stretchX.size() >= 1) {
                                auto [begin, end] = spriteInfo.stretchX[0];
                                stretchinfo.stretchX0Begin = (begin / spriteInfo.width);
                                stretchinfo.stretchX0End = (end / spriteInfo.width) ;

                                if (spriteInfo.stretchX.size() >= 2) {
                                    auto [begin, end] = spriteInfo.stretchX[1];
                                    stretchinfo.stretchX1Begin = (begin / spriteInfo.width);
                                    stretchinfo.stretchX1End = (end / spriteInfo.width);
                                } else {
                                    stretchinfo.stretchX1Begin = stretchinfo.stretchX0End;
                                    stretchinfo.stretchX1End = stretchinfo.stretchX0End;
                                }

                                const float sumStretchedX = (stretchinfo.stretchX0End - stretchinfo.stretchX0Begin) + (stretchinfo.stretchX1End - stretchinfo.stretchX1Begin);
                                const float sumUnstretchedX = 1.0f - sumStretchedX;
                                const float scaleStretchX = (scaleX - sumUnstretchedX) / sumStretchedX;

                                const float sX0 = stretchinfo.stretchX0Begin / scaleX;
                                const float eX0 = sX0 + scaleStretchX / scaleX * (stretchinfo.stretchX0End - stretchinfo.stretchX0Begin);
                                const float sX1 = eX0 + (stretchinfo.stretchX1Begin - stretchinfo.stretchX0End) / scaleX;
                                const float eX1 = sX1 + scaleStretchX / scaleX * (stretchinfo.stretchX1End - stretchinfo.stretchX1Begin);
                                stretchinfo.stretchX0Begin = sX0;
                                stretchinfo.stretchX0End = eX0;
                                stretchinfo.stretchX1Begin = sX1;
                                stretchinfo.stretchX1End = eX1;
                            }

                            if (spriteInfo.stretchY.size() >= 1) {
                                auto [begin, end] = spriteInfo.stretchY[0];
                                stretchinfo.stretchY0Begin = (begin / spriteInfo.height);
                                stretchinfo.stretchY0End = (end / spriteInfo.height);

                                if (spriteInfo.stretchY.size() >= 2) {
                                    auto [begin, end] = spriteInfo.stretchY[1];
                                    stretchinfo.stretchY1Begin = (begin / spriteInfo.height);
                                    stretchinfo.stretchY1End = (end / spriteInfo.height);
                                } else {
                                    stretchinfo.stretchY1Begin = stretchinfo.stretchY0End;
                                    stretchinfo.stretchY1End = stretchinfo.stretchY0End;
                                }

                                const float sumStretchedY = (stretchinfo.stretchY0End - stretchinfo.stretchY0Begin) + (stretchinfo.stretchY1End - stretchinfo.stretchY1Begin);
                                const float sumUnstretchedY = 1.0f - sumStretchedY;
                                const float scaleStretchY = (scaleY - sumUnstretchedY) / sumStretchedY;

                                const float sY0 = stretchinfo.stretchY0Begin / scaleY;
                                const float eY0 = sY0 + scaleStretchY / scaleY * (stretchinfo.stretchY0End - stretchinfo.stretchY0Begin);
                                const float sY1 = eY0 + (stretchinfo.stretchY1Begin - stretchinfo.stretchY0End) / scaleY;
                                const float eY1 = sY1 + scaleStretchY / scaleY * (stretchinfo.stretchY1End - stretchinfo.stretchY1Begin);
                                stretchinfo.stretchY0Begin = sY0;
                                stretchinfo.stretchY0End = eY0;
                                stretchinfo.stretchY1Begin = sY1;
                                stretchinfo.stretchY1End = eY1;
                            }
                            wrapper->symbolShader->updateStretchInfo(stretchinfo);

                            object->setCurrentSymbolName(iconImage);
                            wrapper->symbolObject->setFrame(quad, uvRect);
                        }


                        const double iconPadding = description->style.getIconPadding(evalContext);
                        projectedSymbolQuad = getProjectedFrame(RectCoord(Coord(renderPos.systemIdentifier, quad.topLeft.x, quad.topLeft.y, renderPos.z), Coord(renderPos.systemIdentifier, quad.bottomRight.x, quad.bottomRight.y, renderPos.z)), iconPadding, wrapper->iconModelMatrix);

                        const auto symbolGraphicsObject = wrapper->symbolGraphicsObject;
                        if (spriteTexture && !symbolGraphicsObject->isReady()) {
                            symbolGraphicsObject->setup(renderingContext);
                            wrapper->symbolObject->loadTexture(renderingContext, spriteTexture);
                        }
                    }
                }

                std::optional<Quad2dD> combinedQuad;
                if (projectedTextQuad && projectedSymbolQuad) {
                    combinedQuad = Quad2dD(Vec2D(std::min(projectedTextQuad->topLeft.x, projectedSymbolQuad->topLeft.x), std::min(projectedTextQuad->topLeft.y, projectedSymbolQuad->topLeft.y)),
                                           Vec2D(std::max(projectedTextQuad->topRight.x, projectedSymbolQuad->topRight.x), std::min(projectedTextQuad->topRight.y, projectedSymbolQuad->topRight.y)),
                                           Vec2D(std::max(projectedTextQuad->bottomRight.x, projectedSymbolQuad->bottomRight.x), std::max(projectedTextQuad->bottomRight.y, projectedSymbolQuad->bottomRight.y)),
                                           Vec2D(std::min(projectedTextQuad->bottomLeft.x, projectedSymbolQuad->bottomLeft.x), std::max(projectedTextQuad->bottomLeft.y, projectedSymbolQuad->bottomLeft.y)));
                } else if (projectedTextQuad) {
                    combinedQuad = projectedTextQuad;
                } else if (projectedSymbolQuad) {
                    combinedQuad = projectedSymbolQuad;
                }

                if (!combinedQuad) {
                    // The symbol doesnt have a text nor a icon
                    assert(false);
                    // TODO
                    //wrapper->collides = true;
                }

                wrapper->orientedBoundingBox = OBB2D(*combinedQuad);

                #ifdef DRAW_TEXT_BOUNDING_BOXES
                    wrapper->boundingBox->setFrame(*combinedQuad, RectD(0, 0, 1, 1));
                #endif

                if (!wrapper->collides) {
                    const auto &shader = object->getShader();
                    if (shader) {
                        object->getShader()->setOpacity(description->style.getTextOpacity(evalContext) * alpha);
                        object->getShader()->setColor(description->style.getTextColor(evalContext));
                        object->getShader()->setHaloColor(description->style.getTextHaloColor(evalContext));
                    }
                    if (wrapper->symbolShader) {
                        wrapper->symbolShader->updateAlpha(description->style.getIconOpacity(evalContext) * alpha);
                    }
#ifdef DRAW_TEXT_BOUNDING_BOXES
                    wrapper->boundingBoxShader->setColor(0.0, 1.0, 0.0, 0.5);
#endif
                }

                #ifdef DRAW_COLLIDED_TEXT_BOUNDING_BOXES
                    else {
                        if (object->getShader()) {
                            object->getShader()->setColor(Color(1.0, 0.0, 0.0, 1.0));
                        }
#ifdef DRAW_TEXT_BOUNDING_BOXES
                        wrapper->boundingBoxShader->setColor(1.0, 0.0, 0.0, 0.5);
#endif
                    }
                #endif

                #ifdef __ANDROID__
                if (hasText) {
                    const auto &graphicsObject = object->getTextObject()->asGraphicsObject();
                    graphicsObject->setup(renderingContext);
                }
                if (wrapper->symbolObject) {
                    const auto &graphicsObject = wrapper->symbolObject->asGraphicsObject();
                    graphicsObject->setup(renderingContext);
                }
                #endif
            }
        }
    }

    if(rotations.size() != 0) {
        instancedObject->setInstanceCount(rotations.size());
        instancedObject->setPositions(SharedBytes((int64_t)positions.data(), (int32_t)rotations.size(), 2 * (int32_t)sizeof(float)));
        instancedObject->setTexureCoordinates(SharedBytes((int64_t)textureCoordinates.data(), (int32_t)rotations.size(), 4 * (int32_t)sizeof(float)));
        instancedObject->setScales(SharedBytes((int64_t)scales.data(), (int32_t)rotations.size(), 2 * (int32_t)sizeof(float)));
        instancedObject->setRotations(SharedBytes((int64_t)rotations.data(), (int32_t)rotations.size(), 1 * (int32_t)sizeof(float)));

        instancedObject->setStyleIndices(SharedBytes((int64_t)stylesIndex.data(), (int32_t)stylesIndex.size(), 1 * (int32_t)sizeof(uint16_t)));
        instancedObject->setStyles(SharedBytes((int64_t)styles.data(), (int32_t)styles.size() / 8, 8 * (int32_t)sizeof(float)));
 }

    pregenerateRenderPasses();
}

void Tiled2dMapVectorSourceSymbolDataManager::pregenerateRenderPasses() {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!camera) {
        return ;
    }

    std::vector<std::tuple<int32_t, std::shared_ptr<RenderPassInterface>>> renderPasses;

    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(camera->getZoom());

    {
        std::vector<std::shared_ptr< ::RenderObjectInterface>> renderObjects;
        renderObjects.push_back(std::make_shared<RenderObject>(instancedObject->asGraphicsObject()));
        renderPasses.emplace_back(99, std::make_shared<RenderPass>(RenderPassConfig(99), renderObjects));
    }

    for (const auto &[tile, layers]: tileSymbolMap) {
        for (const auto &[layerIdentifier, objects]: layers) {

            const auto &description = layerDescriptions.at(layerIdentifier);
            if (!(description->minZoom <= zoomIdentifier &&
                  description->maxZoom >= zoomIdentifier)) {
                continue;
            }

            const auto index = layerNameIndexMap.at(layerIdentifier);

            std::vector<std::shared_ptr< ::RenderObjectInterface>> renderObjects;
            for (const auto &wrapper: objects) {
                if (
#ifdef DRAW_COLLIDED_TEXT_BOUNDING_BOXES
                    true
#else
                    !wrapper->collides
#endif
                    ) {

                    if (wrapper->symbolGraphicsObject) {
                        renderObjects.push_back(std::make_shared<RenderObject>(wrapper->symbolGraphicsObject, wrapper->iconModelMatrix));
                    }

                    const auto & textObject = wrapper->textObject->getTextObject();
                    if (textObject) {
//                        renderObjects.push_back(std::make_shared<RenderObject>(textObject->asGraphicsObject(), wrapper->modelMatrix));
#ifdef DRAW_TEXT_BOUNDING_BOXES
                    renderObjects.push_back(std::make_shared<RenderObject>(wrapper->boundingBox->asGraphicsObject(), wrapper->modelMatrix));
#endif
                    } else {
#ifdef DRAW_TEXT_BOUNDING_BOXES
                    renderObjects.push_back(std::make_shared<RenderObject>(wrapper->boundingBox->asGraphicsObject(), wrapper->iconModelMatrix));
#endif
                    }
                }
            }
            renderPasses.emplace_back(index, std::make_shared<RenderPass>(RenderPassConfig(0), renderObjects));
        }
    }

    vectorLayer.syncAccess([source = this->source, &renderPasses](const auto &layer){
        if(auto strong = layer.lock()) {
            strong->onRenderPassUpdate(source, true, renderPasses);
        }
    });
}

bool Tiled2dMapVectorSourceSymbolDataManager::onClickUnconfirmed(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) {
    return false;
}

bool Tiled2dMapVectorSourceSymbolDataManager::onClickConfirmed(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface.lock() : nullptr;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto conversionHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    
    if (!camera || !conversionHelper || !selectionDelegate) {
        return false;
    }

    Coord clickCoords = camera->coordFromScreenPosition(posScreen);
    Coord clickCoordsRenderCoord = conversionHelper->convertToRenderSystem(clickCoords);

    double clickPadding = camera->mapUnitsFromPixels(16);

    OBB2D tinyClickBox(Quad2dD(Vec2D(clickCoordsRenderCoord.x - clickPadding, clickCoordsRenderCoord.y - clickPadding),
                               Vec2D(clickCoordsRenderCoord.x + clickPadding, clickCoordsRenderCoord.y - clickPadding),
                               Vec2D(clickCoordsRenderCoord.x + clickPadding, clickCoordsRenderCoord.y + clickPadding),
                               Vec2D(clickCoordsRenderCoord.x - clickPadding, clickCoordsRenderCoord.y + clickPadding)));

    for(auto const &[wrapper, layerIndentifier]: interactableSet) {
        if (wrapper->collides) { continue; }
        if (wrapper->orientedBoundingBox.overlaps(tinyClickBox)) {
            selectionDelegate->didSelectFeature(wrapper->featureContext.getFeatureInfo(), layerIndentifier, wrapper->textInfo->getCoordinate());
            return true;
        }
    }
    return false;

}

bool Tiled2dMapVectorSourceSymbolDataManager::onDoubleClick(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) {
    return false;
}

bool Tiled2dMapVectorSourceSymbolDataManager::onLongPress(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) {
    return false;
}

bool Tiled2dMapVectorSourceSymbolDataManager::onTwoFingerClick(const std::unordered_set<std::string> &layers, const Vec2F &posScreen1,
                                                          const Vec2F &posScreen2) {
    return false;
}

void Tiled2dMapVectorSourceSymbolDataManager::clearTouch() {

}
