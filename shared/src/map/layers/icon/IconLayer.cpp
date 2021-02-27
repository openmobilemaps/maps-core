/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include <map>
#include <IconType.h>
#include "IconLayer.h"
#include "LambdaTask.h"
#include "RenderPass.h"
#include "MapCamera2dInterface.h"
#include "CameraInterface.h"

IconLayer::IconLayer() : isHidden(false) {}

void IconLayer::setIcons(const std::vector<std::shared_ptr<IconInfoInterface>> &icons) {
    clear();
    for (auto const &icon : icons) {
        add(icon);
    }
    generateRenderPasses();
    if (mapInterface)
        mapInterface->invalidate();
}

std::vector<std::shared_ptr<IconInfoInterface>> IconLayer::getIcons() {
    std::vector<std::shared_ptr<IconInfoInterface>> icons;
    if (!mapInterface) {
        for (auto const &icon : addingQueue) {
            icons.push_back(icon);
        }
        return icons;
    }
    for (auto const &icon : this->icons) {
        icons.push_back(icon.first);
    }
    return icons;
}

void IconLayer::remove(const std::shared_ptr<IconInfoInterface> &icon) {
    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.erase(icon);
        return;
    }
    {
        std::lock_guard<std::recursive_mutex> lock(iconsMutex);
        for (auto it = icons.begin(); it != icons.end(); it++) {
            if (it->first->getIdentifier() == icon->getIdentifier()) {
                icons.erase(it);
                auto quadObject = it->second->getQuadObject();
                mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
                        TaskConfig("IconLayer_remove_" + icon->getIdentifier(), 0, TaskPriority::NORMAL,
                                   ExecutionEnvironment::GRAPHICS),
                        [=] { quadObject->asGraphicsObject()->clear(); }));
                break;
            }
        }
    }
    generateRenderPasses();
    if (mapInterface)
        mapInterface->invalidate();
}

void IconLayer::add(const std::shared_ptr<IconInfoInterface> &icon) {
    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.insert(icon);
        return;
    }

    const auto &objectFactory = mapInterface->getGraphicsObjectFactory();
    const auto &shaderFactory = mapInterface->getShaderFactory();

    auto shader = shaderFactory->createAlphaShader();
    auto quadObject = objectFactory->createQuad(shader->asShaderProgramInterface());

    auto iconObject = std::make_shared<Textured2dLayerObject>(quadObject, shader, mapInterface);

    Coord iconPosRender = mapInterface->getCoordinateConverterHelper()->convertToRenderSystem(icon->getCoordinate());
    float halfW = icon->getIconSize().x * 0.5f;
    float halfH = icon->getIconSize().y * 0.5f;
    iconObject->setRectCoord(
            RectCoord(Coord(iconPosRender.systemIdentifier, iconPosRender.x - halfW, iconPosRender.y - halfH, iconPosRender.y),
                      Coord(iconPosRender.systemIdentifier, iconPosRender.x + halfW, iconPosRender.y + halfH, iconPosRender.y)));

    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
            TaskConfig("IconLayer_setup_" + icon->getIdentifier(), 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
            [=] {
                quadObject->asGraphicsObject()->setup(mapInterface->getRenderingContext());
                quadObject->loadTexture(icon->getTexture());
            }));

    {
        std::lock_guard<std::recursive_mutex> lock(iconsMutex);
        icons[icon] = iconObject;
    }
    generateRenderPasses();
    if (mapInterface)
        mapInterface->invalidate();
}

void IconLayer::clear() {
    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.clear();
        return;
    }
    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
            TaskConfig("IconLayer_clear", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
            [=] {
                std::lock_guard<std::recursive_mutex> lock(iconsMutex);
                for (auto &icon : icons) {
                    icon.second->getQuadObject()->asGraphicsObject()->clear();
                }
                icons.clear();
            }));
    renderPasses.clear();
    if (mapInterface)
        mapInterface->invalidate();
}

void IconLayer::setCallbackHandler(const std::shared_ptr<IconLayerCallbackInterface> &handler) {
    this->callbackHandler = handler;
}

std::shared_ptr<::LayerInterface> IconLayer::asLayerInterface() {
    return shared_from_this();
}

void IconLayer::invalidate() {
    setIcons(getIcons());
}

std::vector<std::shared_ptr<::RenderPassInterface>> IconLayer::buildRenderPasses() {
    if (isHidden) {
        return {};
    } else {
        auto camera = mapInterface->getCamera();
        auto mvpMatrix = std::dynamic_pointer_cast<CameraInterface>(camera)->getMvpMatrix();
        int i = 0;
        std::unordered_map<int, std::vector<float>> transformSet;
        for (auto const &iconTuple : icons) {
            IconType type = iconTuple.first->getType();
            if (type != IconType::FIXED) {
                bool scaleInvariant = type == IconType::INVARIANT || type == IconType::SCALE_INVARIANT;
                bool rotationInvariant = type == IconType::INVARIANT || type == IconType::ROTATION_INVARIANT;
                transformSet[i] = camera->getInvariantMvpMatrix(mvpMatrix, iconTuple.first->getCoordinate(), scaleInvariant,
                                                                rotationInvariant);
            }
            i++;
        }
        for (const auto &pass : renderPasses) {
            pass->setCustomObjectTransforms(transformSet);
        }
        return renderPasses;
    }
}

void IconLayer::generateRenderPasses() {
    std::lock_guard<std::recursive_mutex> lock(iconsMutex);
    std::map<int, std::vector<std::shared_ptr<GraphicsObjectInterface>>> renderPassObjectMap;
    for (auto const &iconTuple : icons) {
        for (auto config : iconTuple.second->getRenderConfig()) {
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

void IconLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface) {
    this->mapInterface = mapInterface;
    {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        for (auto const &icon : addingQueue) {
            add(icon);
        }
        addingQueue.clear();
    }

    mapInterface->getTouchHandler()->addListener(shared_from_this());
}

void IconLayer::pause() {
    {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.clear();
        for (const auto &icon: icons) {
            addingQueue.insert(icon.first);
        }
    }
    clear();
}

void IconLayer::resume() {
    std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
    for (auto const &icon : addingQueue) {
        add(icon);
    }
    addingQueue.clear();
}

void IconLayer::hide() {
    isHidden = true;
    if (mapInterface)
        mapInterface->invalidate();
}

void IconLayer::show() {
    isHidden = false;
    if (mapInterface)
        mapInterface->invalidate();
}

bool IconLayer::onClickConfirmed(const Vec2F &posScreen) {
    if (callbackHandler) {
        // TODO: Check icon-hit
        //callbackHandler->onClickConfirmed()
        //return true;
    }
    return false;
}








