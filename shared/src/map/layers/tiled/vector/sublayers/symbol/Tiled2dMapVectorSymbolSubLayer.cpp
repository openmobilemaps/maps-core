/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorSymbolSubLayer.h"
#include <map>
#include "MapInterface.h"
#include "RenderObject.h"
#include "RenderPass.h"
#include "LambdaTask.h"
#include "LineGroupShaderInterface.h"
#include <algorithm>
#include "MapCamera2dInterface.h"
#include "Tiled2dMapVectorRasterSubLayerConfig.h"
#include "VectorTilePointHandler.h"
#include "TextDescription.h"
#include "FontLoaderResult.h"
#include "TextHelper.h"
#include "OBB2D.h"
#include "Vec2D.h"
#include "Matrix.h"
#include "json.h"
#include "ColorShaderInterface.h"
#include "LineFactory.h"

Tiled2dMapVectorSymbolSubLayer::Tiled2dMapVectorSymbolSubLayer(const std::shared_ptr<FontLoaderInterface> &fontLoader,
                                                               const std::shared_ptr<SymbolVectorLayerDescription> &description)
: fontLoader(fontLoader),
  description(description)
{}

void Tiled2dMapVectorSymbolSubLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface, int32_t layerIndex) {
    Tiled2dMapVectorSubLayer::onAdded(mapInterface, layerIndex);
}

void Tiled2dMapVectorSymbolSubLayer::onRemoved() {
    Tiled2dMapVectorSubLayer::onRemoved();
}

void Tiled2dMapVectorSymbolSubLayer::pause() {
    Tiled2dMapVectorSubLayer::pause();
    {
        std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(tilesInSetupMutex, symbolMutex);

        for (const auto &[tile, wrapperVector] : tileTextMap) {
            tilesInSetup.insert(tile);
            for (auto const &wrapper: wrapperVector) {
                const auto &object = wrapper->textObject->getTextObject();
                if (object && object->asGraphicsObject()->isReady()) {
                    object->asGraphicsObject()->clear();
                }
                if (wrapper->symbolObject && wrapper->symbolGraphicsObject->isReady()) {
                    wrapper->symbolGraphicsObject->clear();
                }
            }
        }
    }

    {
        std::lock_guard<std::recursive_mutex> overlayLock(fontResultsMutex);
        fontLoaderResults.clear();
    }
}

void Tiled2dMapVectorSymbolSubLayer::resume() {
    Tiled2dMapVectorSubLayer::resume();
    std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(tilesInSetupMutex, symbolMutex);

    const auto &context = mapInterface->getRenderingContext();
    for (const auto &[tile, wrapperVector] : tileTextMap) {
        for (auto const &wrapper: wrapperVector) {
            const auto &object = wrapper->textObject->getTextObject();
            if (object) {
                const auto &fontInfo = wrapper->textInfo->getFont();

                auto fontResult = loadFont(fontInfo);

                if (!object->asGraphicsObject()->isReady()) {
                    object->asGraphicsObject()->setup(context);
                }

                if(fontResult.imageData) {
                    object->loadTexture(context, fontResult.imageData);
                }
            }

            auto const &symbolObject = wrapper->symbolObject;
            auto const &symbolGraphicsObject = wrapper->symbolGraphicsObject;
            if (symbolGraphicsObject) {
                if (!symbolGraphicsObject->isReady()) {
                    symbolGraphicsObject->setup(context);
                }
                if (spriteTexture) {
                    symbolObject->loadTexture(context, spriteTexture);
                }
            }
        }
        tilesInSetup.erase(tile);
        if (auto delegate = readyDelegate.lock()) {
            // delegate->tileIsReady(tile, "", std::vector<std::shared_ptr<RenderObjectInterface>>{});
        }
    }
}

void Tiled2dMapVectorSymbolSubLayer::hide() {
    Tiled2dMapVectorSubLayer::hide();
}

void Tiled2dMapVectorSymbolSubLayer::show() {
    Tiled2dMapVectorSubLayer::show();
}


