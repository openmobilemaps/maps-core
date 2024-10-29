/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#ifdef __linux__

#include "GraphicsObjectFactoryOpenGl.h"
#include "OpenGlContext.h"
#include "ShaderFactoryOpenGl.h"

#endif

#include "Scene.h"

std::shared_ptr<SceneInterface> SceneInterface::create(const std::shared_ptr<::GraphicsObjectFactoryInterface> &graphicsFactory,
                                                       const std::shared_ptr<::ShaderFactoryInterface> &shaderFactory,
                                                       const std::shared_ptr<::RenderingContextInterface> &renderingContext) {
    return std::make_shared<Scene>(graphicsFactory, shaderFactory, renderingContext);
}

std::shared_ptr<SceneInterface> SceneInterface::createWithOpenGl() {
#ifdef __linux__
    auto scene = std::static_pointer_cast<SceneInterface>(std::make_shared<Scene>(std::make_shared<GraphicsObjectFactoryOpenGl>(),
                                                                                  std::make_shared<ShaderFactoryOpenGl>(),
                                                                                  std::make_shared<OpenGlContext>()));
    return scene;
#else
    return nullptr;
#endif
}
