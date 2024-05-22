/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "CameraInterface.h"
#include "MapCamera3dInterface.h"
#include "Coord.h"
#include "CoordAnimation.h"
#include "CoordinateConversionHelperInterface.h"
#include "DoubleAnimation.h"
#include "MapCameraInterface.h"
#include "MapCameraListenerInterface.h"
#include "MapCoordinateSystem.h"
#include "SimpleTouchInterface.h"
#include "Vec2I.h"
#include "Vec2F.h"
#include "CameraMode3d.h"
#include <mutex>
#include <optional>
#include <set>

class MapCamera3d : public MapCameraInterface,
                    public MapCamera3dInterface,
                    public CameraInterface,
                    public SimpleTouchInterface,
                    public std::enable_shared_from_this<MapCamera3d> {
  public:
    MapCamera3d(const std::shared_ptr<MapInterface> &mapInterface, float screenDensityPpi);

    ~MapCamera3d(){};

    void freeze(bool freeze) override;

    virtual void moveToCenterPositionZoom(const ::Coord &centerPosition, double zoom, bool animated) override;

    virtual void moveToCenterPosition(const ::Coord &centerPosition, bool animated) override;

    virtual void moveToBoundingBox(const ::RectCoord &boundingBox, float paddingPc, bool animated,
                                   std::optional<double> minZoom, std::optional<double> maxZoom) override;

    virtual ::Coord getCenterPosition() override;

    virtual void setZoom(double zoom, bool animated) override;

    virtual double getZoom() override;

    virtual void setRotation(float angle, bool animated) override;

    virtual float getRotation() override;

    virtual void setMinZoom(double zoomMin) override;

    virtual void setMaxZoom(double zoomMax) override;

    virtual double getMinZoom() override;

    virtual double getMaxZoom() override;

    virtual void setBounds(const ::RectCoord &bounds) override;

    virtual ::RectCoord getBounds() override;

    virtual bool isInBounds(const ::Coord &coords) override;

    virtual void setPaddingLeft(float padding) override;

    virtual void setPaddingRight(float padding) override;

    virtual void setPaddingTop(float padding) override;

    virtual void setPaddingBottom(float padding) override;

    virtual void addListener(const std::shared_ptr<MapCameraListenerInterface> &listener) override;

    virtual void removeListener(const std::shared_ptr<MapCameraListenerInterface> &listener) override;

    virtual std::shared_ptr<::CameraInterface> asCameraInterface() override;

    virtual std::vector<float> getVpMatrix() override;

    std::optional<std::vector<float>> getLastVpMatrix() override;

    std::optional<::RectCoord> getLastVpMatrixViewBounds() override;

    std::optional<float> getLastVpMatrixRotation() override;

    std::optional<float> getLastVpMatrixZoom() override;

    /** this method is called just before the update methods on all layers */
    virtual void update() override;

    virtual std::vector<float> getInvariantModelMatrix(const ::Coord &coordinate, bool scaleInvariant,
                                                       bool rotationInvariant) override;

    virtual bool onTouchDown(const ::Vec2F &posScreen) override;

    virtual bool onMove(const ::Vec2F &deltaScreen, bool confirmed, bool doubleClick) override;

    virtual bool onMoveComplete() override;

    virtual bool onTwoFingerClick(const ::Vec2F &posScreen1, const ::Vec2F &posScreen2) override;

    virtual bool onDoubleClick(const ::Vec2F &posScreen) override;

    virtual bool onTwoFingerMove(const std::vector<::Vec2F> &posScreenOld, const std::vector<::Vec2F> &posScreenNew) override;

    virtual bool onTwoFingerMoveComplete() override;

    virtual void clearTouch() override;

    virtual void viewportSizeChanged() override;

    virtual RectCoord getVisibleRect() override;

    virtual RectCoord getPaddingAdjustedVisibleRect() override;

    virtual ::Coord coordFromScreenPosition(const ::Vec2F &posScreen) override;

    Vec2F screenPosFromCoord(const Coord &coord) override;

    bool coordIsVisibleOnScreen(const ::Coord & coord, float paddingPc) override;

    bool gluInvertMatrix(const std::vector<double> &m, std::vector<double> &invOut);

    virtual double mapUnitsFromPixels(double distancePx) override;

    virtual double getScalingFactor() override;

    virtual void setRotationEnabled(bool enabled) override;

    virtual void setSnapToNorthEnabled(bool enabled) override;

    void setBoundsRestrictWholeVisibleRect(bool centerOnly) override;


    virtual float getScreenDensityPpi() override;

    std::shared_ptr<MapCamera3dInterface> asMapCamera3d() override;

    void setCameraMode(CameraMode3d mode) override;

    void notifyListenerBoundsChange() override;

protected:
    void updateZoom(double zoom);
    virtual void setupInertia();

    double getCameraDistance();
    double getCameraVerticalDisplacement();
    double getCameraPitch();

    std::recursive_mutex listenerMutex;
    std::set<std::shared_ptr<MapCameraListenerInterface>> listeners;

    std::shared_ptr<MapInterface> mapInterface;
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
    MapCoordinateSystem mapCoordinateSystem;
    bool mapSystemRtl;
    bool mapSystemTtb;
    float screenDensityPpi;
    double screenPixelAsRealMeterFactor;

    CameraMode3d mode = CameraMode3d::GLOBE;

    Coord focusPointPosition;
    double cameraVerticalDisplacement = 0.0;
    double cameraPitch = 0; // looking up or down

    double zoom;
    double angle = 0;
    double tempAngle = 0;
    bool isRotationThresholdReached = false;
    bool rotationPossible = true;
    double startZoom = 0;

    double paddingLeft = 0;
    double paddingTop = 0;
    double paddingRight = 0;
    double paddingBottom = 0;

    double zoomMin = 10000000;
    double zoomMax = 10000;

    RectCoord bounds;

    std::recursive_mutex vpDataMutex;
    std::optional<RectCoord> lastVpBounds = std::nullopt;
    std::optional<double> lastVpRotation = std::nullopt;
    std::optional<double> lastVpZoom = std::nullopt;

    bool cameraFrozen = false;

    struct CameraConfiguration {
        bool rotationEnabled = true;
        bool doubleClickZoomEnabled = true;
        bool twoFingerZoomEnabled = true;
        bool moveEnabled = true;
        bool snapToNorthEnabled = true;
        bool boundsRestrictWholeVisibleRect = false;
    };

    long long currentDragTimestamp = 0;
    Vec2F currentDragVelocity = {0, 0};

    /// object describing parameters of inertia
    /// currently only dragging inertia is implemented
    /// zoom and rotation are still missing
    struct Inertia {
        long timestampStart;
        long timestampUpdate;
        Vec2F velocity;
        double t1;
        double t2;

        Inertia(long long timestampStart, Vec2F velocity, double t1, double t2)
            : timestampStart(timestampStart)
            , timestampUpdate(timestampStart)
            , velocity(velocity)
            , t1(t1)
            , t2(t2) {}
    };
    std::optional<Inertia> inertia;
    void inertiaStep();

    CameraConfiguration config;

    enum ListenerType { BOUNDS = 1, ROTATION = 1 << 1, MAP_INTERACTION = 1 << 2 };

    void notifyListeners(const int &listenerType);

    // MARK: Animations

    std::recursive_mutex animationMutex;
    std::shared_ptr<CoordAnimation> coordAnimation;
    std::shared_ptr<DoubleAnimation> zoomAnimation;
    std::shared_ptr<DoubleAnimation> rotationAnimation;
    std::shared_ptr<DoubleAnimation> pitchAnimation;

    std::tuple<Coord, double> getBoundsCorrectedCoords(const Coord &position, double zoom);

    RectCoord getPaddingCorrectedBounds(double zoom);
    Coord adjustCoordForPadding(const Coord &coords, double targetZoom);

    void clampCenterToPaddingCorrectedBounds();

    RectCoord getRectFromViewport(const Vec2I &sizeViewport, const Coord &center);

    std::vector<float> vpMatrix = std::vector<float>(16, 0.0);
    std::vector<float> viewMatrix = std::vector<float>(16, 0.0);
    std::vector<float> projectionMatrix = std::vector<float>(16, 0.0);
    float verticalFov;
    float horizontalFov;
    bool validVpMatrix = false;
                        
    std::optional<::Vec2F> lastOnTouchDownPoint;
    std::optional<::Vec2F> initialTouchDownPoint;
    std::optional<Coord> lastOnTouchDownCoord;
};
