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
#include "Coord.h"
#include "CoordinateConversionHelperInterface.h"
#include "MapCamera2dInterface.h"
#include "MapCamera2dListenerInterface.h"
#include "MapCoordinateSystem.h"
#include "SimpleTouchInterface.h"
#include <optional>
#include <set>

class MapCamera2d : public MapCamera2dInterface,
                    public CameraInterface,
                    public SimpleTouchInterface,
                    public std::enable_shared_from_this<CameraInterface> {
  public:
    MapCamera2d(const std::shared_ptr<MapInterface> &mapInterface, float screenDensityPpi);

    ~MapCamera2d(){};

    virtual void moveToCenterPositionZoom(const ::Coord &centerPosition, double zoom, bool animated) override;

    virtual void moveToCenterPosition(const ::Coord &centerPosition, bool animated) override;

    virtual ::Coord getCenterPosition() override;

    virtual void setZoom(double zoom, bool animated) override;

    virtual double getZoom() override;

    virtual void setRotation(float angle, bool animated) override;

    virtual float getRotation() override;

    virtual void setMinZoom(double zoomMin) override;

    virtual void setMaxZoom(double zoomMax) override;

    virtual void setBounds(const ::RectCoord & bounds) override;

    virtual void setPaddingLeft(float padding, bool animated) override;

    virtual void setPaddingRight(float padding, bool animated) override;

    virtual void setPaddingTop(float padding, bool animated) override;

    virtual void setPaddingBottom(float padding, bool animated) override;

    virtual void addListener(const std::shared_ptr<MapCamera2dListenerInterface> &listener) override;

    virtual void removeListener(const std::shared_ptr<MapCamera2dListenerInterface> &listener) override;

    virtual std::shared_ptr<::CameraInterface> asCameraInterface() override;

    virtual std::vector<float> getMvpMatrix() override;

    virtual bool onMove(const ::Vec2F &deltaScreen, bool confirmed, bool doubleClick) override;

    virtual bool onDoubleClick(const ::Vec2F &posScreen) override;

    virtual bool onTwoFingerMove(const std::vector<::Vec2F> &posScreenOld, const std::vector<::Vec2F> &posScreenNew) override;

    virtual void clearTouch() override;

    virtual void viewportSizeChanged() override;

    virtual RectCoord getVisibleRect() override;

    virtual ::Coord coordFromScreenPosition(const ::Vec2F &posScreen) override;

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
    double tempAngle = 0;
    bool isRotationThreasholdReached = false;

    double paddingLeft = 0;
    double paddingTop = 0;
    double paddingRight = 0;
    double paddingBottom = 0;

    double zoomMin = -1;
    double zoomMax = 200.0;

    RectCoord bounds;

    struct CameraConfiguration {
        bool rotationEnabled = true;
        bool doubleClickZoomEnabled = true;
        bool twoFingerZoomEnabled = true;
        bool moveEnabled = true;
    };

    CameraConfiguration config;

    void notifyListeners();

    // MARK: Animations

    struct CameraAnimation {
        Coord startCenterPosition;
        double startZoom;
        double startRotation;
        Coord targetCenterPosition;
        double targetZoom;
        double targetRotation;
        long long startTime;
        long long duration;
    };

    std::optional<CameraAnimation> cameraAnimation;

    void beginAnimation(double zoom, Coord centerPosition);
    void beginAnimation(double rotationAngle);
    void applyAnimationState();

    Coord getBoundsCorrectedCoords(const Coord &coords);

};
