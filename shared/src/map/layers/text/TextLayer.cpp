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

#include "LambdaTask.h"
#include "AlphaShaderInterface.h"
#include "RectCoord.h"
#include "TextDescription.h"
#include "GlyphDescription.h"
#include "FontLoaderResult.h"
#include "RenderObjectInterface.h"
#include "RenderPass.h"
#include "RenderObject.h"
#include "TextShaderInterface.h"
#include "TextHelper.h"

TextLayer::TextLayer(const std::shared_ptr<::FontLoaderInterface> & fontLoader)
: fontLoader(fontLoader),
  mapInterface(nullptr),
  isHidden(false)
{
    
}

void TextLayer::setTexts(const std::vector<std::shared_ptr<TextInfoInterface>> & texts) {
    clear();
    for (auto const &text : texts) {
        add(text);
    }
    generateRenderPasses();
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

std::shared_ptr<::LayerInterface> TextLayer::asLayerInterface() {
    return shared_from_this();
}

void TextLayer::invalidate() {
    setTexts(getTexts());
}

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

void TextLayer::setMaskingObject(const std::shared_ptr<::MaskingObjectInterface> & maskingObject) {
    // TODO.
}

std::vector<std::shared_ptr<::RenderPassInterface>> TextLayer::buildRenderPasses() {
    if (isHidden) {
        return {};
    } else {
        return renderPasses;
    }
}

void TextLayer::onAdded(const std::shared_ptr<MapInterface> & mapInterface) {
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
    pause();
}

void TextLayer::pause() {
    {
        std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(addingQueueMutex, textMutex);
        addingQueue.clear();
        for (const auto &text : texts) {
            addingQueue.insert(text.first);
        }
    }

    clear();
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
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void TextLayer::show() {
    isHidden = false;
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void TextLayer::clear() {
    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.clear();
        return;
    }

    {
        std::lock_guard<std::recursive_mutex> lock(textMutex);
        auto textsToClear = texts;
        mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
                TaskConfig("TextLayer_clear", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
                [=] {
                    for (auto &text : textsToClear) {
                        text.second->getTextObject()->asGraphicsObject()->clear();
                    }
                }));
        texts.clear();
    }

    renderPasses.clear();
    if (mapInterface) {
        mapInterface->invalidate();
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
        std::shared_ptr<RenderPass> renderPass = std::make_shared<RenderPass>(RenderPassConfig(passEntry.first), passEntry.second);
        newRenderPasses.push_back(renderPass);
    }

    renderPasses = newRenderPasses;
}


void TextLayer::add(const std::shared_ptr<TextInfoInterface> &text) {
    addTexts({ text });
}


void TextLayer::addTexts(const std::vector<std::shared_ptr<TextInfoInterface>> &texts) {
    if (!mapInterface) {
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
        auto textObject = textHelper.textLayer(text, fontData, Vec2F(0.0,0.0));

        if(textObject) {
            textObjects.push_back(std::make_tuple(text, textObject));
        }

        {
            std::lock_guard<std::recursive_mutex> lock(textMutex);
            this->texts[text] = textObject;
        }
    }

    std::string taskId =
            "TextLayer_setup_coll_" + std::get<0>(textObjects.at(0))->getText() + "_[" + std::to_string(textObjects.size()) +
            "]";
    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
            TaskConfig(taskId, 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
            [=] {
                for (const auto& textTuple : textObjects) {
                    const auto &text = std::get<0>(textTuple);
                    const auto &textObject = std::get<1>(textTuple)->getTextObject();
                    textObject->asGraphicsObject()->setup(mapInterface->getRenderingContext());

                    auto font = fontLoader->loadFont(text->getFont());
                    if(font.imageData) {
                        textObject->loadTexture(font.imageData);
                    }
                }
            }));

    generateRenderPasses();

    if (mapInterface)
        mapInterface->invalidate();
}
