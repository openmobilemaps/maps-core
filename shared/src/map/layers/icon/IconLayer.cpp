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
#include "BlendMode.h"
#include "Logger.h"
#include "CoordinateSystemIdentifiers.h"
#include "Matrix.h"
#include <IconType.h>

IconLayer::IconLayer()
    : isHidden(false) {}

void IconLayer::setIcons(const std::vector<std::shared_ptr<IconInfoInterface>> &icons) {
    clear();
    addIcons(icons);
    setAlpha(this->alpha);
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
    removeIdentifier(icon->getIdentifier());
}


void IconLayer::removeList(const std::vector<std::shared_ptr<IconInfoInterface>> &iconsToRemove) {
    std::unordered_set<std::string> identifierSet;
    std::transform(iconsToRemove.begin(), iconsToRemove.end(), std::inserter(identifierSet, identifierSet.begin()),
                   [](const auto &iconInfo) {
                       return iconInfo->getIdentifier();
                   });
    removeIdentifierSet(identifierSet);
}

void IconLayer::removeIdentifier(const std::string &identifier) {
    removeIdentifierSet({identifier});
}

void IconLayer::removeIdentifierList(const std::vector<std::string> &identifiers) {
    std::unordered_set<std::string> identifierSet;
    identifierSet.insert(identifiers.begin(), identifiers.end());
    removeIdentifierSet(identifierSet);
}

void IconLayer::removeIdentifierSet(const std::unordered_set<std::string> &identifiersToRemove) {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    auto scheduler = mapInterface ? mapInterface->getScheduler() : nullptr;
    if (!mapInterface) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        for (auto it = this->addingQueue.begin(); it != this->addingQueue.end();) {
            if (identifiersToRemove.find(it->get()->getIdentifier()) != identifiersToRemove.end()) {
                it = addingQueue.erase(it);
            } else {
                it++;
            }
        }
        return;
    }

    if (!scheduler) {
        return;
    }
    std::vector<std::shared_ptr<GraphicsObjectInterface>> iconsToClear;
    {
        std::lock_guard<std::recursive_mutex> lock(iconsMutex);
        for (auto it = this->icons.begin(); it != this->icons.end();) {
            if (identifiersToRemove.find(it->first->getIdentifier()) != identifiersToRemove.end()) {
                auto graphicsObject = it->second->getGraphicsObject();
                if (graphicsObject->isReady()) {
                    iconsToClear.push_back(graphicsObject);
                }
                it = this->icons.erase(it);
            } else {
                it++;
            }
        }
    }
    if (!iconsToClear.empty()) {
        scheduler->addTask(
                std::make_shared<LambdaTask>(
                        TaskConfig("IconLayer_remove_" + std::to_string(iconsToClear.size()), 0, TaskPriority::NORMAL,
                                   ExecutionEnvironment::GRAPHICS),
                        [iconsToClear] {
                            for (const auto &graphicsObject: iconsToClear) {
                                graphicsObject->clear();
                            }
                        }));
    }

    preGenerateRenderPasses();
    if (mapInterface)
        mapInterface->invalidate();
}

void IconLayer::add(const std::shared_ptr<IconInfoInterface> &icon) {
    addIcons({icon});
    setAlpha(this->alpha);
}

void IconLayer::addList(const std::vector<std::shared_ptr<IconInfoInterface>> &iconsToAdd) {
    addIcons(iconsToAdd);
    setAlpha(this->alpha);
}

