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
#include "Polygon3dInterface.h"

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
    auto polygon = mapInterface->getGraphicsObjectFactory()->createPolygon3d(shader);
    std::vector<float> vertices = {
        -10.0f, -10.0f, -10.0f, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f, // V0
        10.0f, -10.0f, -10.0f, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // V1
        10.0f, 10.0f, -10.0f, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f,   // V2
        -10.0f, 10.0f, -10.0f, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // V3
        -10.0f, -10.0f, 10.0f, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // V4
        10.0f, -10.0f, 10.0f, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f,   // V5
        10.0f, 10.0f, 10.0f, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f,    // V6
        -10.0f, 10.0f, 10.0f, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f    // V7
    };

    std::vector<uint16_t> indices = {
        0, 1, 3, // Triangle 1
        1, 2, 3, // Triangle 2
        1, 5, 2, // Triangle 3
        5, 6, 2, // Triangle 4
        5, 4, 6, // Triangle 5
        4, 7, 6, // Triangle 6
        4, 0, 7, // Triangle 7
        0, 3, 7, // Triangle 8
        3, 2, 7, // Triangle 9
        2, 6, 7, // Triangle 10
        0, 4, 1, // Triangle 11
        4, 5, 1  // Triangle 12
    };
    auto attr = SharedBytes((int64_t)vertices.data(), (int32_t)vertices.size(), (int32_t)sizeof(float));
    auto ind = SharedBytes((int64_t)indices.data(), (int32_t)indices.size(), (int32_t)sizeof(uint16_t));
    polygon->setVertices(attr, ind);
    polygon->asGraphicsObject()->setup(renderingContext);
    auto renderObject = std::make_shared<RenderObject>(polygon->asGraphicsObject());
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


