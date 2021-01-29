#pragma once

#include "SceneInterface.h"
#include "RendererInterface.h"

class Scene: public SceneInterface {
public:
    Scene(const std::shared_ptr<::GraphicsObjectFactoryInterface> & graphicsFactory, const std::shared_ptr<::ShaderFactoryInterface> & shaderFactory);

    virtual void setCallbackHandler(const std::shared_ptr<SceneCallbackInterface> & callbackInterface);

    virtual void setRenderingContext(const std::shared_ptr<RenderingContextInterface> & renderingContext);

    virtual std::shared_ptr<RenderingContextInterface> getRenderingContext();

    virtual void setCamera(const std::shared_ptr<CameraInterface> & camera);

    virtual std::shared_ptr<CameraInterface> getCamera();

    virtual std::shared_ptr<RendererInterface> getRenderer();

    virtual void drawFrame();

    virtual void clear();

    virtual void invalidate();

private:
    std::shared_ptr<RenderingContextInterface> renderingContext;
    std::shared_ptr<CameraInterface> camera;

    const std::shared_ptr<::GraphicsObjectFactoryInterface> graphicsFactory;
    const std::shared_ptr<::ShaderFactoryInterface> & shaderFactory;

    std::shared_ptr<RendererInterface> renderer;

    std::shared_ptr<RenderPassInterface> renderPass;
};