void IconLayer::addIcons(const std::vector<std::shared_ptr<IconInfoInterface>> &iconsToAdd) {
    if (iconsToAdd.empty()) {
        return;
    }

    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    auto objectFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
    auto shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
    auto conversionHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    auto scheduler = mapInterface ? mapInterface->getScheduler() : nullptr;
    if (!objectFactory || !shaderFactory || !scheduler) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        for (const auto &icon : iconsToAdd) {
            addingQueue.push_back(icon);
        }
        return;
    }

    std::vector<std::pair<std::shared_ptr<IconInfoInterface>, std::shared_ptr<Textured2dLayerObject>>> iconObjects;

    for (const auto &icon : iconsToAdd) {
        auto shader = shaderFactory->createAlphaShader();
        shader->asShaderProgramInterface()->setBlendMode(icon->getBlendMode());
        auto quadObject = objectFactory->createQuad(shader->asShaderProgramInterface());

        auto iconObject = std::make_shared<Textured2dLayerObject>(quadObject, shader, mapInterface);

        updateIconPosition(conversionHelper, icon, iconObject);
        iconObjects.push_back(std::make_pair(icon, iconObject));

        {
            std::lock_guard<std::recursive_mutex> lock(iconsMutex);
            this->icons.push_back(std::make_pair(icon, iconObject));
        }
    }

    std::weak_ptr<IconLayer> weakSelfPtr = std::dynamic_pointer_cast<IconLayer>(shared_from_this());
    std::string taskId =
        "IconLayer_setup_coll_" + std::get<0>(iconObjects.at(0))->getIdentifier() + "_[" + std::to_string(iconObjects.size()) + "]";
    scheduler->addTask(std::make_shared<LambdaTask>(
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
    const std::vector<std::pair<std::shared_ptr<IconInfoInterface>, std::shared_ptr<Textured2dLayerObject>>> &iconObjects) {
    auto mapInterface = this->mapInterface;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) {
        return;
    }

    for (const auto iconPair : iconObjects) {
        const auto &icon = iconPair.first;
        const auto &iconObject = iconPair.second;

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
    auto scheduler = mapInterface ? mapInterface->getScheduler() : nullptr;
    if (!scheduler) {
        std::lock_guard<std::recursive_mutex> lock(addingQueueMutex);
        addingQueue.clear();
        return;
    }
    {
        std::lock_guard<std::recursive_mutex> lock(iconsMutex);
        std::weak_ptr<IconLayer> weakSelfPtr = std::dynamic_pointer_cast<IconLayer>(shared_from_this());
        auto iconsToClear = icons;
        scheduler->addTask(std::make_shared<LambdaTask>(
            TaskConfig("IconLayer_clear", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS), [weakSelfPtr, iconsToClear] {
                if (auto self = weakSelfPtr.lock()) {
                    self->clearSync(iconsToClear);
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

void IconLayer::clearSync(const std::vector<std::pair<std::shared_ptr<IconInfoInterface>, std::shared_ptr<Textured2dLayerObject>>> &iconsToClear) {
    for (const auto &icon : iconsToClear) {
        if (icon.second->getGraphicsObject()->isReady()) {
            icon.second->getGraphicsObject()->clear();
        }
    }
}

void IconLayer::setCallbackHandler(const std::shared_ptr<IconLayerCallbackInterface> &handler) { this->callbackHandler = handler; }

std::shared_ptr<::LayerInterface> IconLayer::asLayerInterface() { return shared_from_this(); }

void IconLayer::invalidate() { setIcons(getIcons()); }

void IconLayer::update() {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    auto conversionHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!conversionHelper || !renderingContext) {
        return;
    }

    if (mask) {
        if (!mask->asGraphicsObject()->isReady())
            mask->asGraphicsObject()->setup(mapInterface->getRenderingContext());
    }

    {
        std::lock_guard<std::recursive_mutex> lock(iconsMutex);
        for (const auto &[iconInfo, layerObject]: icons) {
            if (iconInfo.get()->getType() != IconType::FIXED) {
                continue;
            }
            updateIconPosition(conversionHelper, iconInfo, layerObject);
            if (!layerObject->getGraphicsObject()->isReady()) {
                layerObject->getGraphicsObject()->setup(renderingContext);
            }
        }
    }
}

void IconLayer::updateIconPosition(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                   const std::shared_ptr<IconInfoInterface> &iconInfo,
                                   const std::shared_ptr<Textured2dLayerObject> &iconObject) {
    Coord iconPosRender = conversionHelper->convertToRenderSystem(iconInfo->getCoordinate());
    if (iconInfo->getType() != IconType::FIXED) {
        iconPosRender.x = 0;
        iconPosRender.y = 0;
    }
    const Vec2F &anchor = iconInfo->getIconAnchor();
    float ratioLeftRight = std::clamp(anchor.x, 0.0f, 1.0f);
    float ratioTopBottom = std::clamp(anchor.y, 0.0f, 1.0f);
    float leftW = iconInfo->getIconSize().x * ratioLeftRight;
    float topH = iconInfo->getIconSize().y * ratioTopBottom;
    float rightW = iconInfo->getIconSize().x * (1.0f - ratioLeftRight);
    float bottomH = iconInfo->getIconSize().y * (1.0f - ratioTopBottom);
    iconObject->setRectCoord(
            RectCoord(Coord(iconPosRender.systemIdentifier, iconPosRender.x - leftW, iconPosRender.y - topH, iconPosRender.y),
                      Coord(iconPosRender.systemIdentifier, iconPosRender.x + rightW, iconPosRender.y + bottomH, iconPosRender.y)));
}

std::vector<std::shared_ptr<::RenderPassInterface>> IconLayer::buildRenderPasses() {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto conversionHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    if (!camera || !conversionHelper) {
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
                    auto renderCoord = conversionHelper->convert(CoordinateSystemIdentifiers::RENDERSYSTEM(), iconTuple.first->getCoordinate());
                    std::vector<float> modelMatrix =
                        camera->getInvariantModelMatrix(renderCoord, scaleInvariant, rotationInvariant);
                    Matrix::translateM(modelMatrix, 0, renderCoord.x, renderCoord.y, 0.0);
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
                std::make_shared<RenderPass>(RenderPassConfig(passEntry.first, false), passEntry.second, mask);
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
        std::lock_guard<std::recursive_mutex> lock(iconsMutex);
        clearSync(icons);
    }

    if (mask) {
        if (mask->asGraphicsObject()->isReady())
            mask->asGraphicsObject()->clear();
    }
}

void IconLayer::resume() {
    {
        std::lock_guard<std::recursive_mutex> lock(iconsMutex);
        setupIconObjects(icons);
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

std::vector<std::shared_ptr<IconInfoInterface>> IconLayer::getIconsAtPosition(const ::Vec2F &posScreen) {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto conversionHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    if (!camera || !conversionHelper) {
        return {};
    }

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

        return iconsHit;
    }
}

bool IconLayer::onClickConfirmed(const Vec2F &posScreen) {
    if (callbackHandler) {
        auto iconsHit = getIconsAtPosition(posScreen);

        if (!iconsHit.empty()) {
            return callbackHandler->onClickConfirmed(iconsHit);
        }
    }
    return false;
}


bool IconLayer::onLongPress(const Vec2F &posScreen) {
    if (callbackHandler) {
        auto iconsHit = getIconsAtPosition(posScreen);

        if (!iconsHit.empty()) {
            return callbackHandler->onLongPress(iconsHit);
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
    this->alpha = alpha;
}

float IconLayer::getAlpha() {
    return alpha;
}