std::optional<Tiled2dMapVectorSymbolSubLayerPositioningWrapper> Tiled2dMapVectorSymbolSubLayer::getPositioning(std::vector<::Coord>::const_iterator &iterator, const std::vector<::Coord> & collection) {

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

void
Tiled2dMapVectorSymbolSubLayer::updateTileData(const Tiled2dMapTileInfo &tileInfo, const std::shared_ptr<MaskingObjectInterface> &tileMask, const std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>> &layerFeatures) {
    Tiled2dMapVectorSubLayer::updateTileData(tileInfo, tileMask, layerFeatures);
    auto mapInterface = this->mapInterface;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!camera) {
        return;
    }

    std::vector<std::tuple<const FeatureContext, std::shared_ptr<SymbolInfo>>> textInfos;

    tileTextPositionMap[tileInfo] = {};
    double tilePixelFactor = (0.0254 / camera->getScreenDensityPpi()) * tileInfo.zoomLevel;

    for(auto& feature : layerFeatures)
    {
        auto& context = std::get<0>(feature);
        auto& geometry = std::get<1>(feature);

        auto const evalContext = EvaluationContext(tileInfo.zoomIdentifier, context);

        if ((description->filter != nullptr && !description->filter->evaluateOr(evalContext, true))) {
            continue;
        }

        std::vector<FormattedStringEntry> text = description->style.getTextField(evalContext);

        if(description->style.getTextTransform(evalContext) == TextTransform::UPPERCASE && text.size() > 2) {
            for(auto &e: text) {
                e.text = TextHelper::uppercase(e.text);
            }
        }

        std::string fullText;

        for(auto const &textEntry: text) {
            fullText += textEntry.text;
        }

        auto anchor = description->style.getTextAnchor(evalContext);
        auto justify = description->style.getTextJustify(evalContext);
        auto placement = description->style.getTextSymbolPlacement(evalContext);

        auto variableTextAnchor = description->style.getTextVariableAnchor(evalContext);
        if (!variableTextAnchor.empty()) {
            // TODO: evaluate all anchors correctly and choose best one
            // for now the first one is simply used
            anchor = *variableTextAnchor.begin();
        }

        auto fontList = description->style.getTextFont(evalContext);

        double symbolSpacingPx = description->style.getSymbolSpacing(evalContext);
        double symbolSpacingMeters = symbolSpacingPx * tilePixelFactor;

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
                        textInfos.push_back({context, std::make_shared<SymbolInfo>(text,
                                                                                   position,
                                                                                   line,
                                                                                   Font(*fontList.begin()),
                                                                                   anchor,
                                                                                   pos->angle,
                                                                                   justify,
                                                                                   placement)
                        });


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


                            {
                                std::lock_guard<std::recursive_mutex> lock(tileTextPositionMapMutex);

                                std::optional<double> minDistance;

//                                for (auto const &entry: tileTextPositionMap) {
//                                    auto it = entry.second.find(fullText);
//                                    if (it != entry.second.end()) {
//                                        for (auto const &pos: entry.second.at(fullText)) {
//                                            auto distance = Vec2DHelper::distance(Vec2D(pos.x, pos.y), Vec2D(position.x, position.y));
//                                            if (!minDistance || distance < *minDistance) {
//                                                minDistance = distance;
//                                            }
//                                        }
//                                    }
//                                }
//                                if (minDistance && *minDistance < symbolSpacingMeters) {
//                                    continue;
//                                }

                                tileTextPositionMap[tileInfo][fullText].push_back(position);

                            }

                            wasPlaced = true;
                            textInfos.push_back({context, std::make_shared<SymbolInfo>(text,
                                                                                       position,
                                                                                       std::nullopt,
                                                                                       Font(*fontList.begin()),
                                                                                       anchor,
                                                                                       pos->angle,
                                                                                       justify,
                                                                                       placement)
                            });


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

                textInfos.push_back({context, std::make_shared<SymbolInfo>(text,
                                                                           *midP,
                                                                           std::nullopt,
                                                                           Font(*fontList.begin()),
                                                                           anchor,
                                                                           angle,
                                                                           justify,
                                                                           placement)
                });
            }
        }
    }

    addTexts(tileInfo, textInfos);
}

