#include "Scene.h"

std::shared_ptr <SceneInterface> SceneInterface::create(const std::shared_ptr<::GraphicsObjectFactoryInterface> &graphicsFactory,
                                      const std::shared_ptr<::ShaderFactoryInterface> &shaderFactory) {
    return std::make_shared<Scene>(graphicsFactory, shaderFactory);
}

std::shared_ptr <SceneInterface> SceneInterface::createWithOpenGl() {
    return NULL;
}


Scene::Scene(const std::shared_ptr<::GraphicsObjectFactoryInterface> & graphicsFactory,
             const std::shared_ptr<::ShaderFactoryInterface> & shaderFactory) : graphicsFactory(graphicsFactory), shaderFactory(shaderFactory) {

}

void Scene::setRenderingContext(const std::shared_ptr<RenderingContextInterface> & renderingContext) {
    this->renderingContext = renderingContext;
}

std::shared_ptr<RenderingContextInterface> Scene::getRenderingContext() {
    return renderingContext;
}

void Scene::setCamera(const std::shared_ptr<CameraInterface> & camera) {
    this->camera = camera;
}

std::shared_ptr<CameraInterface> Scene::getCamera() {
    return camera;
}

std::shared_ptr<RendererInterface> Scene::getRenderer() {
    return renderer;
}

void Scene::drawFrame() {
    renderer->drawFrame(renderingContext, camera);
}

void Scene::clear() {

}
