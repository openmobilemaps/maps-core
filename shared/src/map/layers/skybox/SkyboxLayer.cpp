/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "SkyboxLayer.h"
#include "CameraInterface.h"
#include "LambdaTask.h"
#include "MapCamera2dInterface.h"
#include "RenderObject.h"
#include "RenderPass.h"
#include "ColorShaderInterface.h"

SkyboxLayer::SkyboxLayer()
   : isHidden(false) {

   }



std::shared_ptr<::LayerInterface> SkyboxLayer::asLayerInterface() { return shared_from_this(); }


std::vector<::RenderTask> SkyboxLayer::getRenderTasks() {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!camera) {
        return {};
    }

    if (isHidden) {
        return {};
    } else {
        return renderTasks;
    }
}


void SkyboxLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface, int32_t layerIndex) {
    this->mapInterface = mapInterface;

    std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> currentRenderPassObjectMap;


    std::vector<std::shared_ptr<RenderPassInterface>> renderPasses;

    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;

    auto shader = mapInterface->getShaderFactory()->createSkyboxShader();
    auto quad = mapInterface->getGraphicsObjectFactory()->createQuad(shader);
    quad->setFrame(Quad2dD(Vec2D(-1, -1), Vec2D(1, -1), Vec2D(1, 1), Vec2D(-1, 1)), RectD(0, 0, 1, 1));
    quad->asGraphicsObject()->setup(renderingContext);
    auto renderObject = std::make_shared<RenderObject>(quad->asGraphicsObject(), true);
    auto renderPass = std::make_shared<RenderPass>(RenderPassConfig(0), std::vector<std::shared_ptr<::RenderObjectInterface>> { renderObject });
    renderPasses.push_back(renderPass);

    //        for (const auto &passEntry : currentRenderPassObjectMap) {
    //            std::shared_ptr<RenderPass> renderPass =
    //                std::make_shared<RenderPass>(RenderPassConfig(passEntry.first), passEntry.second, mask);
    //            renderPasses.push_back(renderPass);
    //        }
    renderTasks = std::vector<::RenderTask>{RenderTask(nullptr, renderPasses)};;

//    {
//        std::scoped_lock<std::recursive_mutex> lock(addingQueueMutex);
//    }
//    if (isLayerClickable) {
//        mapInterface->getTouchHandler()->insertListener(shared_from_this(), layerIndex);
//    }
}

void SkyboxLayer::onRemoved() {
}

void SkyboxLayer::pause() {
}

void SkyboxLayer::resume() {
}

void SkyboxLayer::hide() {
    isHidden = true;
    auto mapInterface = this->mapInterface;
    if (mapInterface)
        mapInterface->invalidate();
}

void SkyboxLayer::show() {
    isHidden = false;
    auto mapInterface = this->mapInterface;
    if (mapInterface)
        mapInterface->invalidate();
}

void SkyboxLayer::update() {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    if (mapInterface && mask) {
        if (!mask->asGraphicsObject()->isReady())
            mask->asGraphicsObject()->setup(mapInterface->getRenderingContext());
    }
}

void SkyboxLayer::setMaskingObject(const std::shared_ptr<::MaskingObjectInterface> &maskingObject) {
    this->mask = maskingObject;
    auto mapInterface = this->mapInterface;
    if (mapInterface) {
        mapInterface->invalidate();
    }
}


