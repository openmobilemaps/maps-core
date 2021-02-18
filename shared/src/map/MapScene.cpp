#include "MapScene.h"
#include "CoordinateConversionHelper.h"
#include "DefaultTouchHandlerInterface.h"
#include "LambdaTask.h"
#include "LayerInterface.h"
#include "MapCallbackInterface.h"
#include "MapCamera2dInterface.h"
#include "TouchInterface.h"
#include <algorithm>

MapScene::MapScene(std::shared_ptr<SceneInterface> scene, const MapConfig &mapConfig,
                   const std::shared_ptr<::SchedulerInterface> &scheduler, float pixelDensity)
    : scene(scene)
    , mapConfig(mapConfig)
    , scheduler(scheduler)
    , conversionHelper(std::make_shared<CoordinateConversionHelper>(mapConfig.mapCoordinateSystem)) {
    // add default touch handler
    setTouchHandler(DefaultTouchHandlerInterface::create(scheduler, pixelDensity));

    // workaround to use sharedpointer to self from constructor
    auto ptr = std::shared_ptr<MapScene>(this, [](MapScene *) {});

    // add default camera
    setCamera(MapCamera2dInterface::create(ptr, pixelDensity));
}

std::shared_ptr<::GraphicsObjectFactoryInterface> MapScene::getGraphicsObjectFactory() { return scene->getGraphicsFactory(); }

std::shared_ptr<::ShaderFactoryInterface> MapScene::getShaderFactory() { return scene->getShaderFactory(); }

std::shared_ptr<::SchedulerInterface> MapScene::getScheduler() { return scheduler; }

std::shared_ptr<::RenderingContextInterface> MapScene::getRenderingContext() { return scene->getRenderingContext(); }

MapConfig MapScene::getMapConfig() { return mapConfig; }

std::shared_ptr<::CoordinateConversionHelperInterface> MapScene::getCoordinateConverterHelper() { return conversionHelper; }

void MapScene::setCallbackHandler(const std::shared_ptr<MapCallbackInterface> &callbackInterface) {
    scene->setCallbackHandler(shared_from_this());
    callbackHandler = callbackInterface;
}

void MapScene::setCamera(const std::shared_ptr<::MapCamera2dInterface> &camera) {
    if (touchHandler && std::dynamic_pointer_cast<TouchInterface>(camera)) {
        auto prevCamera = std::dynamic_pointer_cast<TouchInterface>(scene->getCamera());
        if (prevCamera) {
            touchHandler->removeListener(prevCamera);
        }
        auto newCamera = std::dynamic_pointer_cast<TouchInterface>(camera);
        touchHandler->addListener(newCamera);
    }
    this->camera = camera;
    scene->setCamera(camera->asCameraInterface());
}

std::shared_ptr<::MapCamera2dInterface> MapScene::getCamera() { return camera; }

void MapScene::setTouchHandler(const std::shared_ptr<::TouchHandlerInterface> &touchHandler) {
    auto currentCamera = std::dynamic_pointer_cast<TouchInterface>(scene->getCamera());
    if (this->touchHandler && currentCamera) {
        this->touchHandler->removeListener(currentCamera);
    }
    this->touchHandler = touchHandler;
    if (currentCamera) {
        this->touchHandler->addListener(currentCamera);
    }
}

std::shared_ptr<::TouchHandlerInterface> MapScene::getTouchHandler() { return touchHandler; }

std::vector<std::shared_ptr<LayerInterface>> MapScene::getLayers() { return layers; };

void MapScene::addLayer(const std::shared_ptr<::LayerInterface> &layer) {
    layer->onAdded(shared_from_this());
    std::lock_guard<std::recursive_mutex> lock(layersMutex);
    layers.push_back(layer);
}

void MapScene::insertLayerAt(const std::shared_ptr<LayerInterface> &layer, int32_t atIndex) {
    layer->onAdded(shared_from_this());
    std::lock_guard<std::recursive_mutex> lock(layersMutex);
    auto it = layers.begin() + atIndex;
    layers.insert(it, layer);
};

void MapScene::insertLayerAbove(const std::shared_ptr<LayerInterface> &layer, const std::shared_ptr<LayerInterface> &above) {
    layer->onAdded(shared_from_this());
    std::lock_guard<std::recursive_mutex> lock(layersMutex);
    auto position = std::find(std::begin(layers), std::end(layers), above);
    if (position == layers.end()) {
        throw std::invalid_argument("MapScene does not contain above layer");
    }
    layers.insert(++position, layer);
};

void MapScene::insertLayerBelow(const std::shared_ptr<LayerInterface> &layer, const std::shared_ptr<LayerInterface> &below) {
    layer->onAdded(shared_from_this());
    std::lock_guard<std::recursive_mutex> lock(layersMutex);
    auto position = std::find(std::begin(layers), std::end(layers), below);
    if (position == layers.end()) {
        throw std::invalid_argument("MapScene does not contain below layer");
    }
    layers.insert(position, layer);
};

void MapScene::removeLayer(const std::shared_ptr<::LayerInterface> &layer) {
    layer->onRemoved();
    std::lock_guard<std::recursive_mutex> lock(layersMutex);
    layers.erase(std::remove(layers.begin(), layers.end(), layer), layers.end());
}

void MapScene::setViewportSize(const ::Vec2I &size) {
    getRenderingContext()->setViewportSize(size);
    camera->asCameraInterface()->viewportSizeChanged();
}

void MapScene::setBackgroundColor(const Color &color) { getRenderingContext()->setBackgroundColor(color); }

void MapScene::invalidate() {
    if (auto handler = callbackHandler) {
        handler->invalidate();
    }
}

void MapScene::drawFrame() {
    if (!isResumed)
        return;

    for (const auto &layer : layers) {
        layer->update();
    }

    for (const auto &layer : layers) {
        for (const auto &renderPass : layer->buildRenderPasses()) {
            scene->getRenderer()->addToRenderQueue(renderPass);
        }
    }

    scene->drawFrame();
}

void MapScene::resume() {
    isResumed = true;
    scheduler->addTask(
        std::make_shared<LambdaTask>(TaskConfig("MapScene_pause", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS), [=] {
            for (const auto &layer : layers) {
                layer->resume();
            }
        }));
}

void MapScene::pause() {
    isResumed = false;
    scheduler->addTask(
        std::make_shared<LambdaTask>(TaskConfig("MapScene_pause", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS), [=] {
            for (const auto &layer : layers) {
                layer->pause();
            }
        }));
}
