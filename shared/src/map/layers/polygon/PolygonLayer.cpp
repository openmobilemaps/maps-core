/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "PolygonLayer.h"
#include "ColorShaderInterface.h"
#include "GraphicsObjectInterface.h"
#include "LambdaTask.h"
#include "MapCamera2dInterface.h"
#include "MapInterface.h"
#include "PolygonHelper.h"
#include "RenderPass.h"
#include "RenderObject.h"
#include <map>

PolygonLayer::PolygonLayer()
        : isHidden(false) {}

void PolygonLayer::setPolygons(const std::vector<PolygonInfo> &polygons) {
    clear();
    for (auto const &polygon : polygons) {
        add(polygon);
    }
    generateRenderPasses();
    if (mapInterface)
        mapInterface->invalidate();
}

std::vector<PolygonInfo> PolygonLayer::getPolygons() {
    std::vector<PolygonInfo> polygons;
    if (!mapInterface) {
        for (auto const &polygon : addingQueue) {
            polygons.push_back(polygon);
        }
        return polygons;
    }
    for (auto const &polygon : this->polygons) {
        polygons.push_back(polygon.first);
    }
    return polygons;
}

void PolygonLayer::remove(const PolygonInfo &polygon) {
    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.erase(polygon);
        return;
    }
    {
        std::lock_guard<std::recursive_mutex> lock(polygonsMutex);
        for (auto it = polygons.begin(); it != polygons.end(); it++) {
            if (it->first.identifier == polygon.identifier) {
                polygons.erase(it);
                break;
            }
        }
    }
    generateRenderPasses();
    if (mapInterface)
        mapInterface->invalidate();
}

void PolygonLayer::add(const PolygonInfo &polygon) {
    addAll({polygon});
}


void PolygonLayer::addAll(const std::vector<PolygonInfo> &polygons) {
    if (polygons.empty()) return;

    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        for (const auto &polygon : polygons) {
            addingQueue.insert(polygon);
        }
        return;
    }

    const auto &objectFactory = mapInterface->getGraphicsObjectFactory();
    const auto &shaderFactory = mapInterface->getShaderFactory();

    std::vector<std::shared_ptr<Polygon2dInterface>> polygonGraphicObjects;

    {
        std::lock_guard<std::recursive_mutex> lock(polygonsMutex);
        for (const auto &polygon : polygons) {
            auto shader = shaderFactory->createColorShader();
            auto polygonGraphicsObject = objectFactory->createPolygon(shader->asShaderProgramInterface());

            auto polygonObject =
                    std::make_shared<Polygon2dLayerObject>(mapInterface->getCoordinateConverterHelper(), polygonGraphicsObject,
                                                           shader);

            polygonObject->setPositions(polygon.coordinates, polygon.holes, polygon.isConvex);
            polygonObject->setColor(polygon.color);

            polygonGraphicObjects.push_back(polygonGraphicsObject);
            this->polygons[polygon] = polygonObject;
        }
    }

    std::weak_ptr<PolygonLayer> selfPtr = std::dynamic_pointer_cast<PolygonLayer>(shared_from_this());
    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
            TaskConfig("PolygonLayer_setup_" + polygons[0].identifier + ",...", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
            [selfPtr, polygonGraphicObjects] { if (selfPtr.lock()) selfPtr.lock()->setupPolygons(polygonGraphicObjects); }));

    generateRenderPasses();

}

void PolygonLayer::setupPolygons(const std::vector<std::shared_ptr<Polygon2dInterface>> &polygons) {
    for (const auto &polygonGraphicsObject : polygons) {
        if (!polygonGraphicsObject->asGraphicsObject()->isReady()) {
            polygonGraphicsObject->asGraphicsObject()->setup(mapInterface->getRenderingContext());
        }
    }
    if (mapInterface) mapInterface->invalidate();
}

void PolygonLayer::clear() {
    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.clear();
        return;
    }
    {
        std::lock_guard<std::recursive_mutex> lock(polygonsMutex);
        polygons.clear();
    }
    generateRenderPasses();
    if (mapInterface)
        mapInterface->invalidate();
}

