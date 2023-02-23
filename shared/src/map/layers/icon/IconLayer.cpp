/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "IconLayer.h"
#include "CameraInterface.h"
#include "LambdaTask.h"
#include "MapCamera2dInterface.h"
#include "RenderObject.h"
#include "RenderPass.h"
#include <IconType.h>

IconLayer::IconLayer()
    : isHidden(false) {}

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
    std::lock_guard<std::recursive_mutex> lock(iconsMutex);
    for (auto const &icon : this->icons) {
        icons.push_back(icon.first);
    }
    return icons;
}

void IconLayer::remove(const std::shared_ptr<IconInfoInterface> &icon) {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.erase(std::remove(addingQueue.begin(), addingQueue.end(), icon), addingQueue.end());
        return;
    }
    {
        std::lock_guard<std::recursive_mutex> lock(iconsMutex);
        for (auto it = icons.begin(); it != icons.end(); it++) {
            if (it->first->getIdentifier() == icon->getIdentifier()) {
                auto graphicsObject = it->second->getGraphicsObject();
                icons.erase(it);
                mapInterface->getScheduler()->addTask(
                    std::make_shared<LambdaTask>(TaskConfig("IconLayer_remove_" + icon->getIdentifier(), 0, TaskPriority::NORMAL,
                                                            ExecutionEnvironment::GRAPHICS),
                                                 [=] { graphicsObject->clear(); }));
                break;
            }
        }
    }
    preGenerateRenderPasses();
    if (mapInterface)
        mapInterface->invalidate();
}

void IconLayer::add(const std::shared_ptr<IconInfoInterface> &icon) { addIcons({icon}); }

void IconLayer::addIcons(const std::vector<std::shared_ptr<IconInfoInterface>> &icons) {
    if (icons.empty()) {
        return;
    }

    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        for (const auto &icon : icons) {
            addingQueue.push_back(icon);
        }
        return;
    }

    std::vector<std::tuple<const std::shared_ptr<IconInfoInterface>, std::shared_ptr<Textured2dLayerObject>>> iconObjects;

    for (const auto &icon : icons) {
        auto objectFactory = mapInterface->getGraphicsObjectFactory();
        auto shaderFactory = mapInterface->getShaderFactory();

        auto shader = shaderFactory->createAlphaShader();
        auto quadObject = objectFactory->createQuad(shader->asShaderProgramInterface());

        auto iconObject = std::make_shared<Textured2dLayerObject>(quadObject, shader, mapInterface);

        Coord iconPosRender = mapInterface->getCoordinateConverterHelper()->convertToRenderSystem(icon->getCoordinate());
        const Vec2F &anchor = icon->getIconAnchor();
        float ratioLeftRight = std::clamp(anchor.x, 0.0f, 1.0f);
        float ratioTopBottom = std::clamp(anchor.y, 0.0f, 1.0f);
        float leftW = icon->getIconSize().x * ratioLeftRight;
        float topH = icon->getIconSize().y * ratioTopBottom;
        float rightW = icon->getIconSize().x * (1.0f - ratioLeftRight);
        float bottomH = icon->getIconSize().y * (1.0f - ratioTopBottom);
        iconObject->setRectCoord(
            RectCoord(Coord(iconPosRender.systemIdentifier, iconPosRender.x - leftW, iconPosRender.y - topH, iconPosRender.y),
                      Coord(iconPosRender.systemIdentifier, iconPosRender.x + rightW, iconPosRender.y + bottomH, iconPosRender.y)));
        iconObjects.push_back(std::make_tuple(icon, iconObject));

        {
            std::lock_guard<std::recursive_mutex> lock(iconsMutex);
            this->icons.push_back(std::make_pair(icon, iconObject));
        }
    }

    std::weak_ptr<IconLayer> weakSelfPtr = std::dynamic_pointer_cast<IconLayer>(shared_from_this());
    std::string taskId =
        "IconLayer_setup_coll_" + std::get<0>(iconObjects.at(0))->getIdentifier() + "_[" + std::to_string(iconObjects.size()) + "]";
    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
        TaskConfig(taskId, 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS), [weakSelfPtr, iconObjects] {
            auto selfPtr = weakSelfPtr.lock();
            if (selfPtr)
                selfPtr->setupIconObjects(iconObjects);
        }));

    preGenerateRenderPasses();
    if (mapInterface)
        mapInterface->invalidate();
}

