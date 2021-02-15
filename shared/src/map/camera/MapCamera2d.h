#pragma once

#include "SimpleTouchInterface.h"
#include "CameraInterface.h"
#include "MapCamera2dInterface.h"
#include "MapCoordinateSystem.h"
#include "MapCamera2dListenerInterface.h"
#include "CoordinateConversionHelperInterface.h"
#include <set>
#include <optional>
#include "Coord.h"

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

    virtual void moveToCenterPositionZoom(const ::Coord &centerPosition, double zoom, bool animated);

    virtual void moveToCenterPosition(const ::Coord &centerPosition, bool animated);

    virtual ::Coord getCenterPosition();

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

    virtual bool onDoubleClick(const ::Vec2F &posScreen);

    virtual bool onTwoFingerMove(const std::vector< ::Vec2F> &posScreenOld, const std::vector< ::Vec2F> &posScreenNew);

    virtual void viewportSizeChanged();

    RectCoord getVisibileRect();

protected:

    std::set<std::shared_ptr<MapCamera2dListenerInterface>> listeners;

    std::shared_ptr<MapInterface> mapInterface;
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
    MapCoordinateSystem mapCoordinateSystem;
    float screenDensityPpi;
    double screenPixelAsRealMeterFactor;

    Coord centerPosition;
    double zoom = 0;
    double angle = 0;

    double paddingLeft = 0;
    double paddingTop = 0;
    double paddingRight = 0;
    double paddingBottom = 0;

    struct CameraConfiguration {
        bool roationEnabled = false;
        bool doubleClickZoomEnabled = true;
        bool twoFingerZoomEnabled = true;
        bool moveEnabled = true;
    };
              
    CameraConfiguration config;

    void notifyListeners();

    Coord coordFromScreenPosition(const ::Vec2F &posScreen);
    // MARK: Animations

    struct CameraAnimation {
      Coord startCenterPosition;
      double startZoom;
      Coord targetCenterPosition;
      double targetZoom;
      long long startTime;
      long long duration;
    };

    std::optional<CameraAnimation> cameraAnimation;

    void beginAnimation(double zoom, Coord centerPosition);
    void applyAnimationState();
};