void Tiled2dMapVectorSymbolSubLayer::addTexts(const Tiled2dMapTileInfo &tileInfo,
                                              const std::vector<std::tuple<const FeatureContext, std::shared_ptr<SymbolInfo>>> &texts) {

    auto mapInterface = this->mapInterface;
    auto objectFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!objectFactory || !camera) {
        return;
    }

    if (texts.empty()) {
        if (auto delegate = readyDelegate.lock()) {
            // delegate->tileIsReady(tileInfo, "", std::vector<std::shared_ptr<RenderObjectInterface>>{});
        }
        return;
    }

    std::vector<std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper>> textObjects;

    auto textHelper = TextHelper(mapInterface);

    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(camera->getZoom());

    bool containsSelectedText = false;
    std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper> selectedTextWrapper;
    {
        std::lock_guard<std::recursive_mutex> lock(selectedTextWrapperMutex);
        selectedTextWrapper = this->selectedTextWrapper;
    }

    for (auto& [context, text] : texts) {

        if (selectedTextWrapper && context.identifier == selectedTextWrapper->featureContext.identifier) {
            containsSelectedText = true;
            continue;
        }

        auto fontResult = loadFont(text->getFont());
        if (fontResult.status != LoaderStatus::OK) {
            continue;
        }

        auto const evalContext = EvaluationContext(zoomIdentifier, context);

        auto textOffset = description->style.getTextOffset(evalContext);

        auto const textRadialOffset = description->style.getTextRadialOffset(evalContext);
        // TODO: currently only shifting to top right
        textOffset.x += textRadialOffset;
        textOffset.y -= textRadialOffset;

        auto const letterSpacing = description->style.getTextLetterSpacing(evalContext);

        auto const textObject = textHelper.textLayerObject(text,
                                                     fontResult.fontData,
                                                     textOffset,
                                                     description->style.getTextLineHeight(evalContext),
                                                     letterSpacing);

        if(textObject) {
            int64_t const symbolSortKey = description->style.getSymbolSortKey(evalContext);
            auto wrapper = std::make_shared<Tiled2dMapVectorSymbolFeatureWrapper>(context, text, textObject, symbolSortKey);

#ifdef DRAW_TEXT_BOUNDING_BOXES
            auto colorShader = mapInterface->getShaderFactory()->createColorShader();
            colorShader->setColor(0.0, 1.0, 0.0, 0.5);
            std::shared_ptr<Quad2dInterface> quadObject = mapInterface->getGraphicsObjectFactory()->createQuad(colorShader->asShaderProgramInterface());
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
            
            textObjects.push_back(wrapper);
        }
    }


    {
        std::lock_guard<std::recursive_mutex> lock(symbolMutex);
        if (containsSelectedText) {
            textObjects.push_back(selectedTextWrapper);
        }
        tileTextMap[tileInfo] = textObjects;
    }
    
    {
        std::lock_guard<std::recursive_mutex> lock(sortedSymbolMutex);
        auto cbefroe = sortedSymbols.size();
        std::copy(textObjects.begin(), textObjects.end(), std::inserter(sortedSymbols, sortedSymbols.end()));
        assert(cbefroe + textObjects.size() == sortedSymbols.size());
    }
    
    {
        std::lock_guard<std::recursive_mutex> lock(dirtyMutex);
        hasFreshData = true;
    }

    {
        std::lock_guard<std::recursive_mutex> lock(tilesInSetupMutex);
        tilesInSetup.insert(tileInfo);
    }

    std::weak_ptr<Tiled2dMapVectorSymbolSubLayer> selfPtr =
            std::dynamic_pointer_cast<Tiled2dMapVectorSymbolSubLayer>(shared_from_this());
    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
            TaskConfig("Tiled2dMapVectorSymbolSubLayer_setup", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
            [selfPtr, tileInfo, textObjects] { if (selfPtr.lock()) selfPtr.lock()->setupTexts(tileInfo, textObjects); }));
}

Quad2dD Tiled2dMapVectorSymbolSubLayer::getProjectedFrame(const RectCoord &boundingBox, const float &padding, const std::vector<float> &modelMatrix) {

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

bool Tiled2dMapVectorSymbolSubLayer::isDirty() {
    auto mapInterface = this->mapInterface;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!camera) {
        return false;
    }

    std::lock_guard<std::recursive_mutex> lock(dirtyMutex);
    return hasFreshData || Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(camera->getZoom()) != lastZoom || -camera->getRotation() != lastRotation;
}

