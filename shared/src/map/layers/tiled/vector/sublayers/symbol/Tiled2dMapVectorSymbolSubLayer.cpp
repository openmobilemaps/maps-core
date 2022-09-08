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

Tiled2dMapVectorSymbolSubLayer::Tiled2dMapVectorSymbolSubLayer(const std::shared_ptr<FontLoaderInterface> &fontLoader,
                                                               const std::shared_ptr<SymbolVectorLayerDescription> &description)
: fontLoader(fontLoader),
  description(description)
{}

void Tiled2dMapVectorSymbolSubLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface) {
    Tiled2dMapVectorSubLayer::onAdded(mapInterface);
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
                const auto &object = wrapper.textObject->getTextObject();
                if (object && object->asGraphicsObject()->isReady()) {
                    object->asGraphicsObject()->clear();
                }
                if (wrapper.symbolObject && wrapper.symbolObject->asGraphicsObject()->isReady()) {
                    wrapper.symbolObject->asGraphicsObject()->clear();
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
            const auto &object = wrapper.textObject->getTextObject();
            if (object) {
                const auto &fontInfo = wrapper.textInfo->getFont();

                auto fontResult = loadFont(fontInfo);

                if (!object->asGraphicsObject()->isReady()) {
                    object->asGraphicsObject()->setup(context);
                }

                if(fontResult.imageData) {
                    object->loadTexture(fontResult.imageData);
                }
            }

            auto const &symbolObject = wrapper.symbolObject;
            if (symbolObject) {
                if (!symbolObject->asGraphicsObject()->isReady()) {
                    symbolObject->asGraphicsObject()->setup(context);
                }
                if (spriteTexture) {
                    symbolObject->loadTexture(context, spriteTexture);
                }
            }
        }
        tilesInSetup.erase(tile);
        if (auto delegate = readyDelegate.lock()) {
            delegate->tileIsReady(tile);
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

        if(description->minZoom > tileInfo.zoomIdentifier ||
        description->maxZoom < tileInfo.zoomIdentifier) {
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

        auto variableTextAnchor = description->style.getTextVariableAnchor(evalContext);
        if (!variableTextAnchor.empty()) {
            // TODO: evaluate all anchors correctly and choose best one
            // for now the first one is simply used
            anchor = *variableTextAnchor.begin();
        }

        auto fontList = description->style.getTextFont(evalContext);

        double symbolSpacingPx = description->style.getSymbolSpacing(evalContext);

        double symbolSpacingMeters =  symbolSpacingPx * tilePixelFactor;

        if(context.geomType != vtzero::GeomType::POINT) {

            double distance = symbolSpacingMeters / 2.0;
            double totalDistance = 0;

            bool wasPlaced = false;

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

                        std::optional<double> minDistance;
                        for (auto const &entry: tileTextPositionMap) {
                            auto it = entry.second.find(fullText);
                            if (it != entry.second.end()) {
                                for (auto const &pos: entry.second.at(fullText)) {
                                    auto distance = Vec2DHelper::distance(Vec2D(pos.x, pos.y), Vec2D(position.x, position.y));
                                    if (!minDistance || distance < *minDistance) {
                                        minDistance = distance;
                                    }
                                }
                            }
                        }

                        if (minDistance && *minDistance < symbolSpacingMeters) {
                            continue;
                        }

                        tileTextPositionMap[tileInfo][fullText].push_back(position);

                        wasPlaced = true;
                        textInfos.push_back({context, std::make_shared<SymbolInfo>(text,
                                                                                   position,
                                                                                   Font(*fontList.begin()),
                                                                                   anchor,
                                                                                   pos->angle)
                        });


                        distance = 0;
                    }
                }
            }

            // if no label was placed place it in the middle of the line
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


                            std::optional<double> minDistance;
                            for (auto const &entry: tileTextPositionMap) {
                                auto it = entry.second.find(fullText);
                                if (it != entry.second.end()) {
                                    for (auto const &pos: entry.second.at(fullText)) {
                                        auto distance = Vec2DHelper::distance(Vec2D(pos.x, pos.y), Vec2D(position.x, position.y));
                                        if (!minDistance || distance < *minDistance) {
                                            minDistance = distance;
                                        }
                                    }
                                }
                            }
                            if (minDistance && *minDistance < symbolSpacingMeters) {
                                continue;
                            }

                            tileTextPositionMap[tileInfo][fullText].push_back(position);

                            wasPlaced = true;
                            textInfos.push_back({context, std::make_shared<SymbolInfo>(text,
                                                                                       position,
                                                                                       Font(*fontList.begin()),
                                                                                       anchor,
                                                                                       pos->angle)
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


                std::optional<double> minDistance;
                for (auto const &entry: tileTextPositionMap) {
                    auto it = entry.second.find(fullText);
                    if (it != entry.second.end()) {
                        for (auto const &pos: entry.second.at(fullText)) {
                            auto distance = Vec2DHelper::distance(Vec2D(pos.x, pos.y), Vec2D(midP->x, midP->y));
                            if (!minDistance || distance < *minDistance) {
                                minDistance = distance;
                            }
                        }
                    }
                }
                if (minDistance && *minDistance < symbolSpacingMeters) {
                    continue;
                }

                textInfos.push_back({context, std::make_shared<SymbolInfo>(text,
                                                                           *midP,
                                                                           Font(*fontList.begin()),
                                                                           anchor,
                                                                           angle)
                });


                tileTextPositionMap[tileInfo][fullText].push_back(*midP);
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
            delegate->tileIsReady(tileInfo);
        }
        return;
    }

    std::vector<Tiled2dMapVectorSymbolFeatureWrapper> textObjects;

    auto textHelper = TextHelper(mapInterface);

    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(camera->getZoom());

    for (auto& [context, text] : texts) {
        auto fontResult = loadFont(text->getFont());
        if (fontResult.status != LoaderStatus::OK) continue;

        auto fontData = fontResult.fontData;

        auto const evalContext = EvaluationContext(zoomIdentifier, context);

        auto textOffset = description->style.getTextOffset(evalContext);

        auto textRadialOffset = description->style.getTextRadialOffset(evalContext);
        // TODO: currently only shifting to top right
        textOffset.x += textRadialOffset;
        textOffset.y -= textRadialOffset;

        auto letterSpacing = description->style.getTextLetterSpacing(evalContext);

        auto textObject = textHelper.textLayerObject(text,
                                                     fontData,
                                                     textOffset,
                                                     description->style.getTextLineHeight(evalContext),
                                                     letterSpacing);

        if(textObject) {
            int64_t symbolSortKey = description->style.getSymbolSortKey(evalContext);
            Tiled2dMapVectorSymbolFeatureWrapper wrapper(context, text, textObject, symbolSortKey);


#ifdef DRAW_TEXT_BOUNDING_BOXES
            auto colorShader = mapInterface->getShaderFactory()->createColorShader();
            colorShader->setColor(0.0, 1.0, 0.0, 0.5);
            std::shared_ptr<Quad2dInterface> quadObject = mapInterface->getGraphicsObjectFactory()->createQuad(colorShader->asShaderProgramInterface());
            auto rectCoord = textObject->boundingBox;
            auto coord = rectCoord.topLeft;
            auto width = rectCoord.bottomRight.x - rectCoord.topLeft.x;
            auto height = rectCoord.bottomRight.y - rectCoord.topLeft.y;

            auto coords = QuadCoord(coord, Coord(coord.systemIdentifier, coord.x + width, coord.y, coord.z),
                      Coord(coord.systemIdentifier, coord.x + width, coord.y + height, coord.z),
                                    Coord(coord.systemIdentifier, coord.x, coord.y + height, coord.z));

            QuadCoord renderCoords = mapInterface->getCoordinateConverterHelper()->convertQuadToRenderSystem(coords);

            auto quad = Quad2dD(Vec2D(renderCoords.topLeft.x, renderCoords.topLeft.y), Vec2D(renderCoords.topRight.x, renderCoords.topRight.y),
                    Vec2D(renderCoords.bottomRight.x, renderCoords.bottomRight.y),
                                Vec2D(renderCoords.bottomLeft.x, renderCoords.bottomLeft.y));

            quadObject->setFrame(quad, RectD(0.0, 0.0, 1.0, 1.0));

            quadObject->asGraphicsObject()->setup(mapInterface->getRenderingContext());

            wrapper.boundingBox = quadObject;
#endif
            textObjects.push_back(wrapper);
        }
    }

    std::sort(textObjects.begin(), textObjects.end(), [](const Tiled2dMapVectorSymbolFeatureWrapper &a, const Tiled2dMapVectorSymbolFeatureWrapper &b) {
        return a.symbolSortKey < b.symbolSortKey;
    });

    {
        std::lock_guard<std::recursive_mutex> lock(symbolMutex);
        tileTextMap[tileInfo] = textObjects;
    }

    {
        std::lock_guard<std::recursive_mutex> lock(tilesInSetupMutex);
        tilesInSetup.insert(tileInfo);
    }

    std::weak_ptr<Tiled2dMapVectorSymbolSubLayer> selfPtr =
            std::dynamic_pointer_cast<Tiled2dMapVectorSymbolSubLayer>(shared_from_this());
    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
            TaskConfig("Tiled2dMapVectorPolygonSubLayer_setup", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
            [selfPtr, tileInfo, textObjects] { if (selfPtr.lock()) selfPtr.lock()->setupTexts(tileInfo, textObjects); }));
}

