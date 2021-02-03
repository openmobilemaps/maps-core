#pragma once

#include "MapCamera2dInterface.h"

class MapCamera2d: public MapCamera2dInterface {
public:
    MapCamera2d(float screenDensityPpi);
    
    ~MapCamera2d(){};

    virtual void moveToCenterPosition(const ::Vec2D & position, double zoom, bool animated);

    virtual void moveToCenterPositon(const ::Vec2D & position, bool animated);

    virtual ::Vec2D getCenterPosition();

    virtual void setZoom(double zoom, bool animated);

    virtual double getZoom();

    virtual void setPaddingLeft(float padding);

    virtual void setPaddingRight(float padding);

    virtual void setPaddingTop(float padding);

    virtual void setPaddingBottom(float padding);

    virtual void addListener(const std::shared_ptr<MapCamera2dListenerInterface> & listener);

    virtual void removeListener(const std::shared_ptr<MapCamera2dListenerInterface> & listener);

    virtual std::shared_ptr<::CameraInterface> asCameraIntercace();
};
