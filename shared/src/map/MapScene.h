#pragma once

#include "MapInterface.h"

class MapScene: public MapInterface {
public:
    MapScene();

    virtual ~MapScene() {}

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
};
