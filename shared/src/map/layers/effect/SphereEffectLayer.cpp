/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "SphereEffectLayer.h"
#include "MapInterface.h"
#include "ColorShaderInterface.h"
#include "MapConfig.h"
#include "RenderPass.h"
#include "RenderObject.h"
#include "SchedulerInterface.h"
#include "LambdaTask.h"
#include "PolygonPatternGroup2dInterface.h"
#include "MapCameraInterface.h"
#include "PolygonGroupShaderInterface.h"
#include "PolygonHelper.h"
#include "SphereEffectShaderInterface.h"

std::shared_ptr<SphereEffectLayerInterface> SphereEffectLayerInterface::create() {
    return std::make_shared<SphereEffectLayer>();
}

std::shared_ptr<::LayerInterface> SphereEffectLayer::asLayerInterface() {
    return shared_from_this();
}

void SphereEffectLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface, int32_t layerIndex) {

    this->camera = mapInterface->getCamera()->asMapCamera3d();
    this->castedCamera = std::dynamic_pointer_cast<MapCamera3d>(this->camera);
    this->shader = mapInterface->getShaderFactory()->createSphereEffectShader();
    this->mapInterface = mapInterface;

    this->quad = mapInterface->getGraphicsObjectFactory()->createQuad(shader->asShaderProgramInterface());

    this->quad->setFrame(Quad3dD(Vec3D(-1, 1, 0), Vec3D(1, 1, 0), Vec3D(1, -1, 0), Vec3D(-1, -1, 0)), RectD(-1, -1, 2, 2), Vec3D(0, 0, 0), false);

    std::vector<std::shared_ptr<::RenderObjectInterface>> renderObjects {  };

    renderObjects.push_back(std::make_shared<RenderObject>(this->quad->asGraphicsObject(), true));

    auto renderPass = std::make_shared<RenderPass>(RenderPassConfig(0, false), renderObjects );
    renderPasses = {
        renderPass
    };

    std::weak_ptr<SphereEffectLayer> weakSelfPtr = weak_from_this();
    auto scheduler = mapInterface->getScheduler();
    if (!scheduler) {
        return;
    }
    scheduler->addTask(std::make_shared<LambdaTask>(
                                                    TaskConfig("SphereEffectLayer setup", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS), [weakSelfPtr] {
                                                        auto selfPtr = weakSelfPtr.lock();
                                                        if (!selfPtr) {
                                                            return;
                                                        }
                                                        auto mapInterface = selfPtr->mapInterface;

                                                        if (mapInterface && !selfPtr->quad->asGraphicsObject()->isReady()) {
                                                            selfPtr->quad->asGraphicsObject()->setup(mapInterface->getRenderingContext());
                                                        }
                                                    }));
}

void SphereEffectLayer::update() {
    if (!shader || !castedCamera) {
        return;
    }

    static std::vector<double> coefficients(16, 0.0);
    static std::vector<float> coefficientsFloat(16, 0.0);

    castedCamera->computeEllipseCoefficients(coefficients);

    for(size_t i = 0; i<16; ++i) {
        coefficientsFloat[i] = coefficients[i];
    }

    shader->setEllipse(SharedBytes((int64_t)coefficientsFloat.data(), 16, sizeof(float)));
}

std::vector<std::shared_ptr<RenderPassInterface>> SphereEffectLayer::buildRenderPasses() {
    return renderPasses;
}

void SphereEffectLayer::setAlpha(float alpha) {
    this->alpha = alpha;
}

void SphereEffectLayer::onRemoved() {
}

void SphereEffectLayer::pause() {
}

void SphereEffectLayer::resume() {
}

void SphereEffectLayer::hide() {
}

void SphereEffectLayer::show() {
}
