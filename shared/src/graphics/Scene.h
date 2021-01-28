#pragma once

#include "SceneInterface.h"

class Scene: public SceneInterface {
public:
    Scene(const std::shared_ptr<::GraphicsObjectFactoryInterface> & graphicsFactory, const std::shared_ptr<::ShaderFactoryInterface> & shaderFactory);

    void setRenderingContext(const std::shared_ptr<RenderingContextInterface> & renderingContext);

    std::shared_ptr<RenderingContextInterface> getRenderingContext();

    void setCamera(const std::shared_ptr<CameraInterface> & camera);

    std::shared_ptr<CameraInterface> getCamera();

    std::shared_ptr<RendererInterface> getRenderer();

    void drawFrame();

    void clear();
};
