#ifdef __ANDROID__

#include "GraphicsObjectFactoryOpenGl.h"
#include "ShaderFactoryOpenGl.h"
#include "OpenGlContext.h"

#endif

#include "Scene.h"

std::shared_ptr<SceneInterface> SceneInterface::create(const std::shared_ptr<::GraphicsObjectFactoryInterface> &graphicsFactory,
                                                       const std::shared_ptr<::ShaderFactoryInterface> &shaderFactory) {
    return std::make_shared<Scene>(graphicsFactory, shaderFactory);
}

std::shared_ptr<SceneInterface> SceneInterface::createWithOpenGl() {
#ifdef __ANDROID__
    auto scene = std::static_pointer_cast<SceneInterface>(std::make_shared<Scene>(
            std::make_shared<GraphicsObjectFactoryOpenGl>(),
            std::make_shared<ShaderFactoryOpenGl>()));
    scene->setRenderingContext(std::make_shared<OpenGlContext>());
    return scene;
#else
    return nullptr;
#endif
}
