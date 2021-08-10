/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include <IconType.h>
#include "IconLayer.h"
#include "LambdaTask.h"
#include "RenderPass.h"
#include "RenderObject.h"
#include "MapCamera2dInterface.h"
#include "CameraInterface.h"

IconLayer::IconLayer() : isHidden(false) {}

void IconLayer::setIcons(const std::vector<std::shared_ptr<IconInfoInterface>> &icons) {
    clear();
    addIcons(icons);
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
                auto quadObject = it->second->getQuadObject();
                icons.erase(it);
                mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
                        TaskConfig("IconLayer_remove_" + icon->getIdentifier(), 0, TaskPriority::NORMAL,
                                   ExecutionEnvironment::GRAPHICS),
                        [=] { quadObject->asGraphicsObject()->clear(); }));
                break;
            }
        }
    }
    preGenerateRenderPasses();
    if (mapInterface)
        mapInterface->invalidate();
}

void IconLayer::add(const std::shared_ptr<IconInfoInterface> &icon) {
    addIcons({icon});
}


void IconLayer::addIcons(const std::vector<std::shared_ptr<IconInfoInterface>> &icons) {
    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        for (const auto &icon : icons) {
            addingQueue.insert(icon);
        }
        return;
    }

    std::vector<std::tuple<const std::shared_ptr<IconInfoInterface>, std::shared_ptr<Textured2dLayerObject>>> iconObjects;

    for (const auto &icon : icons) {
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
                          Coord(iconPosRender.systemIdentifier, iconPosRender.x + halfW, iconPosRender.y + halfH,
                                iconPosRender.y)));
        iconObjects.push_back(std::make_tuple(icon, iconObject));

        {
            std::lock_guard<std::recursive_mutex> lock(iconsMutex);
            this->icons[icon] = iconObject;
        }
    }

    std::string taskId =
            "IconLayer_setup_coll_" + std::get<0>(iconObjects.at(0))->getIdentifier() + "_[" + std::to_string(iconObjects.size()) +
            "]";
    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
            TaskConfig(taskId, 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
            [=] {
                for (const auto iconTuple : iconObjects) {
                    const auto &icon = std::get<0>(iconTuple);
                    const auto &quadObject = std::get<1>(iconTuple)->getQuadObject();
                    quadObject->asGraphicsObject()->setup(mapInterface->getRenderingContext());
                    quadObject->loadTexture(icon->getTexture());
                }
            }));

    preGenerateRenderPasses();
    if (mapInterface)
        mapInterface->invalidate();
}


void IconLayer::clear() {
    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.clear();
        return;
    }
    auto iconsToClear = icons;
    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
            TaskConfig("IconLayer_clear", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
            [=] {
                for (auto &icon : iconsToClear) {
                    icon.second->getQuadObject()->asGraphicsObject()->clear();
                }
            }));
    icons.clear();
    renderPassObjectMap.clear();
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

void IconLayer::invalidateIcon(const std::shared_ptr<IconInfoInterface> & icon) {
    {
        std::lock_guard<std::recursive_mutex> lock(iconsMutex);
        for (const auto &existingIcon : icons) {
            if (existingIcon.first->getIdentifier() != icon->getIdentifier()) {
                continue;
            }

            Coord iconPosRender = mapInterface->getCoordinateConverterHelper()->convertToRenderSystem(icon->getCoordinate());
            float halfW = icon->getIconSize().x * 0.5f;
            float halfH = icon->getIconSize().y * 0.5f;
            existingIcon.second->setRectCoord(
                    RectCoord(Coord(iconPosRender.systemIdentifier, iconPosRender.x - halfW, iconPosRender.y - halfH, iconPosRender.y),
                              Coord(iconPosRender.systemIdentifier, iconPosRender.x + halfW, iconPosRender.y + halfH,
                                    iconPosRender.y)));
            std::string taskId =
                    "IconLayer_invalidate_icon_" + icon->getIdentifier();
            mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
                    TaskConfig(taskId, 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
                    [=] {
                        existingIcon.second->getQuadObject()->asGraphicsObject()->clear();
                        existingIcon.second->getQuadObject()->asGraphicsObject()->setup(mapInterface->getRenderingContext());
                        existingIcon.second->getQuadObject()->loadTexture(icon->getTexture());
                    }));
        }
    }

    preGenerateRenderPasses();
    if (mapInterface)
        mapInterface->invalidate();
}

