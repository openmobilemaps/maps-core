#include "MapScene.h"
#include "MapCallbackInterface.h"
#include "TestingLayer.h"
#include <algorithm>

MapScene::MapScene(std::shared_ptr<SceneInterface> scene, const MapConfig & mapConfig, const std::shared_ptr<::SchedulerInterface> & scheduler):
scene(scene), mapConfig(mapConfig), scheduler(scheduler){
    addLayer(std::make_shared<TestingLayer>(scene->getGraphicsFactory(), scene->getShaderFactory()));
}

std::shared_ptr<::RenderingContextInterface> MapScene::getRenderingContext() {
    return scene->getRenderingContext();
}

void MapScene::setCallbackHandler(const std::shared_ptr<MapCallbackInterface> & callbackInterface) {
    scene->setCallbackHandler(shared_from_this());
    callbackHandler = callbackInterface;
}

void MapScene::setCamera(const std::shared_ptr<::CameraInterface> & camera) {
    scene->setCamera(camera);
}

std::shared_ptr<::CameraInterface> MapScene::getCamera() {
    return scene->getCamera();
}

void MapScene::setTouchHandler(const std::shared_ptr<::TouchHandlerInterface> & touchHandler) {

}

std::shared_ptr<::TouchHandlerInterface> MapScene::getTouchHandler() {
    return nullptr;
}

void MapScene::setLoader(const std::shared_ptr<::LoaderInterface> & loader) {
    this->loader = loader;
}

void MapScene::addLayer(const std::shared_ptr<::LayerInterface> & layer) {
    layers.push_back(layer);
}

void MapScene::removeLayer(const std::shared_ptr<::LayerInterface> & layer) {
    layers.erase(std::remove(layers.begin(), layers.end(), layer), layers.end());
}

void MapScene::invalidate() {
    if (auto handler = callbackHandler) {
        handler->invalidate();
    }
}

void MapScene::drawFrame() {
    for (const auto &layer : layers) {
        for (const auto &renderPass : layer->buildRenderPasses()) {
            scene->getRenderer()->addToRenderQueue(renderPass);
        }
    }

    scene->drawFrame();
}

void MapScene::resume() {

}

void MapScene::pause() {

}
