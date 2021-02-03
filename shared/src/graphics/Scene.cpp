#include "Scene.h"
#include "ColorShaderInterface.h"
#include "ColorLineShaderInterface.h"
#include "RenderPass.h"
#include "Rectangle2dInterface.h"
#include "Polygon2dInterface.h"
#include "Line2dInterface.h"
#include "Renderer.h"

Scene::Scene(const std::shared_ptr<::GraphicsObjectFactoryInterface> &graphicsFactory,
             const std::shared_ptr<::ShaderFactoryInterface> &shaderFactory,
             const std::shared_ptr<::RenderingContextInterface> & renderingContext) :
        graphicsFactory(graphicsFactory),
        shaderFactory(shaderFactory),
        renderingContext(renderingContext),
        renderer(std::make_shared<Renderer>()) {
}

void Scene::setCallbackHandler(const std::shared_ptr<SceneCallbackInterface> &callbackInterface) {
    callbackHandler = callbackInterface;
}

void Scene::setCamera(const std::shared_ptr<CameraInterface> &camera) {
    this->camera = camera;
}

std::shared_ptr<::GraphicsObjectFactoryInterface> Scene::getGraphicsFactory() {
    return graphicsFactory;
}

std::shared_ptr<::ShaderFactoryInterface> Scene::getShaderFactory() {
    return shaderFactory;
}

std::shared_ptr<CameraInterface> Scene::getCamera() {
    return camera;
}

std::shared_ptr<RendererInterface> Scene::getRenderer() {
    return renderer;
}

std::shared_ptr<RenderingContextInterface> Scene::getRenderingContext() {
    return renderingContext;
}

void Scene::drawFrame() {
    if (camera) {
        renderer->drawFrame(renderingContext, camera);
    }
}

void Scene::clear() {}

void Scene::invalidate() {
    if (auto handler = callbackHandler) {
        handler->invalidate();
    }
}
