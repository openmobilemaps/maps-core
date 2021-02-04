#pragma once

#include "SimpleTouchInterface.h"
#include "CameraInterface.h"
#include "MapCamera2dInterface.h"
#include "MapCoordinateSystem.h"
#include "MapCamera2dListenerInterface.h"
#include <set>

class MapCamera2d
        : public MapCamera2dInterface,
          public CameraInterface,
          public SimpleTouchInterface,
          public std::enable_shared_from_this<CameraInterface> {
public:
    MapCamera2d(
            const std::shared_ptr<MapInterface> &mapInterface,
            float screenDensityPpi);

    ~MapCamera2d() {};

    virtual void moveToCenterPositionZoom(const ::Vec2D &centerPosition, double zoom, bool animated);

    virtual void moveToCenterPosition(const ::Vec2D &centerPosition, bool animated);

    virtual ::Vec2D getCenterPosition();

    virtual void setZoom(double zoom, bool animated);

    virtual double getZoom();

    virtual void setPaddingLeft(float padding);

    virtual void setPaddingRight(float padding);

    virtual void setPaddingTop(float padding);

    virtual void setPaddingBottom(float padding);

    virtual void addListener(const std::shared_ptr<MapCamera2dListenerInterface> &listener);

    virtual void removeListener(const std::shared_ptr<MapCamera2dListenerInterface> &listener);

    virtual std::shared_ptr<::CameraInterface> asCameraInterface();

    virtual std::vector<float> getMvpMatrix();

    virtual bool onMove(const ::Vec2F &deltaScreen, bool confirmed, bool doubleClick);

protected:

    std::set<std::shared_ptr<MapCamera2dListenerInterface>> listeners;

    std::vector<float> currentMvpMatrix;

    std::shared_ptr<MapInterface> mapInterface;
    MapCoordinateSystem mapCoordinateSystem;
    float screenDensityPpi;
    double screenPixelAsRealMeterFactor;

    Vec2D centerPosition = Vec2D(0.0, 0.0);
    double zoom;
    double angle;

    double paddingLeft;
    double paddingTop;
    double paddingRight;
    double paddingBottom;

};
