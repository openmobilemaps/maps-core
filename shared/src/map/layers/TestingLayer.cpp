#include "TestingLayer.h"
#include "RenderPassConfig.h"
#include "LambdaTask.h"
#include "MapConfig.h"
#include "Coord.h"
#include "AlphaShaderInterface.h"

TestingLayer::TestingLayer(const std::shared_ptr<MapInterface> &mapInterface) : mapInterface(mapInterface) {
    auto shader = mapInterface->getShaderFactory()->createColorShader();
    auto shader2 = mapInterface->getShaderFactory()->createColorShader();
    auto texture = mapInterface->getShaderFactory()->createAlphaShader();
    rectangle = mapInterface->getGraphicsObjectFactory()->createRectangle(shader2->asShaderProgramInterface());
    rectangle2 = mapInterface->getGraphicsObjectFactory()->createRectangle(shader->asShaderProgramInterface());
    rectangle3 = mapInterface->getGraphicsObjectFactory()->createRectangle(texture->asShaderProgramInterface());
    rectangle4 = mapInterface->getGraphicsObjectFactory()->createRectangle(shader2->asShaderProgramInterface());
    polygonObject = std::make_shared<Polygon2dLayerObject>(mapInterface->getCoordinateConverterHelper(),
                                                           mapInterface->getGraphicsObjectFactory()->createPolygon(
                                                                   shader->asShaderProgramInterface()), shader);
    auto renderConfig = polygonObject->getRenderConfig();
    rectangle->setFrame(RectD(-1000, -1000000, 2000, 2000000), RectD(0, 0, 1, 1));
    rectangle2->setFrame(RectD(-1000000, -1000, 2000000, 2000), RectD(0, 0, 1, 1));
    rectangle3->setFrame(RectD(0, 0, 200000, 200000), RectD(0, 0, 1, 1));
    rectangle4->setFrame(RectD(0, 0, 20000, 20000), RectD(0, 0, 1, 1));
    std::string mapCoordSystemId = mapInterface->getMapConfig().mapCoordinateSystem.identifier;
    polygonObject->setPositions({Coord(mapCoordSystemId, 2481483.3, 1107166.7, 0),
                                 Coord(mapCoordSystemId, 2569883.3, 1263166.7, 0),
                                 Coord(mapCoordSystemId, 2685583.3, 1299566.7, 0),
                                 Coord(mapCoordSystemId, 2825333.3, 1209216.7, 0),
                                 Coord(mapCoordSystemId, 2830533.3, 1159166.7, 0),
                                 Coord(mapCoordSystemId, 2806483.3, 1126666.7, 0),
                                 Coord(mapCoordSystemId, 2742133.3, 1149416.7, 0),
                                 Coord(mapCoordSystemId, 2718733.3, 1076616.7, 0),
                                 Coord(mapCoordSystemId, 2671283.3, 1143566.7, 0),
                                 Coord(mapCoordSystemId, 2629033.3, 1084416.7, 0),
                                 Coord(mapCoordSystemId, 2571833.3, 1081166.7, 0),
                                 Coord(mapCoordSystemId, 2534133.3, 1141616.7, 0),
                                 Coord(mapCoordSystemId, 2514633.3, 1123416.7, 0),
                                 Coord(mapCoordSystemId, 2499033.3, 1109116.7, 0),
                                 Coord(mapCoordSystemId, 2491333.3, 1109833.3, 0)});
    shader->setColor(0, 1, 0, 0.25);
    shader2->setColor(1, 0, 0, 0.25);

    auto config = RenderPassConfig(0);
    auto vector = {rectangle3->asGraphicsObject(), rectangle->asGraphicsObject(), rectangle2->asGraphicsObject(),
                   rectangle4->asGraphicsObject(), renderConfig[0]->getGraphicsObject()};
    renderPass = std::make_shared<RenderPass>(config, vector);
}

void TestingLayer::update() {

}

std::vector<std::shared_ptr<::RenderPassInterface>> TestingLayer::buildRenderPasses() {
    return {renderPass};
}

std::string TestingLayer::getIdentifier() {
    return "TestingLayer";
}

void TestingLayer::onAdded(const std::shared_ptr<::MapInterface> & mapInterface) {

}

void TestingLayer::onRemoved() {

}


void TestingLayer::pause() {
    polygonObject->getPolygonObject()->clear();
}

void TestingLayer::resume() {
    auto renderingContext = mapInterface->getRenderingContext();
    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
            TaskConfig("PolygonSchweiz_Setup", 0, TaskPriority::HIGH, ExecutionEnvironment::GRAPHICS),
            [=] {
                polygonObject->getPolygonObject()->setup(renderingContext);
                rectangle->asGraphicsObject()->setup(renderingContext);
                rectangle2->asGraphicsObject()->setup(renderingContext);
                rectangle4->asGraphicsObject()->setup(renderingContext);
            }));
}

void TestingLayer::hide() {

}

void TestingLayer::show() {

}
