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
    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.insert(polygon);
        return;
    }

    const auto &objectFactory = mapInterface->getGraphicsObjectFactory();
    const auto &shaderFactory = mapInterface->getShaderFactory();

    auto shader = shaderFactory->createColorShader();
    auto polygonGraphicsObject = objectFactory->createPolygon(shader->asShaderProgramInterface());

    auto polygonObject =
        std::make_shared<Polygon2dLayerObject>(mapInterface->getCoordinateConverterHelper(), polygonGraphicsObject, shader);

    polygonObject->setPositions(polygon.coordinates, polygon.holes, polygon.isConvex);
    polygonObject->setColor(polygon.color);

    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
        TaskConfig("PolygonLayer_setup_" + polygon.identifier, 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
        [=] { polygonGraphicsObject->asGraphicsObject()->setup(mapInterface->getRenderingContext()); }));

    {
        std::lock_guard<std::recursive_mutex> lock(polygonsMutex);
        polygons[polygon] = polygonObject;
    }
    generateRenderPasses();
    if (mapInterface)
        mapInterface->invalidate();
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
    std::map<int, std::vector<std::shared_ptr<GraphicsObjectInterface>>> renderPassObjectMap;
    for (auto const &polygonTuple : polygons) {
        for (auto config : polygonTuple.second->getRenderConfig()) {
            renderPassObjectMap[config->getRenderIndex()].push_back(config->getGraphicsObject());
        }
    }
    std::vector<std::shared_ptr<RenderPassInterface>> newRenderPasses;
    for (const auto &passEntry : renderPassObjectMap) {
        std::shared_ptr<RenderPass> renderPass = std::make_shared<RenderPass>(RenderPassConfig(passEntry.first), passEntry.second);
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