void IconLayer::setupIconObjects(
    const std::vector<std::tuple<const std::shared_ptr<IconInfoInterface>, std::shared_ptr<Textured2dLayerObject>>> &iconObjects) {
    auto mapInterface = this->mapInterface;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) {
        return;
    }

    for (const auto iconTuple : iconObjects) {
        const auto &icon = std::get<0>(iconTuple);
        const auto &iconObject = std::get<1>(iconTuple);

        iconObject->getGraphicsObject()->setup(renderingContext);
        iconObject->getQuadObject()->loadTexture(renderingContext, icon->getTexture());
        
        if (mask && !mask->asGraphicsObject()->isReady()) {
            mask->asGraphicsObject()->setup(renderingContext);
        }
    }

    mapInterface->invalidate();
}

void IconLayer::clear() {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.clear();
        return;
    }
    {
        std::lock_guard<std::recursive_mutex> lock(iconsMutex);
        auto iconsToClear = icons;
        mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
            TaskConfig("IconLayer_clear", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS), [=] {
                for (auto &icon : iconsToClear) {
                    icon.second->getGraphicsObject()->clear();
                }
            }));
        icons.clear();
    }
    if (mask) {
        if (mask->asGraphicsObject()->isReady())
            mask->asGraphicsObject()->clear();
    }
    renderPassObjectMap.clear();
    mapInterface->invalidate();
}

void IconLayer::setCallbackHandler(const std::shared_ptr<IconLayerCallbackInterface> &handler) { this->callbackHandler = handler; }

std::shared_ptr<::LayerInterface> IconLayer::asLayerInterface() { return shared_from_this(); }

void IconLayer::invalidate() { setIcons(getIcons()); }

void IconLayer::update() {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    if (mapInterface && mask) {
        if (!mask->asGraphicsObject()->isReady())
            mask->asGraphicsObject()->setup(mapInterface->getRenderingContext());
    }
}

std::vector<std::shared_ptr<::RenderPassInterface>> IconLayer::buildRenderPasses() {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!camera) {
        return {};
    }

    if (isHidden) {
        return {};
    } else {
        int i = 0;
        std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> currentRenderPassObjectMap;
        std::unordered_map<int, std::vector<float>> transformSet;
        {
            std::lock_guard<std::recursive_mutex> lock(iconsMutex);
            for (auto const &iconTuple : icons) {
                IconType type = iconTuple.first->getType();
                if (type != IconType::FIXED) {
                    bool scaleInvariant = type == IconType::INVARIANT || type == IconType::SCALE_INVARIANT;
                    bool rotationInvariant = type == IconType::INVARIANT || type == IconType::ROTATION_INVARIANT;
                    std::vector<float> modelMatrix =
                        camera->getInvariantModelMatrix(iconTuple.first->getCoordinate(), scaleInvariant, rotationInvariant);
                    for (const auto &config : iconTuple.second->getRenderConfig()) {
                        currentRenderPassObjectMap[config->getRenderIndex()].push_back(
                            std::make_shared<RenderObject>(config->getGraphicsObject(), modelMatrix));
                    }
                }
                i++;
            }

            for (auto const &passObjectEntry : renderPassObjectMap) {
                for (const auto &object : passObjectEntry.second) {
                    currentRenderPassObjectMap[passObjectEntry.first].push_back(object);
                }
            }
        }

        std::vector<std::shared_ptr<RenderPassInterface>> renderPasses;
        for (const auto &passEntry : currentRenderPassObjectMap) {
            std::shared_ptr<RenderPass> renderPass =
                std::make_shared<RenderPass>(RenderPassConfig(passEntry.first), passEntry.second, mask);
            renderPasses.push_back(renderPass);
        }
        return renderPasses;
    }
}

void IconLayer::preGenerateRenderPasses() {
    std::lock_guard<std::recursive_mutex> lock(iconsMutex);
    std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> newRenderPassObjectMap;
    for (auto const &iconTuple : icons) {
        if (iconTuple.first->getType() != IconType::FIXED)
            continue;
        for (const auto &config : iconTuple.second->getRenderConfig()) {
            newRenderPassObjectMap[config->getRenderIndex()].push_back(std::make_shared<RenderObject>(config->getGraphicsObject()));
        }
    }
    renderPassObjectMap = newRenderPassObjectMap;
}

void IconLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface, int32_t layerIndex) {
    this->mapInterface = mapInterface;
    {
        std::scoped_lock<std::recursive_mutex> lock(addingQueueMutex);
        if (!addingQueue.empty()) {
            std::vector<std::shared_ptr<IconInfoInterface>> icons;
            for (auto const &icon : addingQueue) {
                icons.push_back(icon);
            }
            addingQueue.clear();
            addIcons(icons);
        }
    }
    if (isLayerClickable) {
        mapInterface->getTouchHandler()->insertListener(shared_from_this(), layerIndex);
    }
}

