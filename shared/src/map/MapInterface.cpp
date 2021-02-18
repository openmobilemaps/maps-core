#include "MapScene.h"

std::shared_ptr<MapInterface> MapInterface::create(const std::shared_ptr<::GraphicsObjectFactoryInterface> &graphicsFactory,
                                                   const std::shared_ptr<::ShaderFactoryInterface> &shaderFactory,
                                                   const std::shared_ptr<::RenderingContextInterface> &renderingContext,
                                                   const MapConfig &mapConfig,
                                                   const std::shared_ptr<::SchedulerInterface> &scheduler, float pixelDensity) {
    auto scene = SceneInterface::create(graphicsFactory, shaderFactory, renderingContext);
    return std::make_shared<MapScene>(scene, mapConfig, scheduler, pixelDensity);
}

std::shared_ptr<MapInterface> MapInterface::createWithOpenGl(const MapConfig &mapConfig,
                                                             const std::shared_ptr<::SchedulerInterface> &scheduler,
                                                             float pixelDensity) {
#ifdef __ANDROID__
    return std::make_shared<MapScene>(SceneInterface::createWithOpenGl(), mapConfig, scheduler, pixelDensity);
#else
    return nullptr;
#endif
}