void Tiled2dMapVectorSymbolSubLayer::collisionDetection(std::vector<OBB2D> &placements) {
    auto mapInterface = this->mapInterface;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!camera) {
        return;
    }

    std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper> selectedTextWrapper;
    {
        std::lock_guard<std::recursive_mutex> lock(selectedTextWrapperMutex);
        selectedTextWrapper = this->selectedTextWrapper;
    }

    std::lock_guard<std::recursive_mutex> lock(sortedSymbolMutex);

    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(camera->getZoom());
    double rotation = -camera->getRotation();

    auto scaleFactor = camera->mapUnitsFromPixels(1.0);

    {
        std::lock_guard<std::recursive_mutex> lock(dirtyMutex);
        lastZoom = zoomIdentifier;
        lastRotation = rotation;
        hasFreshData = false;
    }

    auto collisionDetectionLambda = [&](std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper> wrapper){
        auto const evalContext = EvaluationContext(zoomIdentifier, wrapper->featureContext);
        wrapper->collides = false;
    };

    if (selectedTextWrapper) {
        collisionDetectionLambda(selectedTextWrapper);
    }

    for (auto &wrapper: sortedSymbols) {
        if (wrapper == selectedTextWrapper) {
            continue;
        }
        collisionDetectionLambda(wrapper);
    }
}

void Tiled2dMapVectorSymbolSubLayer::setupTexts(const Tiled2dMapTileInfo &tileInfo,
                                                const std::vector<std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper>> texts) {
    auto mapInterface = this->mapInterface;
    if (!mapInterface) return;

    bool informDelegateAndReturn = false;

    {
        std::lock_guard<std::recursive_mutex> lock(symbolMutex);
        if (tileTextMap.count(tileInfo) == 0) {
            informDelegateAndReturn = true;
        }
    }

    if (informDelegateAndReturn) {
        if (auto delegate = readyDelegate.lock()) {
            // delegate->tileIsReady(tileInfo, "", std::vector<std::shared_ptr<RenderObjectInterface>>{});
        }
        return;
    }


    for (auto const &text: texts) {
        const auto &t = text->textInfo;

        const auto &textObject = text->textObject->getTextObject();
        if (textObject) {
            auto renderingContext = mapInterface->getRenderingContext();
            textObject->asGraphicsObject()->setup(renderingContext);

            auto fontResult = loadFont(t->getFont());
            if(fontResult.imageData) {
                textObject->loadTexture(renderingContext, fontResult.imageData);
            }
        }
    }
    {
        std::lock_guard<std::recursive_mutex> lock(tilesInSetupMutex);
        tilesInSetup.erase(tileInfo);
    }

    if (auto delegate = readyDelegate.lock()) {
        // delegate->tileIsReady(tileInfo, "", std::vector<std::shared_ptr<RenderObjectInterface>>{});
    }
}

void Tiled2dMapVectorSymbolSubLayer::update() {
    auto mapInterface = this->mapInterface;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!mapInterface || !camera) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(symbolMutex);

    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(camera->getZoom());
    auto scaleFactor = camera->mapUnitsFromPixels(1.0);

    for (auto &wrapper: sortedSymbols) {
        auto const evalContext = EvaluationContext(zoomIdentifier, wrapper->featureContext);

        const auto &object = wrapper->textObject;
        auto ref = object->getReferenceSize();
        auto scale = scaleFactor * description->style.getTextSize(evalContext) / ref;

        wrapper->textObject->update(scale);
    }
}

