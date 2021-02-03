#include "TestingLayer.h"
#include "RenderPassConfig.h"

TestingLayer::TestingLayer(std::shared_ptr<GraphicsObjectFactoryInterface> graphicsObjectFactory, std::shared_ptr<ShaderFactoryInterface> shaderFactory) {
    auto shader = shaderFactory->createColorShader();
    object = std::make_shared<Polygon2dLayerObject>(graphicsObjectFactory->createPolygon(shader->asShaderProgramInterface()), shader);
    auto renderConfig = object->getRenderConfig();

    object->setPositions({
        Vec2D(2483750.172633792, 1110749.9292120978),
        Vec2D(2484250.1398714944, 1121749.9210555588),
        Vec2D(2493750.1353852837, 1124749.932216558),
        Vec2D(2497750.1114729308, 1134249.9298414628),
        Vec2D(2490250.0966190984, 1140749.9155252017),
        Vec2D(2496750.055996045, 1161249.912233307),
        Vec2D(2519750.007658271, 1181249.9364165873),
        Vec2D(2521249.96731385, 1200749.9402584787),
        Vec2D(2527249.951458698, 1207249.9519499454),
        Vec2D(2540249.945291664, 1210249.9739189378),
        Vec2D(2539249.9310220755, 1216749.9768762388),
        Vec2D(2559249.911010254, 1233750.0250706144),
        Vec2D(2567249.906174939, 1243250.0498991741),
        Vec2D(2556249.887021004, 1242750.0333215091),
        Vec2D(2566749.861025399, 1264250.0877615795),
        Vec2D(2584249.9217585553, 1262750.1053500893)
    });
    shader->setColor(0, 1, 0, 0.5);

    auto config = RenderPassConfig(0);
    auto vector = { renderConfig[0]->getGraphicsObject() };
    renderPass = std::make_shared<RenderPass>(config, vector);
}

std::vector<std::shared_ptr<::RenderPassInterface>> TestingLayer::buildRenderPasses() {
    return { renderPass };
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
