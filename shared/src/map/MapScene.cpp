#include "MapScene.h"

MapScene::MapScene() {

}

void MapScene::setCallbackHandler(const std::shared_ptr<MapCallbackInterface> & callbackInterface) {

}

void MapScene::setCamera(const std::shared_ptr<::CameraInterface> & camera) {

}

std::shared_ptr<::CameraInterface> MapScene::getCamera() {
    return nullptr;
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

}

void MapScene::drawFrame() {

}

void MapScene::resume() {

}

void MapScene::pause() {

}
