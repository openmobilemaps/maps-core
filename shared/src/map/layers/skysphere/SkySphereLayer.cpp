/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "SkySphereLayer.h"
#include "MapInterface.h"
#include "GraphicsObjectInterface.h"
#include "RenderPass.h"
#include "RenderObject.h"

// SimpleLayerInterface

void SkySphereLayer::update() {
    auto mapInterface = this->mapInterface;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!camera) {
        return;
    }

    auto inverseVpMatrix = camera->getLastInverseVpMatrix();
    auto cameraPosition = camera->getLastCameraPosition();
    if (inverseVpMatrix && cameraPosition) {
        shader->setCameraProperties(*inverseVpMatrix, *cameraPosition);
    }
}

std::vector<std::shared_ptr<::RenderPassInterface>> SkySphereLayer::buildRenderPasses() {
    return (isHidden || !skySphereTexture) ? std::vector<std::shared_ptr<::RenderPassInterface>>() : renderPasses;
}

void SkySphereLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface, int32_t layerIndex) {
    this->mapInterface = mapInterface;

    auto scheduler = mapInterface->getScheduler();
    auto selfMailbox = std::make_shared<Mailbox>(scheduler);
    this->mailbox = selfMailbox;

    shader = mapInterface->getShaderFactory()->createSkySphereShader();
    quad = mapInterface->getGraphicsObjectFactory()->createQuad(shader->asShaderProgramInterface());
    quad->setFrame(Quad3dD(Vec3D(-1, 1, 0),
                           Vec3D(1, 1, 0),
                           Vec3D(1, -1, 0),
                           Vec3D(-1, -1, 0)),
                   RectD(-1, -1, 2, 2),
                   Vec3D(0, 0, 0),
                   false);
}

void SkySphereLayer::onRemoved() {
    this->mapInterface = nullptr;
    this->mailbox = nullptr;

    if (quad->asGraphicsObject()->isReady()) {
        quad->asGraphicsObject()->clear();
    }
    this->quad = nullptr;
    this->shader = nullptr;
}

void SkySphereLayer::pause() {
    quad->asGraphicsObject()->clear();
}

void SkySphereLayer::resume() {
    setupSkySphere();
}

void SkySphereLayer::hide() {
    isHidden = false;
}

void SkySphereLayer::show() {
    isHidden = true;
}

// SkySphereLayerInterface

std::shared_ptr<::LayerInterface> SkySphereLayer::asLayerInterface() {
    return shared_from_this();
}

void SkySphereLayer::setTexture(const std::shared_ptr<::TextureHolderInterface> &texture) {
    this->skySphereTexture = texture;
    auto selfMailbox = this->mailbox;
    if (selfMailbox) {
        WeakActor<SkySphereLayer>(selfMailbox, weak_from_this())
                .message(MailboxExecutionEnvironment::graphics, MFN(&SkySphereLayer::setupSkySphere));
    }
}

void SkySphereLayer::setupSkySphere() {
    auto mapInterface = this->mapInterface;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) {
        return;
    }

    auto skySphereTexture = this->skySphereTexture;
    auto quad = this->quad;
    if (skySphereTexture && renderingContext && quad) {
        quad->asGraphicsObject()->setup(renderingContext);
        quad->loadTexture(renderingContext, skySphereTexture);

        std::vector<std::shared_ptr<RenderObjectInterface>> renderObjects = {
                std::make_shared<RenderObject>(quad->asGraphicsObject(), true)
        };
        renderPasses = {std::make_shared<RenderPass>(RenderPassConfig(0, false), renderObjects)};
    }
}
