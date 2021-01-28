#include "Scene.h"

std::shared_ptr <SceneInterface> SceneInterface::create(const std::shared_ptr<::GraphicsObjectFactoryInterface> &graphicsFactory,
                                      const std::shared_ptr<::ShaderFactoryInterface> &shaderFactory) {
    return std::make_shared<Scene>(graphicsFactory, shaderFactory);
}

std::shared_ptr <SceneInterface> SceneInterface::createWithOpenGl() {
    return NULL;
}


Scene::Scene(const std::shared_ptr<::GraphicsObjectFactoryInterface> & graphicsFactory,
             const std::shared_ptr<::ShaderFactoryInterface> & shaderFactory) {

}

void Scene::setRenderingContext(const std::shared_ptr<RenderingContextInterface> & renderingContext) {

}

std::shared_ptr<RenderingContextInterface> Scene::getRenderingContext() {

}

void Scene::setCamera(const std::shared_ptr<CameraInterface> & camera) {

}

std::shared_ptr<CameraInterface> Scene::getCamera() {

}

std::shared_ptr<RendererInterface> Scene::getRenderer() {

}

void Scene::drawFrame() {

}

void Scene::clear() {

}
