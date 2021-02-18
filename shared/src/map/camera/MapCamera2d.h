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

    virtual void moveToCenterPositionZoom(const ::Coord &centerPosition, double zoom, bool animated) override;

    virtual void moveToCenterPosition(const ::Coord &centerPosition, bool animated) override;

    virtual ::Coord getCenterPosition() override;

    virtual void setZoom(double zoom, bool animated) override;

    virtual double getZoom() override;

    virtual void setPaddingLeft(float padding) override;

    virtual void setPaddingRight(float padding) override;

    virtual void setPaddingTop(float padding) override;

    virtual void setPaddingBottom(float padding) override;

    virtual void addListener(const std::shared_ptr<MapCamera2dListenerInterface> &listener) override;

    virtual void removeListener(const std::shared_ptr<MapCamera2dListenerInterface> &listener) override;

    virtual std::shared_ptr<::CameraInterface> asCameraInterface() override;

    virtual std::vector<float> getMvpMatrix() override;

    virtual bool onMove(const ::Vec2F &deltaScreen, bool confirmed, bool doubleClick) override;

    virtual bool onDoubleClick(const ::Vec2F &posScreen) override;

    virtual bool onTwoFingerMove(const std::vector< ::Vec2F> &posScreenOld, const std::vector< ::Vec2F> &posScreenNew) override;

    virtual void viewportSizeChanged() override;

    virtual RectCoord getVisibleRect() override;

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
        bool rotationEnabled = false;
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
