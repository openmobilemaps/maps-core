#include "MapScene.h"

std::shared_ptr<MapInterface> MapInterface::create(const std::shared_ptr<::GraphicsObjectFactoryInterface> & graphicsFactory,
                                                    const std::shared_ptr<::ShaderFactoryInterface> & shaderFactory,
                                                    const MapConfig & mapConfig,
                                                    const std::shared_ptr<::SchedulerInterface> & scheduler) {
    return std::make_shared<MapScene>();
}

std::shared_ptr<MapInterface> MapInterface::createWithOpenGl(const MapConfig & mapConfig, const std::shared_ptr<::SchedulerInterface> & scheduler) {
#ifdef __ANDROID__
    return std::make_shared<MapScene>();
#else
    return nullptr;
#endif
}
