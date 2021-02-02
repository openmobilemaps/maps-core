#pragma once

#include "SceneInterface.h"
#include "RendererInterface.h"
#include "SceneCallbackInterface.h"
#include <optional>

class Scene: public SceneInterface {
public:
    Scene(const std::shared_ptr<::GraphicsObjectFactoryInterface> & graphicsFactory,
          const std::shared_ptr<::ShaderFactoryInterface> & shaderFactory,
          const std::shared_ptr<::RenderingContextInterface> & renderingContext);

    virtual void setCallbackHandler(const std::shared_ptr<SceneCallbackInterface> & callbackInterface);

    virtual void setCamera(const std::shared_ptr<CameraInterface> & camera);

    virtual std::shared_ptr<CameraInterface> getCamera();

    virtual std::shared_ptr<RendererInterface> getRenderer();

    virtual std::shared_ptr<::GraphicsObjectFactoryInterface> getGraphicsFactory();

    virtual std::shared_ptr<::ShaderFactoryInterface> getShaderFactory();

    virtual void drawFrame();

    virtual void clear();

    virtual void invalidate();

private:
    std::shared_ptr<RenderingContextInterface> renderingContext;
    std::shared_ptr<CameraInterface> camera;

    std::shared_ptr<SceneCallbackInterface> callbackHandler;
    const std::shared_ptr<::GraphicsObjectFactoryInterface> graphicsFactory;
    const std::shared_ptr<::ShaderFactoryInterface> & shaderFactory;

    std::shared_ptr<RendererInterface> renderer;

    std::shared_ptr<RenderPassInterface> renderPass;
};
