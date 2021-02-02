#include "MapScene.h"
#include "MapCallbackInterface.h"

MapScene::MapScene(std::shared_ptr<SceneInterface> scene, const MapConfig & mapConfig, const std::shared_ptr<::SchedulerInterface> & scheduler):
scene(scene), mapConfig(mapConfig), scheduler(scheduler){
}

void MapScene::setCallbackHandler(const std::shared_ptr<MapCallbackInterface> & callbackInterface) {
    scene->setCallbackHandler(shared_from_this());
    callbackHandler = callbackInterface;
}

void MapScene::setCamera(const std::shared_ptr<::CameraInterface> & camera) {
    scene->setCallbackHandler(camera);
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

}

void MapScene::addLayer(const std::shared_ptr<::LayerInterface> & layer) {

}

void MapScene::removeLayer(const std::shared_ptr<::LayerInterface> & layer) {

}

void MapScene::invalidate() {
    if (auto handler = callbackHandler) {
        handler->invalidate();
    }
}

void MapScene::drawFrame() {
    scene->drawFrame();
}

void MapScene::resume() {

}

void MapScene::pause() {

}
