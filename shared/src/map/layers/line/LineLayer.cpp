/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "LineLayer.h"
#include "ColorLineShaderInterface.h"
#include "GraphicsObjectInterface.h"
#include "LambdaTask.h"
#include "MapCamera2dInterface.h"
#include "MapInterface.h"
#include "RenderPass.h"
#include "RenderObject.h"
#include <map>
#include "LineStyle.h"
#include "LineHelper.h"
#include "SizeType.h"


LineLayer::LineLayer(): isHidden(false) {};


void LineLayer::setLines(const std::vector<std::shared_ptr<LineInfoInterface>> & lines) {
    clear();
    for (auto const &line : lines) {
        add(line);
    }
    generateRenderPasses();
    if (mapInterface)
        mapInterface->invalidate();
}

std::vector<std::shared_ptr<LineInfoInterface>> LineLayer::getLines() {
    std::vector<std::shared_ptr<LineInfoInterface>> lines;
    if (!mapInterface) {
        for (auto const &line : addingQueue) {
            lines.push_back(line);
        }
        return lines;
    }
    for (auto const &line : this->lines) {
        lines.push_back(line.first);
    }
    return lines;
}

void LineLayer::remove(const std::shared_ptr<LineInfoInterface> & line) {
    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.erase(line);
        return;
    }
    {
        std::lock_guard<std::recursive_mutex> lock(linesMutex);
        for (auto it = lines.begin(); it != lines.end(); it++) {
            if (it->first->getIdentifier() == line->getIdentifier()) {
                lines.erase(it);
                break;
            }
        }
    }
    generateRenderPasses();
    if (mapInterface)
        mapInterface->invalidate();
}

void LineLayer::add(const std::shared_ptr<LineInfoInterface> & line) {
    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.insert(line);
        return;
    }

    const auto &objectFactory = mapInterface->getGraphicsObjectFactory();
    const auto &shaderFactory = mapInterface->getShaderFactory();

    auto shader = shaderFactory->createColorLineShader();
    auto lineGraphicsObject = objectFactory->createLine(shader->asShaderProgramInterface());

    auto lineObject =
            std::make_shared<Line2dLayerObject>(mapInterface->getCoordinateConverterHelper(), lineGraphicsObject, shader);

    lineObject->setStyle(line->getStyle());

    lineObject->setPositions(line->getCoordinates());


    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
                                                                       TaskConfig("LineLayer_setup_" + line->getIdentifier(), 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
            [=] { lineGraphicsObject->asGraphicsObject()->setup(mapInterface->getRenderingContext()); }));

    {
        std::lock_guard<std::recursive_mutex> lock(linesMutex);
        lines[line] = lineObject;
    }
    generateRenderPasses();
    if (mapInterface)
        mapInterface->invalidate();
}

void LineLayer::clear() {
    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.clear();
        return;
    }
    {
        std::lock_guard<std::recursive_mutex> lock(linesMutex);
        lines.clear();
    }
    generateRenderPasses();
    if (mapInterface)
        mapInterface->invalidate();
}

void LineLayer::setCallbackHandler(const std::shared_ptr<LineLayerCallbackInterface> & handler) {
    callbackHandler = handler;
}

std::shared_ptr<::LayerInterface> LineLayer::asLayerInterface() {
    return shared_from_this();
}

void LineLayer::invalidate() {
    setLines(getLines());
}

void LineLayer::generateRenderPasses() {
    std::lock_guard<std::recursive_mutex> lock(linesMutex);
    std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> renderPassObjectMap;
    for (auto const &lineTuple : lines) {
        for (auto config : lineTuple.second->getRenderConfig()) {
            std::vector<float> modelMatrix = mapInterface->getCamera()->getInvariantModelMatrix(lineTuple.first->getCoordinates()[0], false,false);
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

std::vector<std::shared_ptr<::RenderPassInterface>> LineLayer::buildRenderPasses() {
    if (isHidden) {
        return {};
    } else {
        return renderPasses;
    }
}

void LineLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface) {
    this->mapInterface = mapInterface;
    {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        for (auto const &line : addingQueue) {
            add(line);
        }
        addingQueue.clear();
    }

    mapInterface->getTouchHandler()->addListener(shared_from_this());
}

void LineLayer::pause() {
    std::lock_guard<std::recursive_mutex> overlayLock(linesMutex);
    for (const auto &line : lines) {
        line.second->getLineObject()->clear();
    }
}

void LineLayer::resume() {
    std::lock_guard<std::recursive_mutex> overlayLock(linesMutex);
    for (const auto &line : lines) {
        line.second->getLineObject()->setup(mapInterface->getRenderingContext());
    }
}

void LineLayer::hide() {
    isHidden = true;
    if (mapInterface)
        mapInterface->invalidate();
}

void LineLayer::show() {
    isHidden = false;
    if (mapInterface)
        mapInterface->invalidate();
}

bool LineLayer::onTouchDown(const ::Vec2F &posScreen) {
    auto point = mapInterface->getCamera()->coordFromScreenPosition(posScreen);


    std::lock_guard<std::recursive_mutex> lock(linesMutex);
    for (auto const &line : lines) {

        auto distance = line.first->getStyle().width;

        if (line.first->getStyle().widthType == SizeType::SCREEN_PIXEL) {
            distance = mapInterface->getCamera()->mapUnitsFromPixels(distance);
        }

        if (LineHelper::pointWithin(line.first, point, distance, mapInterface->getCoordinateConverterHelper())) {
            line.second->setHighlighted(true);
            mapInterface->invalidate();
            return true;
        }
    }
    return false;
}

bool LineLayer::onClickConfirmed(const ::Vec2F &posScreen) {
    auto point = mapInterface->getCamera()->coordFromScreenPosition(posScreen);


    std::lock_guard<std::recursive_mutex> lock(linesMutex);
    for (auto const &line : lines) {

        auto distance = line.first->getStyle().width;

        if (line.first->getStyle().widthType == SizeType::SCREEN_PIXEL) {
            distance = mapInterface->getCamera()->mapUnitsFromPixels(distance);
        }

        if (LineHelper::pointWithin(line.first, point, distance, mapInterface->getCoordinateConverterHelper())) {
            line.second->setHighlighted(false);
            if (callbackHandler) {
                callbackHandler->onLineClickConfirmed(line.first);
            }
            mapInterface->invalidate();
            return true;
        }
    }
    return false;
}

void LineLayer::clearTouch() {
    {
        std::lock_guard<std::recursive_mutex> lock(linesMutex);
        for (auto const &line : lines) {
            line.second->setHighlighted(false);
        }
    }
    mapInterface->invalidate();
}


void LineLayer::setMaskingObject(const std::shared_ptr<::MaskingObjectInterface> & maskingObject) {}