void Tiled2dMapVectorSymbolSubLayer::clearTileData(const Tiled2dMapTileInfo &tileInfo) {
    auto mapInterface = this->mapInterface;
    if (!mapInterface) { return; }
    
    std::vector<std::shared_ptr<GraphicsObjectInterface>> objectsToClear;
    Tiled2dMapVectorSubLayer::clearTileData(tileInfo);


    std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper> selectedTextWrapper;
    {
        std::lock_guard<std::recursive_mutex> lock(selectedTextWrapperMutex);
        selectedTextWrapper = this->selectedTextWrapper;
    }

    {
        std::lock_guard<std::recursive_mutex> lock(symbolMutex);
        std::lock_guard<std::recursive_mutex> symbolLock(sortedSymbolMutex);
        if (tileTextMap.count(tileInfo) != 0) {
            for (auto const &wrapper: tileTextMap[tileInfo]) {
                const auto &textObject = wrapper->textObject->getTextObject();
                if (textObject && wrapper != selectedTextWrapper) {
                    objectsToClear.push_back(textObject->asGraphicsObject());
                }
                if(wrapper->symbolObject && wrapper != selectedTextWrapper) {
                    objectsToClear.push_back(wrapper->symbolGraphicsObject);
                }
                sortedSymbols.erase(wrapper);
            }
            tileTextMap.erase(tileInfo);
        }
    }

    {
        std::lock_guard<std::recursive_mutex> lock(tileTextPositionMapMutex);
        tileTextPositionMap.erase(tileInfo);
    }

    {
        std::lock_guard<std::recursive_mutex> lock(dirtyMutex);
        hasFreshData = true;
    }

    if (objectsToClear.empty()) return;

    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
            TaskConfig("LineGroupTile_clear_" + std::to_string(tileInfo.zoomIdentifier) + "/" + std::to_string(tileInfo.x) + "/" +
                       std::to_string(tileInfo.y), 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
            [objectsToClear] {
                for (const auto &textObject : objectsToClear) {
                    if (textObject->isReady()) {
                        textObject->clear();
                    }
                }
            }));
}

std::vector<std::shared_ptr<::RenderPassInterface>> Tiled2dMapVectorSymbolSubLayer::buildRenderPasses(const std::unordered_set<Tiled2dMapTileInfo> &tiles)
{
    auto camera = mapInterface->getCamera();

    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(camera->getZoom());

    std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper> selectedTextWrapper;
    {
        std::lock_guard<std::recursive_mutex> lock(selectedTextWrapperMutex);
        selectedTextWrapper = this->selectedTextWrapper;
    }

    std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> renderPassObjectMap;

    if (selectedTextWrapper && selectedTextWrapper->symbolGraphicsObject) {
        renderPassObjectMap[description->renderPassIndex.value_or(1)].push_back(std::make_shared<RenderObject>(selectedTextWrapper->symbolGraphicsObject, selectedTextWrapper->iconModelMatrix));


#ifdef DRAW_TEXT_BOUNDING_BOXES
        renderPassObjectMap[description->renderPassIndex.value_or(0)].push_back(std::make_shared<RenderObject>(selectedTextWrapper->boundingBox->asGraphicsObject()));
#endif

#ifdef DRAW_TEXT_LINES
        if(selectedTextWrapper->lineObject) {
            renderPassObjectMap[description->renderPassIndex.value_or(0)].push_back(std::make_shared<RenderObject>(selectedTextWrapper->lineObject->getLineObject()));
        }
#endif
    }

    if(!(description->minZoom > zoomIdentifier || description->maxZoom < zoomIdentifier)) {
        std::lock_guard<std::recursive_mutex> lock(symbolMutex);
        for (auto const &[tile, wrapperVector] : tileTextMap) {
            if (tiles.count(tile) == 0) continue;
            for (auto const &wrapper : wrapperVector) {
                if (wrapper == selectedTextWrapper) {
                    continue;
                }
#ifndef DRAW_COLLIDED_TEXT_BOUNDING_BOXES
                if (wrapper->collides) {
                    continue;
                }
#endif
                const auto &object = wrapper->textObject;

                const auto &configs = object->getRenderConfig();

                if (wrapper->symbolGraphicsObject) {
                    renderPassObjectMap[description->renderPassIndex.value_or(1)].push_back(std::make_shared<RenderObject>(wrapper->symbolGraphicsObject, wrapper->iconModelMatrix));
                }


                if (!configs.empty()) {
                    std::lock_guard<std::recursive_mutex> lock(selectedFeatureIdentifierMutex);
                    if (wrapper->featureContext.identifier != selectedFeatureIdentifier) {
                        renderPassObjectMap[description->renderPassIndex.value_or(configs.front()->getRenderIndex())].push_back(std::make_shared<RenderObject>(configs.front()->getGraphicsObject(), wrapper->modelMatrix));
                    }
                }

#ifdef DRAW_TEXT_BOUNDING_BOXES
                renderPassObjectMap[description->renderPassIndex.value_or(0)].push_back(std::make_shared<RenderObject>(wrapper->boundingBox->asGraphicsObject()));
#endif

#ifdef DRAW_TEXT_LINES
                if(wrapper->lineObject) {
                    renderPassObjectMap[description->renderPassIndex.value_or(0)].push_back(std::make_shared<RenderObject>(wrapper->lineObject->getLineObject()));
                }
#endif

#ifdef DRAW_TEXT_LETTER_BOXES
                if(wrapper->textObject) {
                    auto boxes = wrapper->textObject->getLetterBoxes();

                    for(auto& b : boxes) {
                        auto cs = mapInterface->getShaderFactory()->createColorShader();
                        cs->setColor(0.0, 0.0, 1.0, 0.5);
                        std::shared_ptr<Quad2dInterface> quadObject = mapInterface->getGraphicsObjectFactory()->createQuad(cs->asShaderProgramInterface());
                        quadObject->asGraphicsObject()->setup(mapInterface->getRenderingContext());
                        quadObject->setFrame(b, RectD(0.0, 0.0, 1.0, 1.0));

                        renderPassObjectMap[description->renderPassIndex.value_or(0)].push_back(std::make_shared<RenderObject>(quadObject->asGraphicsObject()));
                    }
                }
#endif
            }

            
        }

    }


    std::vector<std::shared_ptr<RenderPassInterface>> newRenderPasses;
    for (const auto &passEntry : renderPassObjectMap) {
        std::shared_ptr<RenderPass> renderPass = std::make_shared<RenderPass>(RenderPassConfig(passEntry.first), passEntry.second);
        renderPass->setScissoringRect(scissorRect);
        newRenderPasses.push_back(renderPass);
    }

    return newRenderPasses;
}

