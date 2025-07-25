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
#include "GraphicsObjectInterface.h"
#include "LambdaTask.h"
#include "LineHelper.h"
#include "LineStyle.h"
#include "MapCameraInterface.h"
#include "MapInterface.h"
#include "RenderObject.h"
#include "RenderPass.h"
#include "SizeType.h"
#include <map>
#include <algorithm>

LineLayer::LineLayer()
    : isHidden(false){};

void LineLayer::setLines(const std::vector<std::shared_ptr<LineInfoInterface>> &lines) {
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

void LineLayer::remove(const std::shared_ptr<LineInfoInterface> &line) {
    auto mapInterface = this->mapInterface;
    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.erase(std::remove(addingQueue.begin(), addingQueue.end(), line), addingQueue.end());
        return;
    }
    {
        std::lock_guard<std::recursive_mutex> lock(linesMutex);
        for (auto it = lines.begin(); it != lines.end(); it++) {
            if (it->first->getIdentifier() == line->getIdentifier()) {
                auto linePtr = it->second;
                auto scheduler = mapInterface->getScheduler();
                if (scheduler) {
                    scheduler->addTask(std::make_shared<LambdaTask>(
                            TaskConfig("IconLayer_clearLine", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
                            [linePtr] {
                                if (linePtr) {
                                    linePtr->getLineObject()->clear();
                                }
                            }));
                }
                lines.erase(it);
                animatedLineStyleIds.erase(line->getIdentifier());
                hasAnimatedLineStyles = !animatedLineStyleIds.empty();
                break;
            }
        }
    }
    generateRenderPasses();
    mapInterface->invalidate();
}

void LineLayer::add(const std::shared_ptr<LineInfoInterface> &line) {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    auto objectFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
    auto shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
    auto scheduler = mapInterface ? mapInterface->getScheduler() : nullptr;
    if (!objectFactory || !shaderFactory || !scheduler) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.push_back(line);
        return;
    }

    auto shader = mapInterface->is3d() ? shaderFactory->createUnitSphereLineGroupShader() : shaderFactory->createLineGroupShader();
    auto lineGraphicsObject = objectFactory->createLineGroup(shader->asShaderProgramInterface());

    auto lineObject = std::make_shared<Line2dLayerObject>(mapInterface->getCoordinateConverterHelper(), lineGraphicsObject, shader, mapInterface->is3d());

    const auto &lineStyle = line->getStyle();
    lineObject->setStyle(lineStyle);
    bool isAnimatedLineStyle = lineStyle.dashAnimationSpeed > 0.0;

    bool is3d = mapInterface->is3d();

    auto origin = Vec3D(0.0, 0.0, 0.0);
    const auto& coords = line->getCoordinates();

    if(coords.size() > 0) {
        Coord renderCoord = mapInterface->getCoordinateConverterHelper()->convertToRenderSystem(coords[0]);
        double x = is3d ? renderCoord.z * sin(renderCoord.y) * cos(renderCoord.x) : renderCoord.x;
        double y = is3d ? renderCoord.z * cos(renderCoord.y) : renderCoord.y;
        double z = is3d ? -renderCoord.z * sin(renderCoord.y) * sin(renderCoord.x) : 0.0;
        origin = Vec3D(x,y,z);
    }

    lineObject->setPositions(coords, origin);

    std::weak_ptr<LineLayer> weakSelfPtr = std::dynamic_pointer_cast<LineLayer>(shared_from_this());
    scheduler->addTask(std::make_shared<LambdaTask>(
        TaskConfig("LineLayer_setup_" + line->getIdentifier(), 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
        [weakSelfPtr, lineGraphicsObject] {
            auto selfPtr = weakSelfPtr.lock();
            if (selfPtr)
                selfPtr->setupLine(lineGraphicsObject);
        }));

    {
        std::lock_guard<std::recursive_mutex> lock(linesMutex);
        lines.push_back(std::make_pair(line, lineObject));
        if (isAnimatedLineStyle) {
            animatedLineStyleIds.insert(line->getIdentifier());
            hasAnimatedLineStyles = true;
        }
    }
    generateRenderPasses();
}

void LineLayer::setupLine(const std::shared_ptr<LineGroup2dInterface> &line) {
    auto mapInterface = this->mapInterface;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) {
        return;
    }

    if (!line->asGraphicsObject()->isReady()) {
        line->asGraphicsObject()->setup(renderingContext);
    }
    if (mask && !mask->asGraphicsObject()->isReady()) {
        mask->asGraphicsObject()->setup(renderingContext);
    }
    mapInterface->invalidate();
}

void LineLayer::clear() {
    auto mapInterface = this->mapInterface;
    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.clear();
        return;
    }
    {
        std::lock_guard<std::recursive_mutex> lock(linesMutex);
        auto scheduler = mapInterface->getScheduler();
        if (scheduler) {
            auto linesToClear = lines;
            scheduler->addTask(std::make_shared<LambdaTask>(TaskConfig("LineLayer_clearLines", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS), [linesToClear]{
                for (const auto &line : linesToClear) {
                    if (line.second->getLineObject()->isReady()) {
                        line.second->getLineObject()->clear();
                    }
                }
            }));
        }
        lines.clear();
        animatedLineStyleIds.clear();
        hasAnimatedLineStyles = false;
    }
    generateRenderPasses();
    mapInterface->invalidate();
}

void LineLayer::setCallbackHandler(const std::shared_ptr<LineLayerCallbackInterface> &handler) { callbackHandler = handler; }

