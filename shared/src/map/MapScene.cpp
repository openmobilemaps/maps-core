#include "MapScene.h"
#include "MapCallbackInterface.h"
#include "TestingLayer.h"
#include "DefaultTouchHandler.h"
#include "TouchInterface.h"
#include <algorithm>
#include "CoordinateConversionHelper.h"

MapScene::MapScene(std::shared_ptr<SceneInterface> scene, const MapConfig & mapConfig, const std::shared_ptr<::SchedulerInterface> & scheduler):
scene(scene),
mapConfig(mapConfig),
scheduler(scheduler),
conversionHelper(std::make_shared<CoordinateConversionHelper>(mapConfig.mapCoordinateSystem)){
}

std::shared_ptr<::GraphicsObjectFactoryInterface> MapScene::getGraphicsObjectFactory() {
    return scene->getGraphicsFactory();
}

std::shared_ptr<::ShaderFactoryInterface> MapScene::getShaderFactory() {
    return scene->getShaderFactory();
}

std::shared_ptr<::SchedulerInterface> MapScene::getScheduler() {
    return scheduler;
}

std::shared_ptr<::RenderingContextInterface> MapScene::getRenderingContext() {
    return scene->getRenderingContext();
}

MapConfig MapScene::getMapConfig() {
    return mapConfig;
}

std::shared_ptr<::CoordinateConversionHelperInterface> MapScene::getCoordinateConverterHelper() {
    return conversionHelper;
}


void MapScene::setCallbackHandler(const std::shared_ptr<MapCallbackInterface> & callbackInterface) {
    scene->setCallbackHandler(shared_from_this());
    callbackHandler = callbackInterface;
}

void MapScene::setCamera(const std::shared_ptr<::CameraInterface> & camera) {
    if(touchHandler && std::dynamic_pointer_cast<TouchInterface>(camera)) {
        auto prevCamera = std::dynamic_pointer_cast<TouchInterface>(scene->getCamera());
        if (prevCamera) {
            touchHandler->removeListener(prevCamera);
        }
        auto newCamera = std::dynamic_pointer_cast<TouchInterface>(camera);
        touchHandler->addListener(newCamera);
    }
    scene->setCamera(camera);
}

std::shared_ptr<::CameraInterface> MapScene::getCamera() {
    return scene->getCamera();
}

void MapScene::addDefaultTouchHandler(float density) {
    auto touchHandler = std::make_shared<DefaultTouchHandler>(scheduler, density);
    setTouchHandler(touchHandler);
}

void MapScene::setTouchHandler(const std::shared_ptr<::TouchHandlerInterface> & touchHandler) {
    auto currentCamera = std::dynamic_pointer_cast<TouchInterface>(scene->getCamera());
    if (this->touchHandler && currentCamera) {
        this->touchHandler->removeListener(currentCamera);
    }
    this->touchHandler = touchHandler;
    if (currentCamera) {
        this->touchHandler->addListener(currentCamera);
    }
}

std::shared_ptr<::TouchHandlerInterface> MapScene::getTouchHandler() {
    return touchHandler;
}

void MapScene::addLayer(const std::shared_ptr<::LayerInterface> & layer) {
    layer->onAdded();
    layers.push_back(layer);

    layers.push_back(std::make_shared<TestingLayer>(shared_from_this()));
}

void MapScene::removeLayer(const std::shared_ptr<::LayerInterface> & layer) {
    layer->onRemoved();
    layers.erase(std::remove(layers.begin(), layers.end(), layer), layers.end());
}


void MapScene::setViewportSize(const ::Vec2I & size) {
    getRenderingContext()->setViewportSize(size);
    getCamera()->viewportSizeChanged();
}

void MapScene::invalidate() {
    if (auto handler = callbackHandler) {
        handler->invalidate();
    }
}

void MapScene::drawFrame() {
    if (!isResumed) return;

    for (const auto &layer : layers) {
        for (const auto &renderPass : layer->buildRenderPasses()) {
            scene->getRenderer()->addToRenderQueue(renderPass);
        }
    }

    scene->drawFrame();
}

void MapScene::resume() {
    isResumed = true;
    for (const auto &layer : layers) {
        layer->resume();
    }
}

void MapScene::pause() {
    isResumed = false;
    for (const auto &layer : layers) {
        layer->pause();
    }
}
