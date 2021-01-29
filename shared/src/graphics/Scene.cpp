#include "Scene.h"
#include "ColorShaderInterface.h"
#include "RenderPass.h"
#include "Rectangle2dInterface.h"
#include "Renderer.h"
#include "ExampleCamera.h"

std::shared_ptr <SceneInterface> SceneInterface::create(const std::shared_ptr<::GraphicsObjectFactoryInterface> &graphicsFactory,
                                      const std::shared_ptr<::ShaderFactoryInterface> &shaderFactory) {
    return std::make_shared<Scene>(graphicsFactory, shaderFactory);
}

std::shared_ptr <SceneInterface> SceneInterface::createWithOpenGl() {
    return NULL;
}


Scene::Scene(const std::shared_ptr<::GraphicsObjectFactoryInterface> & graphicsFactory,
             const std::shared_ptr<::ShaderFactoryInterface> & shaderFactory) :
graphicsFactory(graphicsFactory),
shaderFactory(shaderFactory),
renderer(std::make_shared<Renderer>()) {

    // Begin testing code
    auto shader = shaderFactory->createColorShader();
    auto rect = graphicsFactory->createRectangle(shader->asShaderProgramInterface());
    rect->setFrame(RectF(0, 0, 0.8, 0.8), RectF(0, 0, 0.2, 0.2));
    shader->setColor(1, 0, 0, 0.5);
    std::vector<std::shared_ptr<GraphicsObjectInterface>> objects{rect->asGraphicsObject()};
    renderPass = std::make_shared<RenderPass>(RenderPassConfig(0), objects);
    setCamera(std::make_shared<ExampleCamera>());
    // End testing code
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
    if (auto renderPass = this->renderPass) {
        // Begin testing code
        renderer->addToRenderQueue(renderPass);
        // End testing code
        renderer->drawFrame(renderingContext, camera);
    }
}

void Scene::clear() {}

void Scene::invalidate() {}

void Scene::setCallbackHandler(const std::shared_ptr<SceneCallbackInterface> & callbackInterface) {}
