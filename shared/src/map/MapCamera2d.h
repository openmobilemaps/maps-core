#pragma once

#include "MapCamera2dInterface.h"

class MapCamera2d: public MapCamera2dInterface {

    MapCamera2d();
    
    ~MapCamera2d() {};

    virtual void moveToCenterPosition(const ::Vec2F & position, float zoom, bool animated);

    virtual void moveToCenterPositon(const ::Vec2F & position, bool animated);

    virtual ::Vec2F getCenterPosition();

    virtual void setZoom(float zoom, bool animated);

    virtual float getZoom();

    virtual void setPaddingLeft(float padding);

    virtual void setPaddingRight(float padding);

    virtual void setPaddingTop(float padding);

    virtual void setPaddingBottom(float padding);

    virtual void addListener(const std::shared_ptr<MapCamera2dListenerInterface> & listener);

    virtual void removeListener(const std::shared_ptr<MapCamera2dListenerInterface> & listener);

    virtual std::shared_ptr<::CameraInterface> asCameraIntercace();
}