std::shared_ptr<::LayerInterface> LineLayer::asLayerInterface() { return shared_from_this(); }

void LineLayer::invalidate() { setLines(getLines()); }

void LineLayer::generateRenderPasses() {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    if (!mapInterface) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(linesMutex);
    std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> renderPassObjectMap;
    for (auto const &lineTuple : lines) {
        for (auto config : lineTuple.second->getRenderConfig()) {
            if (!lineTuple.first->getCoordinates().empty()) {
                std::vector<float> modelMatrix =
                    mapInterface->getCamera()->getInvariantModelMatrix(lineTuple.first->getCoordinates()[0], false, false);
                renderPassObjectMap[renderPassIndex].push_back(
                    std::make_shared<RenderObject>(config->getGraphicsObject()));
            }
        }
    }
    std::vector<std::shared_ptr<RenderPassInterface>> newRenderPasses;
    for (const auto &passEntry : renderPassObjectMap) {
        std::shared_ptr<RenderPass> renderPass =
            std::make_shared<RenderPass>(RenderPassConfig(passEntry.first, false, renderTarget), passEntry.second, mask);
        newRenderPasses.push_back(renderPass);
    }
    {
        std::lock_guard<std::recursive_mutex> overlayLock(renderPassMutex);
        renderPasses = newRenderPasses;
    }
}

void LineLayer::update() {
    const auto &mapInterface = this->mapInterface;
    if (mapInterface) {
        if (maskGraphicsObject && !maskGraphicsObject->isReady()) {
            maskGraphicsObject->setup(mapInterface->getRenderingContext());
        }
        if (hasAnimatedLineStyles) {
            mapInterface->invalidate();
        }

        auto camera = mapInterface->getCamera();
        if (camera) {
            auto cameraZoom = camera->getZoom();

            auto zoomIdentifier = std::max(0.0, std::round(log(500000000.0 * 1.0 / cameraZoom) / log(2) * 100) / 100);
            double factor = pow(2, floor(zoomIdentifier));
            auto zoom = 500000000.0 * 1.0 / factor;

            auto scalingFactor = (camera->asCameraInterface()->getScalingFactor() / cameraZoom) * zoom;

            for (auto const &line: lines) {
                line.second->setScalingFactor(scalingFactor);
            }
        }
    }
}

std::vector<std::shared_ptr<::RenderPassInterface>> LineLayer::buildRenderPasses() {
    if (isHidden) {
        return {};
    } else {
        std::lock_guard<std::recursive_mutex> overlayLock(renderPassMutex);
        return renderPasses;
    }
}

void LineLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface, int32_t layerIndex) {
    this->mapInterface = mapInterface;
    {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        for (auto const &line : addingQueue) {
            add(line);
        }
        addingQueue.clear();
    }
    if (isLayerClickable) {
        mapInterface->getTouchHandler()->insertListener(shared_from_this(), layerIndex);
    }
}

void LineLayer::onRemoved() {
    {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.clear();
    }

    if (mapInterface && isLayerClickable)
        mapInterface->getTouchHandler()->removeListener(shared_from_this());
    mapInterface = nullptr;
}

void LineLayer::pause() {
    std::lock_guard<std::recursive_mutex> overlayLock(linesMutex);
    for (const auto &line : lines) {
        line.second->getLineObject()->clear();
    }
    if (mask) {
        if (mask->asGraphicsObject()->isReady())
            mask->asGraphicsObject()->clear();
    }
}

void LineLayer::resume() {
    auto mapInterface = this->mapInterface;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) {
        return;
    }
    std::lock_guard<std::recursive_mutex> overlayLock(linesMutex);
    for (const auto &line : lines) {
        line.second->getLineObject()->setup(renderingContext);
    }
    if (maskGraphicsObject && !maskGraphicsObject->isReady()) {
        maskGraphicsObject->setup(renderingContext);
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

bool LineLayer::onClickUnconfirmed(const ::Vec2F &posScreen) {
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
            setSelected({line.first->getIdentifier()});
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

void LineLayer::setMaskingObject(const std::shared_ptr<::MaskingObjectInterface> &maskingObject) {
    this->mask = maskingObject;
    maskGraphicsObject = mask ? mask->asGraphicsObject() : nullptr;

    generateRenderPasses();
    auto mapInterface = this->mapInterface;
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void LineLayer::setLayerClickable(bool isLayerClickable) {
    if (this->isLayerClickable == isLayerClickable)
        return;
    this->isLayerClickable = isLayerClickable;
    if (mapInterface) {
        if (isLayerClickable) {
            mapInterface->getTouchHandler()->addListener(shared_from_this());
        } else {
            mapInterface->getTouchHandler()->removeListener(shared_from_this());
        }
    }
}

void LineLayer::resetSelection() {
    {
        std::lock_guard<std::recursive_mutex> lock(linesMutex);
        for (auto const &line : lines) {
            line.second->setHighlighted(false);
        }
    }
    if (mapInterface)
        mapInterface->invalidate();
}

void LineLayer::setSelected(const std::unordered_set<std::string> &selectedIds) {
    resetSelection();
    {
        std::lock_guard<std::recursive_mutex> lock(linesMutex);
        for (auto const &line : lines) {
            if (selectedIds.count(line.first->getIdentifier()) > 0) {
                line.second->setHighlighted(true);
            }
        }
    }
    if (mapInterface)
        mapInterface->invalidate();
}

void LineLayer::setRenderPassIndex(int32_t index) {
    renderPassIndex = index;
    generateRenderPasses();

    if (mapInterface) {
        mapInterface->invalidate();
    }
}