FontLoaderResult Tiled2dMapVectorSymbolSubLayer::loadFont(const Font &font) {
    std::lock_guard<std::recursive_mutex> overlayLock(fontResultsMutex);
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

void Tiled2dMapVectorSymbolSubLayer::setSprites(std::shared_ptr<TextureHolderInterface> spriteTexture, std::shared_ptr<SpriteData> spriteData) {
    this->spriteData = spriteData;
    this->spriteTexture = spriteTexture;

    auto mapInterface = this->mapInterface;
    if (!mapInterface) return;
    
    std::weak_ptr<Tiled2dMapVectorSymbolSubLayer> weakSelfPtr =
    std::dynamic_pointer_cast<Tiled2dMapVectorSymbolSubLayer>(shared_from_this());
    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(TaskConfig("Tiled2dMapVectorSymbolSubLayer_setSprites",
                                                                                           0,
                                                                                           TaskPriority::NORMAL,
                                                                                           ExecutionEnvironment::GRAPHICS),
             [weakSelfPtr] {
                 auto selfPtr = weakSelfPtr.lock();
                 if (!selfPtr) { return; }
                 {
                     std::lock_guard<std::recursive_mutex> lock(selfPtr->symbolMutex);
                     for (auto const &[tile, objects]: selfPtr->tileTextMap) {
                         for (auto const &wrapper: objects) {
                             if (wrapper->symbolObject) {
                                 wrapper->symbolObject->loadTexture(selfPtr->mapInterface->getRenderingContext(), selfPtr->spriteTexture);
                                 wrapper->symbolGraphicsObject->setup(selfPtr->mapInterface->getRenderingContext());
                             }
                         }
                     }
                 }
                 auto mapInterface = selfPtr->mapInterface;
                 if (mapInterface) {
                    mapInterface->invalidate();
                 }

    }));
}

void Tiled2dMapVectorSymbolSubLayer::setScissorRect(const std::optional<::RectI> &scissorRect) {
    this->scissorRect = scissorRect;
}

