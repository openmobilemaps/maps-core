#ifdef __ANDROID__

#include "GraphicsObjectFactoryOpenGl.h"
#include "ShaderFactoryOpenGl.h"
#include "OpenGlContext.h"

#endif

#include "Scene.h"
#include "ColorShaderInterface.h"
#include "RenderPass.h"
#include "Rectangle2dInterface.h"
#include "Polygon2dInterface.h"
#include "Renderer.h"
#include "ExampleCamera.h"

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


Scene::Scene(const std::shared_ptr<::GraphicsObjectFactoryInterface> &graphicsFactory,
             const std::shared_ptr<::ShaderFactoryInterface> &shaderFactory) :
        graphicsFactory(graphicsFactory),
        shaderFactory(shaderFactory),
        renderer(std::make_shared<Renderer>()) {

    // Begin testing code
    auto shader = shaderFactory->createColorShader();
    auto rect = graphicsFactory->createPolygon(shader->asShaderProgramInterface());
    //rect->setFrame(RectF(0, 0, 0.8, 0.8), RectF(0, 0, 0.2, 0.2));
    rect->setPolygonPositions({Vec2F(-0.2f, -0.2f), Vec2F(0.2f, -0.2f), Vec2F(0.2f, 0.2f), Vec2F(-0.2f, 0.2f)}, {}, true);
    shader->setColor(1, 0, 0, 0.5);
    std::vector<std::shared_ptr<GraphicsObjectInterface>> objects{rect->asGraphicsObject()};
    renderPass = std::make_shared<RenderPass>(RenderPassConfig(0), objects);
    setCamera(std::make_shared<ExampleCamera>());
    // End testing code
}

void Scene::setRenderingContext(const std::shared_ptr<RenderingContextInterface> &renderingContext) {
    this->renderingContext = renderingContext;
}

void Scene::setCallbackHandler(const std::shared_ptr<SceneCallbackInterface> & callbackInterface) {
    callbackHandler = callbackInterface;
}

std::shared_ptr<RenderingContextInterface> Scene::getRenderingContext() {
    return renderingContext;
}

void Scene::setCamera(const std::shared_ptr<CameraInterface> &camera) {
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

void Scene::invalidate() {
    if (auto handler = callbackHandler) {
        (*handler)->invalidate();
    }
}
