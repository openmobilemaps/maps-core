#include "TestingLayer.h"
#include "RenderPassConfig.h"

TestingLayer::TestingLayer(std::shared_ptr<GraphicsObjectFactoryInterface> graphicsObjectFactory, std::shared_ptr<ShaderFactoryInterface> shaderFactory) {
    /*auto shader = shaderFactory->createAlphaShader();
    object = std::make_shared<Textured2dLayerObject>(graphicsObjectFactory->createRectangle(shader->asShaderProgramInterface()), shader);
    auto renderConfig = object->getRenderConfig();*/
    //renderPass = std::make_shared<RenderPass>(RenderPassConfig(0), {renderConfig.front()->getGraphicsObject()});
}

std::vector<std::shared_ptr<::RenderPassInterface>> TestingLayer::buildRenderPasses() {
    return {renderPass};
}

std::string TestingLayer::getIdentifier() {
    return "TestingLayer";
}

void TestingLayer::pause() {

}

void TestingLayer::resume() {

}

void TestingLayer::hide() {

}

void TestingLayer::show() {

}
