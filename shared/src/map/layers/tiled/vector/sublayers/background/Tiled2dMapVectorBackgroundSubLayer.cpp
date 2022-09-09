/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorBackgroundSubLayer.h"
#include "MapInterface.h"
#include "ColorShaderInterface.h"
#include "MapConfig.h"
#include "RenderPass.h"
#include "RenderObject.h"
#include "SchedulerInterface.h"
#include "LambdaTask.h"

void Tiled2dMapVectorBackgroundSubLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface) {
    Tiled2dMapVectorSubLayer::onAdded(mapInterface);
    const auto shader = mapInterface->getShaderFactory()->createColorShader();
    object = mapInterface->getGraphicsObjectFactory()->createQuad(shader->asShaderProgramInterface());

    object->setFrame(Quad2dD(Vec2D(-1, 1),
                             Vec2D(1, 1),
                             Vec2D(1, -1),
                             Vec2D(-1, -1)),
                     RectD(0, 0, 1, 1));

    auto color = description->style.getColor(EvaluationContext(std::nullopt, FeatureContext()));
    shader->setColor(color.r, color.g, color.b, color.a);
    auto renderObject = std::make_shared<RenderObject>(object->asGraphicsObject(), true);
    auto renderPass = std::make_shared<RenderPass>(RenderPassConfig(0), std::vector<std::shared_ptr<::RenderObjectInterface>> { renderObject } );
    renderPasses = {
        renderPass
    };

    std::weak_ptr<Tiled2dMapVectorBackgroundSubLayer> weakSelfPtr = weak_from_this();
    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
           TaskConfig("Tiled2dMapVectorBackgroundSubLayer setup", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS), [weakSelfPtr] {
                auto selfPtr = weakSelfPtr.lock();
                if (!selfPtr) {
                    return;
                }

                if (!selfPtr->object->asGraphicsObject()->isReady()) {
                    selfPtr->object->asGraphicsObject()->setup(selfPtr->mapInterface->getRenderingContext());
                }
            }));
}

std::vector<std::shared_ptr<RenderPassInterface>> Tiled2dMapVectorBackgroundSubLayer::buildRenderPasses() {
    return renderPasses;
}

std::vector<std::shared_ptr<RenderPassInterface>> Tiled2dMapVectorBackgroundSubLayer::buildRenderPasses(const std::unordered_set<Tiled2dMapTileInfo> &tiles) {
    return renderPasses;
}

void Tiled2dMapVectorBackgroundSubLayer::onRemoved() {
    Tiled2dMapVectorSubLayer::onRemoved();
    pause();
}

void Tiled2dMapVectorBackgroundSubLayer::pause() {
    Tiled2dMapVectorSubLayer::pause();
    if (object) {
        object->asGraphicsObject()->clear();
    }
}

void Tiled2dMapVectorBackgroundSubLayer::resume() {
    Tiled2dMapVectorSubLayer::resume();

    if (object && !object->asGraphicsObject()->isReady()) {
        object->asGraphicsObject()->setup(mapInterface->getRenderingContext());
    }
}

void Tiled2dMapVectorBackgroundSubLayer::hide() {
    Tiled2dMapVectorSubLayer::hide();
}

void Tiled2dMapVectorBackgroundSubLayer::show() {
    Tiled2dMapVectorSubLayer::show();
}

void Tiled2dMapVectorBackgroundSubLayer::setScissorRect(const std::optional<::RectI> &scissorRect) {
    for (auto const &renderPass: renderPasses) {
        std::dynamic_pointer_cast<RenderPass>(renderPass)->setScissoringRect(scissorRect);
    }
}