void IconLayer::onRemoved() {
    {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.clear();
    }

    auto mapInterface = this->mapInterface;
    if (mapInterface && isLayerClickable) {
        mapInterface->getTouchHandler()->removeListener(shared_from_this());
    }
    this->mapInterface = nullptr;
}

void IconLayer::pause() {
    {
        std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> newRenderPassObjectMap;
        std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(addingQueueMutex, iconsMutex);
        addingQueue.clear();
        for (const auto &icon : icons) {
            addingQueue.push_back(icon.first);
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
    if (mask) {
        auto mapInterface = this->mapInterface;
        auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
        if (renderingContext && !mask->asGraphicsObject()->isReady())
            mask->asGraphicsObject()->setup(renderingContext);
    }
}

void IconLayer::hide() {
    isHidden = true;
    auto mapInterface = this->mapInterface;
    if (mapInterface)
        mapInterface->invalidate();
}

void IconLayer::show() {
    isHidden = false;
    auto mapInterface = this->mapInterface;
    if (mapInterface)
        mapInterface->invalidate();
}

bool IconLayer::onClickConfirmed(const Vec2F &posScreen) {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto conversionHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    if (!camera || !conversionHelper) {
        return false;
    }

    if (callbackHandler) {
        std::vector<std::shared_ptr<IconInfoInterface>> iconsHit;

        Coord clickCoords = camera->coordFromScreenPosition(posScreen);

        double angle = -(camera->getRotation() * M_PI / 180.0);
        double sinAng = std::sin(angle);
        double cosAng = std::cos(angle);

        {
            std::lock_guard<std::recursive_mutex> lock(iconsMutex);
            for (const auto &iconTuple : icons) {
                std::shared_ptr<IconInfoInterface> icon = iconTuple.first;

                const Vec2F &anchor = icon->getIconAnchor();
                float ratioLeftRight = std::clamp(anchor.x, 0.0f, 1.0f);
                float ratioTopBottom = std::clamp(anchor.y, 0.0f, 1.0f);
                float leftW = icon->getIconSize().x * ratioLeftRight;
                float topH = icon->getIconSize().y * ratioTopBottom;
                float rightW = icon->getIconSize().x * (1.0f - ratioLeftRight);
                float bottomH = icon->getIconSize().y * (1.0f - ratioTopBottom);

                Coord iconPos = conversionHelper->convert(clickCoords.systemIdentifier, icon->getCoordinate());
                IconType type = icon->getType();
                if (type == IconType::INVARIANT || type == IconType::SCALE_INVARIANT) {
                    leftW = camera->mapUnitsFromPixels(leftW);
                    topH = camera->mapUnitsFromPixels(topH);
                    rightW = camera->mapUnitsFromPixels(rightW);
                    bottomH = camera->mapUnitsFromPixels(bottomH);
                }

                Vec2D clickPos = Vec2D(clickCoords.x - iconPos.x, clickCoords.y - iconPos.y);
                if (type == IconType::INVARIANT || type == IconType::ROTATION_INVARIANT) {
                    float newX = cosAng * clickPos.x - sinAng * clickPos.y;
                    float newY = sinAng * clickPos.x + cosAng * clickPos.y;
                    clickPos.x = newX;
                    clickPos.y = newY;
                }
                if (clickPos.x > -leftW && clickPos.x < rightW && clickPos.y < topH && clickPos.y > -bottomH) {
                    iconsHit.push_back(icon);
                }
            }
        }

        if (!iconsHit.empty()) {
            return callbackHandler->onClickConfirmed(iconsHit);
        }
    }
    return false;
}

void IconLayer::setMaskingObject(const std::shared_ptr<::MaskingObjectInterface> &maskingObject) {
    this->mask = maskingObject;
    auto mapInterface = this->mapInterface;
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void IconLayer::setLayerClickable(bool isLayerClickable) {
    if (this->isLayerClickable == isLayerClickable)
        return;
    this->isLayerClickable = isLayerClickable;
    auto mapInterface = this->mapInterface;
    if (mapInterface) {
        if (isLayerClickable) {
            mapInterface->getTouchHandler()->addListener(shared_from_this());
        } else {
            mapInterface->getTouchHandler()->removeListener(shared_from_this());
        }
    }
}

void IconLayer::setAlpha(float alpha) {
    std::lock_guard<std::recursive_mutex> lock(iconsMutex);
    for (auto const &icon : this->icons) {
        icon.second->setAlpha(alpha);
    }
}

