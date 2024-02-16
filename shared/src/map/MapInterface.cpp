/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "MapScene.h"
#ifdef __ANDROID__
#include "ThreadPoolSchedulerImpl.h"
#include "AndroidSchedulerCallback.h"
#endif

std::shared_ptr<MapInterface> MapInterface::create(const std::shared_ptr<::GraphicsObjectFactoryInterface> &graphicsFactory,
                                                   const std::shared_ptr<::ShaderFactoryInterface> &shaderFactory,
                                                   const std::shared_ptr<::RenderingContextInterface> &renderingContext,
                                                   const MapConfig &mapConfig,
                                                   const std::shared_ptr<::SchedulerInterface> &scheduler, float pixelDensity,
                                                   bool is3D) {
    auto scene = SceneInterface::create(graphicsFactory, shaderFactory, renderingContext);
    return std::make_shared<MapScene>(scene, mapConfig, scheduler, pixelDensity, is3D);
}

std::shared_ptr<MapInterface> MapInterface::createWithOpenGl(const MapConfig &mapConfig,
                                                             float pixelDensity,
                                                             bool is3D) {
#ifdef __ANDROID__
    auto scheduler = std::make_shared<ThreadPoolSchedulerImpl>(std::make_shared<AndroidSchedulerCallback>());
    return std::make_shared<MapScene>(SceneInterface::createWithOpenGl(), mapConfig, scheduler, pixelDensity, is3D);
#else
    return nullptr;
#endif
}