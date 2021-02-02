#include "Scene.h"
#include "ColorShaderInterface.h"
#include "ColorLineShaderInterface.h"
#include "RenderPass.h"
#include "Rectangle2dInterface.h"
#include "Polygon2dInterface.h"
#include "Line2dInterface.h"
#include "Renderer.h"
#include "ExampleCamera.h"

Scene::Scene(const std::shared_ptr<::GraphicsObjectFactoryInterface> &graphicsFactory,
             const std::shared_ptr<::ShaderFactoryInterface> &shaderFactory) :
        graphicsFactory(graphicsFactory),
        shaderFactory(shaderFactory),
        renderer(std::make_shared<Renderer>()) {

    // Begin testing code
    auto colorShader = shaderFactory->createColorShader();
    colorShader->setColor(1.0, 0.0, 0.0, 0.5);
    auto poly = graphicsFactory->createPolygon(colorShader->asShaderProgramInterface());
    poly->setPolygonPositions({Vec2F(0.1, -0.3), Vec2F(0.4, -0.4), Vec2F(0.5, -0.05), Vec2F(0.15, 0)}, {}, true);
    auto rect = graphicsFactory->createRectangle(colorShader->asShaderProgramInterface());
    rect->setFrame(RectF(-0.2, -0.2, 0.4, 0.4), RectF(0, 0, 1, 1));
    auto lineShader = shaderFactory->createColorLineShader();
    lineShader->setColor(1.0, 0.0, 0.0, 0.5);
    lineShader->setMiter(0.005);
    auto line = graphicsFactory->createLine(lineShader->asLineShaderProgramInterface());
    line->setLinePositions({Vec2F(-0.4, -0.1), Vec2F(0.4, 0.1)});
    std::vector<std::shared_ptr<GraphicsObjectInterface>> objects{rect->asGraphicsObject(), line->asGraphicsObject(),
                                                                  poly->asGraphicsObject()};
    renderPass = std::make_shared<RenderPass>(RenderPassConfig(0), objects);
    setCamera(std::make_shared<ExampleCamera>());
    // End testing code
}

void Scene::setRenderingContext(const std::shared_ptr<RenderingContextInterface> &renderingContext) {
    this->renderingContext = renderingContext;
}

void Scene::setCallbackHandler(const std::shared_ptr<SceneCallbackInterface> &callbackInterface) {
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
