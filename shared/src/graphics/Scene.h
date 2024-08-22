/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "RendererInterface.h"
#include "SceneCallbackInterface.h"
#include "SceneInterface.h"
#include <optional>

class Scene : public SceneInterface {
  public:
    Scene(const std::shared_ptr<::GraphicsObjectFactoryInterface> &graphicsFactory,
          const std::shared_ptr<::ShaderFactoryInterface> &shaderFactory,
          const std::shared_ptr<::RenderingContextInterface> &renderingContext);

    virtual void setCallbackHandler(const std::shared_ptr<SceneCallbackInterface> &callbackInterface) override;

    virtual void setCamera(const std::shared_ptr<CameraInterface> &camera) override;

    virtual std::shared_ptr<CameraInterface> getCamera() override;

    virtual std::shared_ptr<RendererInterface> getRenderer() override;

    virtual std::shared_ptr<RenderingContextInterface> getRenderingContext() override;

    virtual std::shared_ptr<::GraphicsObjectFactoryInterface> getGraphicsFactory() override;

    virtual std::shared_ptr<::ShaderFactoryInterface> getShaderFactory() override;

    virtual void drawFrame() override;

    virtual void prepare() override;

    virtual void compute() override;

    virtual void clear() override;

    virtual void invalidate() override;

  private:
    std::shared_ptr<RenderingContextInterface> renderingContext;
    std::shared_ptr<CameraInterface> camera;

    std::shared_ptr<SceneCallbackInterface> callbackHandler;
    const std::shared_ptr<::GraphicsObjectFactoryInterface> graphicsFactory;
    const std::shared_ptr<::ShaderFactoryInterface> shaderFactory;

    std::shared_ptr<RendererInterface> renderer;
};
