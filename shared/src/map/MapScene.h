#pragma once

#include "Scene.h"
#include "MapInterface.h"
#include "MapConfig.h"

class MapScene: public MapInterface, public SceneCallbackInterface, public std::enable_shared_from_this<MapScene> {
public:
    MapScene(std::shared_ptr<SceneInterface> scene, const MapConfig & mapConfig, const std::shared_ptr<::SchedulerInterface> & scheduler);

    virtual ~MapScene() {}

    virtual std::shared_ptr<::RenderingContextInterface> getRenderingContext();

    virtual void setCallbackHandler(const std::shared_ptr<MapCallbackInterface> & callbackInterface);

    virtual void setCamera(const std::shared_ptr<::CameraInterface> & camera);

    virtual std::shared_ptr<::CameraInterface> getCamera();

    virtual void setTouchHandler(const std::shared_ptr<::TouchHandlerInterface> & touchHandler);

    virtual std::shared_ptr<::TouchHandlerInterface> getTouchHandler();

    virtual void setLoader(const std::shared_ptr<::LoaderInterface> & loader);

    virtual void addLayer(const std::shared_ptr<::LayerInterface> & layer);

    virtual void removeLayer(const std::shared_ptr<::LayerInterface> & layer);

    virtual void invalidate();

    virtual void drawFrame();

    virtual void resume();

    virtual void pause();
private:

    const MapConfig mapConfig;
    
    std::shared_ptr<MapCallbackInterface> callbackHandler;

    std::shared_ptr<SchedulerInterface> scheduler;

    std::shared_ptr<SceneInterface> scene;
};
