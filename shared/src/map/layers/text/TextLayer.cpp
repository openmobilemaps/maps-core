/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "TextLayer.h"

#include "AlphaShaderInterface.h"
#include "FontLoaderResult.h"
#include "GlyphDescription.h"
#include "LambdaTask.h"
#include "RectCoord.h"
#include "RenderObject.h"
#include "RenderObjectInterface.h"
#include "RenderPass.h"
#include "TextDescription.h"
#include "TextHelper.h"
#include "TextShaderInterface.h"

TextLayer::TextLayer(const std::shared_ptr<::FontLoaderInterface> &fontLoader)
    : fontLoader(fontLoader)
    , mapInterface(nullptr)
    , isHidden(false) {}

void TextLayer::setTexts(const std::vector<std::shared_ptr<TextInfoInterface>> &texts) {
    clear();
    for (auto const &text : texts) {
        add(text);
    }
    generateRenderPasses();
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

std::shared_ptr<::LayerInterface> TextLayer::asLayerInterface() { return shared_from_this(); }

void TextLayer::invalidate() { setTexts(getTexts()); }

std::vector<std::shared_ptr<TextInfoInterface>> TextLayer::getTexts() {
    std::vector<std::shared_ptr<TextInfoInterface>> texts;
    if (!mapInterface) {
        for (auto const &line : addingQueue) {
            texts.push_back(line);
        }
        return texts;
    }

    for (auto const &text : this->texts) {
        texts.push_back(text.first);
    }

    return texts;
}

void TextLayer::setMaskingObject(const std::shared_ptr<::MaskingObjectInterface> &maskingObject) {
    // TODO.
}

std::vector<std::shared_ptr<::RenderPassInterface>> TextLayer::buildRenderPasses() {
    if (isHidden) {
        return {};
    } else {
        std::lock_guard<std::recursive_mutex> overlayLock(renderPassMutex);
        return renderPasses;
    }
}

void TextLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface, int32_t layerIndex) {
    this->mapInterface = mapInterface;

    {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        for (auto const &text : addingQueue) {
            add(text);
        }

        addingQueue.clear();
    }
}

void TextLayer::onRemoved() {
    {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.clear();
    }
}

void TextLayer::pause() {
    {
        std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(addingQueueMutex, textMutex);
        addingQueue.clear();
        for (const auto &text : texts) {
            addingQueue.insert(text.first);
        }
    }

    std::lock_guard<std::recursive_mutex> lock(textMutex);
    clearSync(texts);
    texts.clear();
}

void TextLayer::resume() {
    std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
    if (!addingQueue.empty()) {
        std::vector<std::shared_ptr<TextInfoInterface>> texts;
        for (auto const &text : addingQueue) {
            texts.push_back(text);
        }
        addingQueue.clear();

        addTexts(texts);
    }
}

void TextLayer::hide() {
    isHidden = true;
    auto mapInterface = this->mapInterface;
    if (mapInterface)
        mapInterface->invalidate();
}

void TextLayer::show() {
    isHidden = false;
    auto mapInterface = this->mapInterface;
    if (mapInterface)
        mapInterface->invalidate();
}

void TextLayer::clear() {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    auto scheduler = mapInterface ? mapInterface->getScheduler() : nullptr;
    if (!scheduler) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.clear();
        return;
    }

    {
        std::lock_guard<std::recursive_mutex> lock(textMutex);
        std::weak_ptr<TextLayer> weakSelfPtr = std::dynamic_pointer_cast<TextLayer>(shared_from_this());
        auto textsToClear = texts;
        scheduler->addTask(std::make_shared<LambdaTask>(
            TaskConfig("TextLayer_clear", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS), [weakSelfPtr, textsToClear] {
                if (auto self = weakSelfPtr.lock()) {
                    self->clearSync(textsToClear);
                }
            }));
        texts.clear();
    }
    {
        std::lock_guard<std::recursive_mutex> overlayLock(renderPassMutex);
        renderPasses.clear();
    }

    mapInterface->invalidate();
}

