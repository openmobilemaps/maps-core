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

Tiled2dMapVectorSymbolSubLayer::Tiled2dMapVectorSymbolSubLayer(const std::shared_ptr<FontLoaderInterface> &fontLoader, const std::shared_ptr<SymbolVectorLayerDescription> &description)
: fontLoader(fontLoader),
  description(description)
{}

void Tiled2dMapVectorSymbolSubLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface) {
    Tiled2dMapVectorSubLayer::onAdded(mapInterface);
    shader = mapInterface->getShaderFactory()->createTextShader();
}

void Tiled2dMapVectorSymbolSubLayer::onRemoved() {
    Tiled2dMapVectorSubLayer::onRemoved();
}

void Tiled2dMapVectorSymbolSubLayer::pause() {
    Tiled2dMapVectorSubLayer::pause();
    {
        std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(tilesInSetupMutex, symbolMutex);

        for (const auto &tileGroup : tileTextMap) {
            tilesInSetup.insert(tileGroup.first);
            for (auto const &text: tileGroup.second) {
                const auto &object = std::get<1>(text)->getTextObject();
                if (object->asGraphicsObject()->isReady()) {
                    object->asGraphicsObject()->clear();
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
    for (const auto &tileGroup : tileTextMap) {
        for (auto const &text: tileGroup.second) {
            const auto &fontInfo = std::get<0>(text)->getFont();
            const auto &object = std::get<1>(text)->getTextObject();

            auto fontResult = loadFont(fontInfo);
            if(fontResult.imageData) {
                object->loadTexture(fontResult.imageData);
            }

            if (!object->asGraphicsObject()->isReady()) {
                object->asGraphicsObject()->setup(context);
            }
        }
        tilesInSetup.erase(tileGroup.first);
        if (auto delegate = readyDelegate.lock()) {
            delegate->tileIsReady(tileGroup.first);
        }
    }
}

void Tiled2dMapVectorSymbolSubLayer::hide() {
    Tiled2dMapVectorSubLayer::hide();
}

void Tiled2dMapVectorSymbolSubLayer::show() {
    Tiled2dMapVectorSubLayer::show();
}

void
Tiled2dMapVectorSymbolSubLayer::updateTileData(const Tiled2dMapTileInfo &tileInfo, const std::shared_ptr<MaskingObjectInterface> &tileMask, const std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>> &layerFeatures) {
    Tiled2dMapVectorSubLayer::updateTileData(tileInfo, tileMask, layerFeatures);
    if (!mapInterface) {
        return;
    }

    std::vector<std::shared_ptr<TextInfoInterface>> textInfos;

    for(auto& feature : layerFeatures)
    {
        auto& context = std::get<0>(feature);
        auto& geometry = std::get<1>(feature);

        if ((description->filter != nullptr && description->filter->evaluate(EvaluationContext(-1, context)))) {
            continue;
        }

        if(description->minZoom > tileInfo.zoomIdentifier ||
        description->maxZoom < tileInfo.zoomIdentifier) {
            continue;
        }

        if(context.geomType != vtzero::GeomType::POINT) {
            continue;
        }

        std::optional<std::string> text = std::nullopt;

        for(const auto& p : context.propertiesMap) {
            if(p.first == description->style.textField) {
                text = std::get<0>(p.second);
            }
        }

        if(text) {
            if(description->style.transform == TextTransform::UPPERCASE && text->size() > 2) {
                auto a = TextHelper::uppercase(*text);
                text = TextHelper::uppercase(*text);
            }

            for(const auto& p : geometry.getPointCoordinates()) {
                for(const auto& pp : p) {
                    textInfos.push_back(std::make_shared<TextInfo>(*text, pp, description->style.font));
                }
            }
        }
    }

    addTexts(tileInfo, textInfos);
}

void Tiled2dMapVectorSymbolSubLayer::addTexts(const Tiled2dMapTileInfo &tileInfo, std::vector<std::shared_ptr<TextInfoInterface>> &texts){

    if (texts.empty()) {
        if (auto delegate = readyDelegate.lock()) {
            delegate->tileIsReady(tileInfo);
        }
        return;
    }

    const auto &objectFactory = mapInterface->getGraphicsObjectFactory();

    std::vector<std::tuple<std::shared_ptr<TextInfoInterface>, std::shared_ptr<TextLayerObject>>> textObjects;

    auto textHelper = TextHelper(mapInterface);

    for (auto& text : texts) {
        auto fontResult = loadFont(text->getFont());
        if (fontResult.status != LoaderStatus::OK) continue;

        auto fontData = fontResult.fontData;

        auto textObject = textHelper.textLayer(text, fontData, description->style.textOffset);

        if(textObject) {
            textObjects.push_back(std::make_tuple(text, textObject));
        }
    }

    {
        std::lock_guard<std::recursive_mutex> lock(symbolMutex);
        tileTextMap[tileInfo] = textObjects;
    }

    {
        std::lock_guard<std::recursive_mutex> lock(tilesInSetupMutex);
        tilesInSetup.insert(tileInfo);
    }

    std::weak_ptr<Tiled2dMapVectorSymbolSubLayer> selfPtr = std::dynamic_pointer_cast<Tiled2dMapVectorSymbolSubLayer>(
            shared_from_this());
    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
            TaskConfig("Tiled2dMapVectorPolygonSubLayer_setup", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
            [selfPtr, tileInfo, textObjects] { if (selfPtr.lock()) selfPtr.lock()->setupTexts(tileInfo, textObjects); }));
}

void Tiled2dMapVectorSymbolSubLayer::setupTexts(const Tiled2dMapTileInfo &tileInfo,
                                                const std::vector<std::tuple<std::shared_ptr<TextInfoInterface>, std::shared_ptr<TextLayerObject>>> &texts) {
    {
        std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(tilesInSetupMutex, symbolMutex);
        if (tileTextMap.count(tileInfo) == 0) {
            if (auto delegate = readyDelegate.lock()) {
                delegate->tileIsReady(tileInfo);
            }
            return;
        };
        for (auto const &text: texts) {
            const auto &t = std::get<0>(text);
            const auto &textObject = std::get<1>(text)->getTextObject();
            textObject->asGraphicsObject()->setup(mapInterface->getRenderingContext());

            auto fontResult = loadFont(t->getFont());
            if(fontResult.imageData) {
                textObject->loadTexture(fontResult.imageData);
            }
        }
        tilesInSetup.erase(tileInfo);
    }
    if (auto delegate = readyDelegate.lock()) {
        delegate->tileIsReady(tileInfo);
    }
}

void Tiled2dMapVectorSymbolSubLayer::update() {
    std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(featureGroupsMutex, symbolMutex);

    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(mapInterface->getCamera()->getZoom());

    auto s = description->style.size->evaluate(EvaluationContext(zoomIdentifier, FeatureContext()));

    for(const auto& tileTextTuple : tileTextMap) {
        for (auto const &tile : tileTextTuple.second) {
            const auto &object = std::get<1>(tile);
            auto ref = object->getReferenceSize();
            auto scaleFactor = mapInterface->getCamera()->mapUnitsFromPixels(1.0);
            object->getShader()->setScale(scaleFactor * s / ref);

            auto &c = description->style.color;
            object->getShader()->setColor(c.r, c.g, c.b, c.a);
        }
    }
}


void Tiled2dMapVectorSymbolSubLayer::clearTileData(const Tiled2dMapTileInfo &tileInfo) {
    auto mapInterface = this->mapInterface;
    if (!mapInterface) { return; }
    
    std::vector<std::shared_ptr<GraphicsObjectInterface>> objectsToClear;
    Tiled2dMapVectorSubLayer::clearTileData(tileInfo);

    {
        std::lock_guard<std::recursive_mutex> lock(symbolMutex);
        if (tileTextMap.count(tileInfo) != 0) {
            for (auto const &text: tileTextMap[tileInfo]) {
                const auto &textObject = std::get<1>(text)->getTextObject();
                objectsToClear.push_back(textObject->asGraphicsObject());
            }
            tileTextMap.erase(tileInfo);
        }
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
    for (auto const &tileTextTuple : tileTextMap) {
        if (tiles.count(tileTextTuple.first) == 0) continue;
        std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> renderPassObjectMap;
        for (auto const &tile : tileTextTuple.second) {
            const auto &object = std::get<1>(tile);
            const auto& ref = object->getReferencePoint();
            const auto& modelMatrix = camera->getInvariantModelMatrix(ref, false, true);

            for (const auto &config : object->getRenderConfig()) {
                renderPassObjectMap[config->getRenderIndex()].push_back(
                        std::make_shared<RenderObject>(config->getGraphicsObject(), modelMatrix));
            }
        }

        const auto &tileMask = tileMaskMap[tileTextTuple.first];
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