bool Tiled2dMapVectorSymbolSubLayer::onClickConfirmed(const ::Vec2F &posScreen) {

    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto conversionHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    auto selectionDelegate = this->selectionDelegate.lock();
    if (!camera || !conversionHelper || !selectionDelegate) {
        return false;
    }


    Coord clickCoords = camera->coordFromScreenPosition(posScreen);
    Coord clickCoordsRenderCoord = conversionHelper->convertToRenderSystem(clickCoords);

    double clickPadding = camera->mapUnitsFromPixels(32);

    OBB2D tinyClickBox(Quad2dD(Vec2D(clickCoordsRenderCoord.x - clickPadding, clickCoordsRenderCoord.y - clickPadding),
                               Vec2D(clickCoordsRenderCoord.x + clickPadding, clickCoordsRenderCoord.y - clickPadding),
                               Vec2D(clickCoordsRenderCoord.x + clickPadding, clickCoordsRenderCoord.y + clickPadding),
                               Vec2D(clickCoordsRenderCoord.x - clickPadding, clickCoordsRenderCoord.y + clickPadding)));

    std::optional<FeatureContext> selectedFeatureContext;
    std::optional<Coord> selectedCoordinate;
    {
        std::lock_guard<std::recursive_mutex> lock(symbolMutex);
        for (auto &[tile, wrapperVector]: tileTextMap) {
            for (auto &wrapper: wrapperVector) {
                if (wrapper->collides) { continue; }
                if (wrapper->orientedBoundingBox.overlaps(tinyClickBox)) {
                    selectedCoordinate = wrapper->textInfo->getCoordinate();
                    selectedFeatureContext = wrapper->featureContext;
                    break;
                }
            }
            if (selectedFeatureContext && selectedCoordinate) {
                break;
            }
        }
    }

    if (selectedFeatureContext && selectedCoordinate) {
        selectionDelegate->didSelectFeature(*selectedFeatureContext, description, *selectedCoordinate);
        return true;
    }

    return false;
}


std::string Tiled2dMapVectorSymbolSubLayer::getLayerDescriptionIdentifier() {
    return description->identifier;
}

void Tiled2dMapVectorSymbolSubLayer::setSelectedFeatureIdentfier(std::optional<int64_t> identifier) {
    Tiled2dMapVectorSubLayer::setSelectedFeatureIdentfier(identifier);

    std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper> selectedTextWrapper;

    {
        std::lock_guard<std::recursive_mutex> lock(symbolMutex);
        for (auto &[tile, wrapperVector]: tileTextMap) {
            for (auto &wrapper: wrapperVector) {
                if (wrapper->featureContext.identifier == identifier) {
                    selectedTextWrapper = wrapper;
                }
            }
        }
    }

    std::shared_ptr<Tiled2dMapVectorSymbolFeatureWrapper> previouslySelectedWrapper;
    {
        std::lock_guard<std::recursive_mutex> lock(selectedTextWrapperMutex);
        previouslySelectedWrapper = this->selectedTextWrapper;
        this->selectedTextWrapper = selectedTextWrapper;
    }

    if (previouslySelectedWrapper) {
        bool found = false;
        {
            std::lock_guard<std::recursive_mutex> lock(symbolMutex);
            for (auto &[tile, wrapperVector]: tileTextMap) {
                for (auto &wrapper: wrapperVector) {
                    if (wrapper == previouslySelectedWrapper) {
                        found = true;
                    }
                }
            }
        }

        // Symbol is no contained in tile map anymore, therefore we have to clear it
        if (!found) {
            mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
                                                                               TaskConfig("LineGroupTile_setSelectedFeatureIdentfier", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
                                                                               [previouslySelectedWrapper] {
                                                                                   if (previouslySelectedWrapper->textObject->getTextObject()->asGraphicsObject()->isReady()) {
                                                                                       previouslySelectedWrapper->textObject->getTextObject()->asGraphicsObject()->clear();
                                                                                   }
                                                                                   if (previouslySelectedWrapper->symbolObject && previouslySelectedWrapper->symbolGraphicsObject->isReady()) {
                                                                                       previouslySelectedWrapper->symbolGraphicsObject->clear();
                                                                                   }
                                                                               }));
        }
    }


    {
        std::lock_guard<std::recursive_mutex> lock(dirtyMutex);
        hasFreshData = true;
    }
}
