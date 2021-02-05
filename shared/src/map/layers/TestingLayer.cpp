#include "TestingLayer.h"
#include "RenderPassConfig.h"
#include "LambdaTask.h"

TestingLayer::TestingLayer(const std::shared_ptr<MapInterface> &mapInterface) : mapInterface(mapInterface) {
    auto shader = mapInterface->getShaderFactory()->createColorShader();
    auto shader2 = mapInterface->getShaderFactory()->createColorShader();
    rectangle = mapInterface->getGraphicsObjectFactory()->createRectangle(shader2->asShaderProgramInterface());
    rectangle2 = mapInterface->getGraphicsObjectFactory()->createRectangle(shader->asShaderProgramInterface());
    object = std::make_shared<Polygon2dLayerObject>(
            mapInterface->getGraphicsObjectFactory()->createPolygon(shader->asShaderProgramInterface()), shader);
    auto renderConfig = object->getRenderConfig();
    rectangle->setFrame(RectD(2485071.58, 1075346.31, 343444.24, 224595.48), RectD(0, 0, 1, 1));
    rectangle2->setFrame(RectD(2641681.14, 1158846.230, 1000, 1000), RectD(0, 0, 1, 1));
    object->setPositions({Vec2D(2481483.3, 1107166.7),
                          Vec2D(2569883.3, 1263166.7),
                          Vec2D(2685583.3, 1299566.7),
                          Vec2D(2825333.3, 1209216.7),
                          Vec2D(2830533.3, 1159166.7),
                          Vec2D(2806483.3, 1126666.7),
                          Vec2D(2742133.3, 1149416.7),
                          Vec2D(2718733.3, 1076616.7),
                          Vec2D(2671283.3, 1143566.7),
                          Vec2D(2629033.3, 1084416.7),
                          Vec2D(2571833.3, 1081166.7),
                          Vec2D(2534133.3, 1141616.7),
                          Vec2D(2514633.3, 1123416.7),
                          Vec2D(2499033.3, 1109116.7),
                          Vec2D(2491333.3, 1109833.3)});
    shader->setColor(0, 1, 0, 0.5);
    shader2->setColor(1, 0, 0, 0.5);

    auto config = RenderPassConfig(0);
    auto vector = {rectangle->asGraphicsObject(), rectangle2->asGraphicsObject(), renderConfig[0]->getGraphicsObject()};
    renderPass = std::make_shared<RenderPass>(config, vector);
}

std::vector<std::shared_ptr<::RenderPassInterface>> TestingLayer::buildRenderPasses() {
    return {renderPass};
}

std::string TestingLayer::getIdentifier() {
    return "TestingLayer";
}

void TestingLayer::pause() {
    object->getPolygonObject()->clear();
}

void TestingLayer::resume() {
    auto renderingContext = mapInterface->getRenderingContext();
    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
            TaskConfig("PolygonSchweiz_Setup", 0, TaskPriority::HIGH, ExecutionEnvironment::GRAPHICS),
            [=] {
                object->getPolygonObject()->setup(renderingContext);
                rectangle->asGraphicsObject()->setup(renderingContext);
                rectangle2->asGraphicsObject()->setup(renderingContext);
            }));
}

void TestingLayer::hide() {

}

void TestingLayer::show() {

}