void Tiled2dMapVectorSymbolSubLayer::collisionDetection(std::vector<OBB2D> &placements) {
    auto mapInterface = this->mapInterface;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!camera) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(symbolMutex);


    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(camera->getZoom());
    auto scaleFactor = camera->mapUnitsFromPixels(1.0);


    for (auto &[tile, wrapperVector]: tileTextMap) {
        for (auto &wrapper: wrapperVector) {

            auto const evalContext = EvaluationContext(zoomIdentifier, wrapper.featureContext);

            const auto &object = wrapper.textObject;
            auto ref = object->getReferenceSize();
            const auto& refP = object->getReferencePoint();
            auto scale = scaleFactor * description->style.getTextSize(evalContext) / ref;
            if (object->getShader())  {
                object->getShader()->setScale(1.0);
            }

            const bool hasText = object->getTextObject() != nullptr;

            Coord renderCoord = mapInterface->getCoordinateConverterHelper()->convertToRenderSystem(refP);

            double rotation = -camera->getRotation();

            if (hasText) {
                Matrix::setIdentityM(wrapper.modelMatrix, 0);
                Matrix::translateM(wrapper.modelMatrix, 0, renderCoord.x, renderCoord.y, renderCoord.z);

                Matrix::scaleM(wrapper.modelMatrix, 0, scale, scale, 1.0);

                rotation += description->style.getTextRotate(evalContext);

                if (wrapper.textInfo->angle) {

                    double rotateSum = fmod(rotation - (*wrapper.textInfo->angle) + 720.0, 360);
                    if (rotateSum > 270 || rotateSum < 90) {
                        rotation = *wrapper.textInfo->angle;
                    } else {
                        rotation = *wrapper.textInfo->angle + 180;
                    }
                }
                Matrix::rotateM(wrapper.modelMatrix, 0.0, rotation, 0.0, 0.0, 1.0);

                Matrix::translateM(wrapper.modelMatrix, 0, -renderCoord.x, -renderCoord.y, -renderCoord.z);

            }

            wrapper.collides = false;

            if (hasText) {
                auto const &curB = object->boundingBox;

                auto topLeft = Vec2D(curB.topLeft.x, curB.topLeft.y);
                auto topRight = Vec2D(curB.bottomRight.x, topLeft.y);
                auto bottomRight = Vec2D(curB.bottomRight.x, curB.bottomRight.y);
                auto bottomLeft = Vec2D(topLeft.x, bottomRight.y);

                float padding = description->style.getTextPadding(evalContext);

                topLeft.x -= padding;
                topLeft.y -= padding;

                topRight.x += padding;
                topRight.y -= padding;

                bottomLeft.x -= padding;
                bottomLeft.y += padding;

                bottomRight.x += padding;
                bottomRight.y += padding;

                auto topLeftProj = Matrix::multiply(wrapper.modelMatrix, {(float)topLeft.x, (float)topLeft.y, 0.0, 1.0});
                auto topRightProj = Matrix::multiply(wrapper.modelMatrix, {(float)topRight.x, (float)topRight.y, 0.0, 1.0});
                auto bottomRightProj = Matrix::multiply(wrapper.modelMatrix, {(float)bottomRight.x, (float)bottomRight.y, 0.0, 1.0});
                auto bottomLeftProj = Matrix::multiply(wrapper.modelMatrix, {(float)bottomLeft.x, (float)bottomLeft.y, 0.0, 1.0});

#ifdef DRAW_TEXT_BOUNDING_BOXES
                wrapper.boundingBox->setFrame(Quad2dD(Vec2D(topLeftProj[0], topLeftProj[1]), Vec2D(topRightProj[0], topRightProj[1]), Vec2D(bottomRightProj[0], bottomRightProj[1]), Vec2D(bottomLeftProj[0], bottomLeftProj[1])), RectD(0, 0, 1, 1));
#endif

                OBB2D orientedBox(Vec2D(topLeftProj[0], topLeftProj[1]), Vec2D(topRightProj[0], topRightProj[1]),
                                  Vec2D(bottomRightProj[0], bottomRightProj[1]), Vec2D(bottomLeftProj[0], bottomLeftProj[1]));

                for ( auto const &otherB: placements ) {
                    if (otherB.overlaps(orientedBox)) {
                        wrapper.collides = true;
                        break;
                    }
                }
                if (!wrapper.collides) {
                    placements.push_back(orientedBox);
                }
            }

            if (!wrapper.collides) {
                auto const &shader = object->getShader();
                if (shader) {
                    object->getShader()->setColor(description->style.getTextColor(evalContext));
                    object->getShader()->setHaloColor(description->style.getTextHaloColor(evalContext));
                }

                if (spriteData && spriteTexture) {
                    auto iconImage = description->style.getIconImage(evalContext);

                    if (iconImage != "") {
                        Matrix::setIdentityM(wrapper.iconModelMatrix, 0);
                        Matrix::translateM(wrapper.iconModelMatrix, 0, renderCoord.x, renderCoord.y, renderCoord.z);

                        auto iconSize = description->style.getIconSize(evalContext) * scaleFactor;

                        Matrix::scaleM(wrapper.iconModelMatrix, 0, iconSize, iconSize, 1.0);
                        Matrix::rotateM(wrapper.iconModelMatrix, 0.0, rotation, 0.0, 0.0, 1.0);
                        Matrix::translateM(wrapper.iconModelMatrix, 0, -renderCoord.x, -renderCoord.y, -renderCoord.z);
                    }

                    if (iconImage != "" && object->getCurrentSymbolName() != iconImage && spriteData->sprites.count(iconImage) != 0) {
                        object->setCurrentSymbolName(iconImage);

                        auto const &spriteInfo = spriteData->sprites.at(iconImage);

                        if (!wrapper.symbolShader) {
                            wrapper.symbolShader = mapInterface->getShaderFactory()->createAlphaShader();
                        }

                        if (!wrapper.symbolObject) {
                            wrapper.symbolObject = mapInterface->getGraphicsObjectFactory()->createQuad(wrapper.symbolShader->asShaderProgramInterface());
                        }

                        Coord renderPos = mapInterface->getCoordinateConverterHelper()->convertToRenderSystem(wrapper.textInfo->getCoordinate());

                        double densityOffset = (mapInterface->getCamera()->getScreenDensityPpi() / 160.0) / spriteInfo.pixelRatio;

                        auto iconOffset = description->style.getIconOffset(evalContext);
                        renderPos.y -= iconOffset.y;
                        renderPos.x += iconOffset.x;

                        auto x = renderPos.x - (spriteInfo.width * densityOffset) / 2;
                        auto y = renderPos.y + (spriteInfo.height * densityOffset) / 2;
                        auto xw = renderPos.x + (spriteInfo.width * densityOffset) / 2;
                        auto yh = renderPos.y - (spriteInfo.height * densityOffset) / 2;

                        Quad2dD quad = Quad2dD(Vec2D(x, yh), Vec2D(xw, yh), Vec2D(xw, y), Vec2D(x, y));

                        auto textureWidth = (double) spriteTexture->getImageWidth();
                        auto textureHeight = (double) spriteTexture->getImageHeight();

                        wrapper.symbolObject->setFrame(quad, RectD( ((double) spriteInfo.x) / textureWidth,
                                                                         ((double) spriteInfo.y) / textureHeight,
                                                                         ((double) spriteInfo.width) / textureWidth,
                                                                         ((double) spriteInfo.height) / textureHeight));

                        if (spriteTexture) {
                            wrapper.symbolObject->asGraphicsObject()->setup(mapInterface->getRenderingContext());
                            wrapper.symbolObject->loadTexture(mapInterface->getRenderingContext(), spriteTexture);
                        }
                    }
                }
            }
#ifdef DRAW_TEXT_BOUNDING_BOXES
            else {
                if (object->getShader()) {
                    object->getShader()->setColor(Color(1.0, 0.0, 0.0, 1.0));
                }
            }
#endif
        }
    }
}