void TextLayer::clearSync(const std::unordered_map<std::shared_ptr<TextInfoInterface>, std::shared_ptr<TextLayerObject>> &textsToClear) {
    for (auto &text : textsToClear) {
        if (text.second->getTextGraphicsObject()->isReady())
        text.second->getTextGraphicsObject()->clear();
        text.second->getTextObject()->removeTexture();
    }
}

void TextLayer::update() {
    for (auto const &textTuple : texts) {
        textTuple.second->update();
    }
}

void TextLayer::generateRenderPasses() {
    std::lock_guard<std::recursive_mutex> lock(textMutex);

    std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> renderPassObjectMap;

    for (auto const &textTuple : texts) {
        for (auto config : textTuple.second->getRenderConfig()) {

            renderPassObjectMap[config->getRenderIndex()].push_back(std::make_shared<RenderObject>(config->getGraphicsObject()));
        }
    }

    std::vector<std::shared_ptr<RenderPassInterface>> newRenderPasses;
    for (const auto &passEntry : renderPassObjectMap) {
        std::shared_ptr<RenderPass> renderPass = std::make_shared<RenderPass>(RenderPassConfig(passEntry.first, false), passEntry.second);
        newRenderPasses.push_back(renderPass);
    }

    {
        std::lock_guard<std::recursive_mutex> overlayLock(renderPassMutex);
        renderPasses = newRenderPasses;
    }
}

void TextLayer::add(const std::shared_ptr<TextInfoInterface> &text) { addTexts({text}); }

void TextLayer::addTexts(const std::vector<std::shared_ptr<TextInfoInterface>> &texts) {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    auto scheduler = mapInterface ? mapInterface->getScheduler() : nullptr;
    if (!scheduler) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        for (const auto &text : texts) {
            addingQueue.insert(text);
        }
        return;
    }

    std::vector<std::tuple<const std::shared_ptr<TextInfoInterface>, std::shared_ptr<TextLayerObject>>> textObjects;

    auto textHelper = TextHelper(mapInterface);

    for (const auto &text : texts) {
        auto fontData = fontLoader->loadFont(text->getFont()).fontData;
        if(!fontData) {
          continue;
        }
        auto textObject = textHelper.textLayerObject(text, fontData, Vec2F(0.0, 0.0), 1.2, 0.0, 15, 45.0, SymbolAlignment::AUTO);

        if (textObject) {
            textObjects.push_back(std::make_tuple(text, textObject));
        }

        {
            std::lock_guard<std::recursive_mutex> lock(textMutex);
            this->texts[text] = textObject;
        }
    }

    std::weak_ptr<TextLayer> weakSelfPtr = std::dynamic_pointer_cast<TextLayer>(shared_from_this());
    std::string taskId =
        "TextLayer_setup_coll_" + std::get<0>(textObjects.at(0))->getText().begin()->text + "_[" + std::to_string(textObjects.size()) + "]";
    scheduler->addTask(std::make_shared<LambdaTask>(
        TaskConfig(taskId, 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS), [weakSelfPtr, textObjects] {
            auto selfPtr = weakSelfPtr.lock();
            if (selfPtr)
                selfPtr->setupTextObjects(textObjects);
        }));

    generateRenderPasses();

    if (mapInterface)
        mapInterface->invalidate();
}

void TextLayer::setupTextObjects(
    const std::vector<std::tuple<const std::shared_ptr<TextInfoInterface>, std::shared_ptr<TextLayerObject>>> &textObjects) {
    auto mapInterface = this->mapInterface;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) {
        return;
    }

    for (const auto &textTuple : textObjects) {
        const auto &text = std::get<0>(textTuple);
        const auto &textObject = std::get<1>(textTuple)->getTextObject();
        textObject->asGraphicsObject()->setup(renderingContext);

        auto font = fontLoader->loadFont(text->getFont());
        if (font.imageData) {
            textObject->loadTexture(renderingContext, font.imageData);
        }
    }

    mapInterface->invalidate();
}