std::vector<std::shared_ptr<::RenderPassInterface>> IconLayer::buildRenderPasses() {
    if (isHidden) {
        return {};
    } else {
        auto camera = mapInterface->getCamera();
        int i = 0;
        std::lock_guard<std::recursive_mutex> lock(iconsMutex);
        std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> currentRenderPassObjectMap;
        std::unordered_map<int, std::vector<float>> transformSet;
        for (auto const &iconTuple : icons) {
            IconType type = iconTuple.first->getType();
            if (type != IconType::FIXED) {
                bool scaleInvariant = type == IconType::INVARIANT || type == IconType::SCALE_INVARIANT;
                bool rotationInvariant = type == IconType::INVARIANT || type == IconType::ROTATION_INVARIANT;
                std::vector<float> modelMatrix = camera->getInvariantModelMatrix(iconTuple.first->getCoordinate(), scaleInvariant,
                                                                                 rotationInvariant);
                for (const auto &config : iconTuple.second->getRenderConfig()) {
                    currentRenderPassObjectMap[config->getRenderIndex()].push_back(
                            std::make_shared<RenderObject>(config->getGraphicsObject(), modelMatrix));
                }
            }
            i++;
        }

        for (auto const &passObjectEntry : renderPassObjectMap) {
            for (const auto &object: passObjectEntry.second) {
                currentRenderPassObjectMap[passObjectEntry.first].push_back(object);
            }
        }

        std::vector<std::shared_ptr<RenderPassInterface>> renderPasses;
        for (const auto &passEntry : currentRenderPassObjectMap) {
            std::shared_ptr<RenderPass> renderPass = std::make_shared<RenderPass>(RenderPassConfig(passEntry.first),
                                                                                  passEntry.second);
            renderPasses.push_back(renderPass);
        }
        return renderPasses;
    }
}

void IconLayer::preGenerateRenderPasses() {
    std::lock_guard<std::recursive_mutex> lock(iconsMutex);
    std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> newRenderPassObjectMap;
    for (auto const &iconTuple : icons) {
        if (iconTuple.first->getType() != IconType::FIXED) continue;
        for (const auto &config : iconTuple.second->getRenderConfig()) {
            newRenderPassObjectMap[config->getRenderIndex()].push_back(
                    std::make_shared<RenderObject>(config->getGraphicsObject()));
        }
    }
    renderPassObjectMap = newRenderPassObjectMap;
}

void IconLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface) {
    this->mapInterface = mapInterface;
    {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        if (!addingQueue.empty()) {
            std::vector<std::shared_ptr<IconInfoInterface>> icons;
            for (auto const &icon : addingQueue) {
                icons.push_back(icon);
            }
            addingQueue.clear();
            addIcons(icons);
        }
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
    if (!addingQueue.empty()) {
        std::vector<std::shared_ptr<IconInfoInterface>> icons;
        for (auto const &icon : addingQueue) {
            icons.push_back(icon);
        }
        addingQueue.clear();
        addIcons(icons);
    }
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
        std::shared_ptr<MapCamera2dInterface> camera = mapInterface->getCamera();
        std::vector<std::shared_ptr<IconInfoInterface>> iconsHit;

        Coord clickCoords = camera->coordFromScreenPosition(posScreen);

        double angle = (camera->getRotation() * M_PI / 180.0);
        double sinAng = std::sin(angle);
        double cosAng = std::cos(angle);

        for (const auto &iconTuple : icons) {
            std::shared_ptr<IconInfoInterface> icon = iconTuple.first;

            double halfW = icon->getIconSize().x * 0.5f;
            double halfH = icon->getIconSize().y * 0.5f;
            Coord iconPos = mapInterface->getCoordinateConverterHelper()->convert(clickCoords.systemIdentifier,
                                                                                  icon->getCoordinate());
            IconType type = icon->getType();
            if (type == IconType::INVARIANT || type == IconType::SCALE_INVARIANT) {
                halfW = camera->mapUnitsFromPixels(halfW);
                halfH = camera->mapUnitsFromPixels(halfH);
            }

            Vec2D clickPos = Vec2D(clickCoords.x - iconPos.x, clickCoords.y - iconPos.y);
            if (type == IconType::INVARIANT || type == IconType::ROTATION_INVARIANT) {
                clickPos.x = cosAng * clickPos.x - sinAng * clickPos.y;
                clickPos.y = sinAng * clickPos.y + cosAng * clickPos.x;
            }
            if (clickPos.x > -halfW && clickPos.x < halfW &&
                clickPos.y > -halfH && clickPos.y < halfH) {
                iconsHit.push_back(icon);
            }
        }

        if (!iconsHit.empty()) {
            return callbackHandler->onClickConfirmed(iconsHit);
        }
    }
    return false;
}