void Tiled2dMapVectorSymbolSubLayer::setupTexts(const Tiled2dMapTileInfo &tileInfo,
                                                const std::vector<Tiled2dMapVectorSymbolFeatureWrapper> texts) {
    {
        std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(tilesInSetupMutex, symbolMutex);
        auto mapInterface = this->mapInterface;
        if (!mapInterface) return;
        if (tileTextMap.count(tileInfo) == 0) {
            if (auto delegate = readyDelegate.lock()) {
                delegate->tileIsReady(tileInfo);
            }
            return;
        };
        for (auto const &text: texts) {
            const auto &t = text.textInfo;

            const auto &textObject = text.textObject->getTextObject();
            if (textObject) {
                textObject->asGraphicsObject()->setup(mapInterface->getRenderingContext());

                auto fontResult = loadFont(t->getFont());
                if(fontResult.imageData) {
                    textObject->loadTexture(fontResult.imageData);
                }
            }
        }
        tilesInSetup.erase(tileInfo);
    }
    if (auto delegate = readyDelegate.lock()) {
        delegate->tileIsReady(tileInfo);
    }
}

void Tiled2dMapVectorSymbolSubLayer::update() {}


void Tiled2dMapVectorSymbolSubLayer::clearTileData(const Tiled2dMapTileInfo &tileInfo) {
    auto mapInterface = this->mapInterface;
    if (!mapInterface) { return; }
    
    std::vector<std::shared_ptr<GraphicsObjectInterface>> objectsToClear;
    Tiled2dMapVectorSubLayer::clearTileData(tileInfo);

    {
        std::lock_guard<std::recursive_mutex> lock(symbolMutex);
        if (tileTextMap.count(tileInfo) != 0) {
            for (auto const &wrapper: tileTextMap[tileInfo]) {
                const auto &textObject = wrapper.textObject->getTextObject();
                if (textObject) {
                    objectsToClear.push_back(textObject->asGraphicsObject());
                }
                if(wrapper.symbolObject) {
                    objectsToClear.push_back(wrapper.symbolObject->asGraphicsObject());
                }
            }
            tileTextMap.erase(tileInfo);
        }
        tileTextPositionMap.erase(tileInfo);
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

    std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(maskMutex, symbolMutex);
    std::vector<std::shared_ptr<RenderPassInterface>> newRenderPasses;
    for (auto const &[tile, wrapperVector] : tileTextMap) {
        if (tiles.count(tile) == 0) continue;
        std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> renderPassObjectMap;
        for (auto const &wrapper : wrapperVector) {
#ifndef DRAW_TEXT_BOUNDING_BOXES
            if (wrapper.collides) {
                continue;
            }
#endif
            const auto &object = wrapper.textObject;

            const auto &configs = object->getRenderConfig();

            if (wrapper.symbolObject) {
                renderPassObjectMap[0].push_back(std::make_shared<RenderObject>(wrapper.symbolObject->asGraphicsObject(), wrapper.iconModelMatrix));
            }

            if (!configs.empty()) {
                renderPassObjectMap[configs.front()->getRenderIndex()].push_back(std::make_shared<RenderObject>(configs.front()->getGraphicsObject(), wrapper.modelMatrix));
            }

            #ifdef DRAW_TEXT_BOUNDING_BOXES
            renderPassObjectMap[0].push_back(std::make_shared<RenderObject>(wrapper.boundingBox->asGraphicsObject()));
            #endif
        }

        const auto &tileMask = tileMaskMap[tile];
        for (const auto &passEntry : renderPassObjectMap) {
            std::shared_ptr<RenderPass> renderPass = std::make_shared<RenderPass>(RenderPassConfig(passEntry.first),
                                                                                  passEntry.second,
                                                                                  (/*tileMask
                                                                                   ? tileMask
                                                                                   : */nullptr));
            newRenderPasses.push_back(renderPass);
        }
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
                             if (wrapper.symbolObject) {
                                 wrapper.symbolObject->loadTexture(selfPtr->mapInterface->getRenderingContext(), selfPtr->spriteTexture);
                                 wrapper.symbolObject->asGraphicsObject()->setup(selfPtr->mapInterface->getRenderingContext());
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