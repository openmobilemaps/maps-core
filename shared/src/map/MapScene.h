#pragma once

#include "Scene.h"
#include "MapInterface.h"
#include "MapConfig.h"

class MapScene: public MapInterface, public SceneCallbackInterface, public std::enable_shared_from_this<MapScene> {
public:
    MapScene(std::shared_ptr<SceneInterface> scene, const MapConfig & mapConfig, const std::shared_ptr<::SchedulerInterface> & scheduler, float pixelDensity);

    virtual ~MapScene() {}

    virtual std::shared_ptr<::GraphicsObjectFactoryInterface> getGraphicsObjectFactory() override;

    virtual std::shared_ptr<::ShaderFactoryInterface> getShaderFactory() override;

    virtual std::shared_ptr<::SchedulerInterface> getScheduler() override;

    virtual std::shared_ptr<::RenderingContextInterface> getRenderingContext() override;

    virtual MapConfig getMapConfig() override;

    virtual std::shared_ptr<::CoordinateConversionHelperInterface> getCoordinateConverterHelper() override;

    virtual void setCallbackHandler(const std::shared_ptr<MapCallbackInterface> & callbackInterface) override;

    virtual void setCamera(const std::shared_ptr<::CameraInterface> & camera) override;

    virtual std::shared_ptr<::CameraInterface> getCamera() override;

    virtual void setTouchHandler(const std::shared_ptr<::TouchHandlerInterface> & touchHandler) override;

    virtual std::shared_ptr<::TouchHandlerInterface> getTouchHandler() override;

    virtual void addLayer(const std::shared_ptr<::LayerInterface> & layer) override;

    virtual void removeLayer(const std::shared_ptr<::LayerInterface> & layer) override;

    virtual void setViewportSize(const ::Vec2I & size) override;

    virtual void setBackgroundColor(const Color &color) override;

    virtual void invalidate() override;

    virtual void drawFrame() override;

    virtual void resume() override;

    virtual void pause() override;
private:

    const MapConfig mapConfig;
    
    std::shared_ptr<MapCallbackInterface> callbackHandler;

    std::shared_ptr<SchedulerInterface> scheduler;

    std::shared_ptr<SceneInterface> scene;

    std::vector<std::shared_ptr<LayerInterface>> layers;

    std::shared_ptr<TouchHandlerInterface> touchHandler;

    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;

    bool isResumed = false;
};