void PolygonLayer::pause() {
    std::lock_guard<std::recursive_mutex> overlayLock(polygonsMutex);
    for (const auto &polygon : polygons) {
        polygon.second->getPolygonObject()->clear();
    }
}

void PolygonLayer::resume() {
    std::lock_guard<std::recursive_mutex> overlayLock(polygonsMutex);
    for (const auto &polygon : polygons) {
        polygon.second->getPolygonObject()->setup(mapInterface->getRenderingContext());
    }
}

std::shared_ptr<::LayerInterface> PolygonLayer::asLayerInterface() { return shared_from_this(); }

void PolygonLayer::generateRenderPasses() {
    std::lock_guard<std::recursive_mutex> lock(polygonsMutex);
    std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> renderPassObjectMap;
    for (auto const &polygonTuple : polygons) {
        for (auto config : polygonTuple.second->getRenderConfig()) {
            renderPassObjectMap[config->getRenderIndex()].push_back(std::make_shared<RenderObject>(config->getGraphicsObject()));
        }
    }
    std::vector<std::shared_ptr<RenderPassInterface>> newRenderPasses;
    for (const auto &passEntry : renderPassObjectMap) {
        std::shared_ptr<RenderPass> renderPass = std::make_shared<RenderPass>(RenderPassConfig(passEntry.first), passEntry.second, mask);
        newRenderPasses.push_back(renderPass);
    }
    renderPasses = newRenderPasses;
}

std::vector<std::shared_ptr<::RenderPassInterface>> PolygonLayer::buildRenderPasses() {
    if (isHidden) {
        return {};
    } else {
        return renderPasses;
    }
}

void PolygonLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface) {
    this->mapInterface = mapInterface;
    {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        for (auto const &polygon : addingQueue) {
            add(polygon);
        }
        addingQueue.clear();
    }

    mapInterface->getTouchHandler()->addListener(shared_from_this());
}

void PolygonLayer::setCallbackHandler(const std::shared_ptr<PolygonLayerCallbackInterface> &handler) { callbackHandler = handler; }

void PolygonLayer::hide() {
    isHidden = true;
    if (mapInterface)
        mapInterface->invalidate();
}

void PolygonLayer::show() {
    isHidden = false;
    if (mapInterface)
        mapInterface->invalidate();
}

bool PolygonLayer::onTouchDown(const ::Vec2F &posScreen) {
    auto point = mapInterface->getCamera()->coordFromScreenPosition(posScreen);

    std::lock_guard<std::recursive_mutex> lock(polygonsMutex);
    for (auto const &polygon : polygons) {
        if (PolygonHelper::pointInside(polygon.first, point, mapInterface->getCoordinateConverterHelper())) {
            polygon.second->setColor(polygon.first.highlightColor);
            highlightedPolygon = polygon.first;
            mapInterface->invalidate();
            return true;
        }
    }
    return false;
}

void PolygonLayer::clearTouch() {
    if (highlightedPolygon) {
        {
            std::lock_guard<std::recursive_mutex> lock(polygonsMutex);
            polygons[*highlightedPolygon]->setColor(highlightedPolygon->color);
        }

        highlightedPolygon = std::nullopt;
        mapInterface->invalidate();
    }
}

bool PolygonLayer::onClickUnconfirmed(const ::Vec2F &posScreen) {
    if (highlightedPolygon) {
        {
            std::lock_guard<std::recursive_mutex> lock(polygonsMutex);
            polygons[*highlightedPolygon]->setColor(highlightedPolygon->color);
        }

        if (callbackHandler) {
            callbackHandler->onClickConfirmed(*highlightedPolygon);
        }

        highlightedPolygon = std::nullopt;
        mapInterface->invalidate();
        return true;
    }
    return false;
}


void PolygonLayer::setMaskingObject(const std::shared_ptr<::MaskingObjectInterface> & maskingObject) {
    this->mask = maskingObject;
    generateRenderPasses();
    if (mapInterface) mapInterface->invalidate();
}
