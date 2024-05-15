/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "MapCamera3d.h"
#include "Coord.h"
#include "CoordAnimation.h"
#include "DateHelper.h"
#include "DoubleAnimation.h"
#include "MapConfig.h"
#include "MapInterface.h"
#include "Matrix.h"
#include "MatrixD.h"
#include "Vec2D.h"
#include "Vec2DHelper.h"
#include "Vec2FHelper.h"
#include "Vec3DHelper.h"
#include "Logger.h"
#include "CoordinateSystemIdentifiers.h"

#include "MapCamera3DHelper.h"

#define DEFAULT_ANIM_LENGTH 300
#define ROTATION_THRESHOLD 20
#define ROTATION_LOCKING_ANGLE 10
#define ROTATION_LOCKING_FACTOR 1.5

#define FIELD_OF_VIEW 22.5

MapCamera3d::MapCamera3d(const std::shared_ptr<MapInterface> &mapInterface, float screenDensityPpi)
    : mapInterface(mapInterface)
    , conversionHelper(mapInterface->getCoordinateConverterHelper())
    , mapCoordinateSystem(mapInterface->getMapConfig().mapCoordinateSystem)
    , screenDensityPpi(screenDensityPpi)
    , screenPixelAsRealMeterFactor(0.0254 / screenDensityPpi * mapCoordinateSystem.unitToScreenMeterFactor),
      focusPointPosition(CoordinateSystemIdentifiers::EPSG4326(), 0, 0, 0),
      focusPointAltitude(0),
      cameraDistance(10000),
      cameraPitch(0),
      lastOnTouchDownPoint(std::nullopt)
    , bounds(mapCoordinateSystem.bounds) {
    mapSystemRtl = mapCoordinateSystem.bounds.bottomRight.x > mapCoordinateSystem.bounds.topLeft.x;
    mapSystemTtb = mapCoordinateSystem.bounds.bottomRight.y > mapCoordinateSystem.bounds.topLeft.y;
    updateZoom(zoomMin);
}

void MapCamera3d::viewportSizeChanged() {
    Vec2I viewportSize = mapInterface->getRenderingContext()->getViewportSize();
    if (viewportSize.x > 0 && viewportSize.y > 0 && zoomMin < 0) {
        double boundsWidthM = std::abs(bounds.topLeft.x - bounds.bottomRight.x);
        double boundsHeightM = std::abs(bounds.topLeft.y - bounds.bottomRight.y);
        double widthDeviceM = screenPixelAsRealMeterFactor * viewportSize.x;
        double heightDeviceM = screenPixelAsRealMeterFactor * viewportSize.y;
        zoomMin = std::max(boundsHeightM / heightDeviceM, boundsWidthM / widthDeviceM);
        updateZoom(zoom);
    }

    notifyListeners(ListenerType::BOUNDS);
}

void MapCamera3d::freeze(bool freeze) {
    cameraFrozen = freeze;
    {
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        if (coordAnimation)
            coordAnimation->cancel();
        if (zoomAnimation)
            zoomAnimation->cancel();
        if (rotationAnimation)
            rotationAnimation->cancel();
    }
    inertia = std::nullopt;
}

void MapCamera3d::moveToCenterPositionZoom(const ::Coord &centerPosition, double zoom, bool animated) {
    if (cameraFrozen)
        return;
    inertia = std::nullopt;
    Coord focusPosition = mapInterface->getCoordinateConverterHelper()->convert(CoordinateSystemIdentifiers::EPSG4326(), centerPosition);

    if (animated) {
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        coordAnimation = std::make_shared<CoordAnimation>(
            DEFAULT_ANIM_LENGTH, focusPointPosition, focusPosition, centerPosition, InterpolatorFunction::EaseInOut,
            [=](Coord positionMapSystem) {
                this->focusPointPosition = positionMapSystem;
                notifyListeners(ListenerType::BOUNDS);
                mapInterface->invalidate();
            },
            [=] {
                this->focusPointPosition = this->coordAnimation->endValue;
                notifyListeners(ListenerType::BOUNDS);
                mapInterface->invalidate();
                this->coordAnimation = nullptr;
            });
        coordAnimation->start();
        setZoom(zoom, true);
        mapInterface->invalidate();
    } else {
        this->focusPointPosition = focusPosition;
        updateZoom(zoom);
        notifyListeners(ListenerType::BOUNDS);
        mapInterface->invalidate();
    }
}

void MapCamera3d::moveToCenterPosition(const ::Coord &centerPosition, bool animated) {
    if (cameraFrozen)
        return;
    inertia = std::nullopt;
    Coord focusPosition = mapInterface->getCoordinateConverterHelper()->convert(CoordinateSystemIdentifiers::EPSG4326(), centerPosition);

    if (animated) {
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        coordAnimation = std::make_shared<CoordAnimation>(
            DEFAULT_ANIM_LENGTH, focusPointPosition, focusPosition, centerPosition, InterpolatorFunction::EaseInOut,
            [=](Coord positionMapSystem) {
                this->focusPointPosition = positionMapSystem;
                notifyListeners(ListenerType::BOUNDS);
                mapInterface->invalidate();
            },
            [=] {
                this->focusPointPosition = this->coordAnimation->endValue;
                notifyListeners(ListenerType::BOUNDS);
                mapInterface->invalidate();
                this->coordAnimation = nullptr;
            });
        coordAnimation->start();
        mapInterface->invalidate();
    } else {
        this->focusPointPosition = focusPosition;
        notifyListeners(ListenerType::BOUNDS);
        mapInterface->invalidate();
    }
}

void MapCamera3d::moveToBoundingBox(const RectCoord &boundingBox, float paddingPc, bool animated, std::optional<double> minZoom, std::optional<double> maxZoom) {
    if (cameraFrozen)
        return;
    RectCoord mapSystemBBox = conversionHelper->convertRect(mapCoordinateSystem.identifier, boundingBox);
    float newLeft = mapSystemBBox.topLeft.x + paddingPc * (mapSystemBBox.topLeft.x - mapSystemBBox.bottomRight.x);
    float newRight = mapSystemBBox.bottomRight.x + paddingPc * (mapSystemBBox.bottomRight.x - mapSystemBBox.topLeft.x);
    float newTop = mapSystemBBox.topLeft.y + paddingPc * (mapSystemBBox.topLeft.y - mapSystemBBox.bottomRight.y);
    float newBottom = mapSystemBBox.bottomRight.y + paddingPc * (mapSystemBBox.bottomRight.y - mapSystemBBox.topLeft.y);
    Vec2F centerAABBox = Vec2F(newLeft + 0.5 * (newRight - newLeft), newTop + 0.5 * (newBottom - newTop));

    Coord targetCenterNotBC = Coord(mapCoordinateSystem.identifier, centerAABBox.x, centerAABBox.y, 0.0);

    Vec2F caTopLeft = Vec2FHelper::rotate(Vec2F(newLeft, newTop), centerAABBox, -angle);
    Vec2F caTopRight = Vec2FHelper::rotate(Vec2F(newRight, newTop), centerAABBox, -angle);
    Vec2F caBottomLeft = Vec2FHelper::rotate(Vec2F(newLeft, newBottom), centerAABBox, -angle);
    Vec2F caBottomRight = Vec2FHelper::rotate(Vec2F(newRight, newBottom), centerAABBox, -angle);

    float caMinX = std::min(std::min(std::min(caTopLeft.x, caTopRight.x), caBottomLeft.x), caBottomRight.x);
    float caMaxX = std::max(std::max(std::max(caTopLeft.x, caTopRight.x), caBottomLeft.x), caBottomRight.x);
    float caMinY = std::min(std::min(std::min(caTopLeft.y, caTopRight.y), caBottomLeft.y), caBottomRight.y);
    float caMaxY = std::max(std::max(std::max(caTopLeft.y, caTopRight.y), caBottomLeft.y), caBottomRight.y);

    float caXSpan = caMaxX - caMinX;
    float caYSpan = caMaxY - caMinY;

    Vec2I viewSize = mapInterface->getRenderingContext()->getViewportSize();

    double caZoomX = caXSpan / ((viewSize.x - paddingLeft - paddingRight) * screenPixelAsRealMeterFactor);
    double caZoomY = caYSpan / ((viewSize.y - paddingTop - paddingBottom) * screenPixelAsRealMeterFactor);
    double targetZoom = std::max(caZoomX, caZoomY);

    if (minZoom.has_value()) {
        targetZoom = std::max(targetZoom, *minZoom);
    }
    if (maxZoom.has_value()) {
        targetZoom = std::min(targetZoom, *maxZoom);
    }

    moveToCenterPositionZoom(targetCenterNotBC, targetZoom, animated);
}

::Coord MapCamera3d::getCenterPosition() {
    return focusPointPosition;
}

void MapCamera3d::setZoom(double zoom, bool animated) {
    if (cameraFrozen) {
        return;
    }

    double targetZoom = std::clamp(zoom, zoomMax, zoomMin);

    if (animated) {
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        zoomAnimation = std::make_shared<DoubleAnimation>(
            DEFAULT_ANIM_LENGTH, this->zoom, targetZoom, InterpolatorFunction::EaseIn,
            [=](double zoom) { this->setZoom(zoom, false); },
            [=] {
                this->setZoom(targetZoom, false);
                this->zoomAnimation = nullptr;
            });
        zoomAnimation->start();
        mapInterface->invalidate();
    } else {
        updateZoom(zoom);
        notifyListeners(ListenerType::BOUNDS);
        mapInterface->invalidate();
    }
}

double MapCamera3d::getZoom() { return zoom; }

void MapCamera3d::setRotation(float angle, bool animated) {
    if (cameraFrozen)
        return;
    double newAngle = (angle > 360 || angle < 0) ? fmod(angle + 360.0, 360.0) : angle;
    if (animated) {
        double currentAngle = fmod(this->angle, 360.0);
        if (abs(currentAngle - newAngle) > abs(currentAngle - (newAngle + 360.0))) {
            newAngle += 360.0;
        } else if (abs(currentAngle - newAngle) > abs(currentAngle - (newAngle - 360.0))) {
            newAngle -= 360.0;
        }
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        rotationAnimation = std::make_shared<DoubleAnimation>(
            DEFAULT_ANIM_LENGTH, currentAngle, newAngle, InterpolatorFunction::Linear,
            [=](double angle) { this->setRotation(angle, false); },
            [=] {
                this->setRotation(newAngle, false);
                this->rotationAnimation = nullptr;
            });
        rotationAnimation->start();
        mapInterface->invalidate();
    } else {
        double angleDiff = newAngle - this->angle;
        Coord centerScreen = focusPointPosition;
        Coord realCenter = getCenterPosition();
        Vec2D rotatedDiff =
            Vec2DHelper::rotate(Vec2D(centerScreen.x - realCenter.x, centerScreen.y - realCenter.y), Vec2D(0.0, 0.0), angleDiff);
        focusPointPosition.x = realCenter.x + rotatedDiff.x;
        focusPointPosition.y = realCenter.y + rotatedDiff.y;

        this->angle = newAngle;
        notifyListeners(ListenerType::ROTATION | ListenerType::BOUNDS);
        mapInterface->invalidate();
    }
}

float MapCamera3d::getRotation() { return angle; }

void MapCamera3d::setPaddingLeft(float padding) {
    paddingLeft = padding;
    std::lock_guard<std::recursive_mutex> lock(animationMutex);
    if (coordAnimation && coordAnimation->helperCoord.has_value()) {
        double targetZoom = (zoomAnimation) ? zoomAnimation->endValue : getZoom();
        const auto [adjPosition, adjZoom] =
                getBoundsCorrectedCoords(adjustCoordForPadding(*coordAnimation->helperCoord, targetZoom), targetZoom);
        coordAnimation->endValue = adjPosition;
        if (zoomAnimation) {
            zoomAnimation->endValue = adjZoom;
        }
    } else {
        clampCenterToPaddingCorrectedBounds();
    }
}

void MapCamera3d::setPaddingRight(float padding) {
    paddingRight = padding;
    std::lock_guard<std::recursive_mutex> lock(animationMutex);
    if (coordAnimation && coordAnimation->helperCoord.has_value()) {
        double targetZoom = (zoomAnimation) ? zoomAnimation->endValue : getZoom();
        const auto [adjPosition, adjZoom] =
                getBoundsCorrectedCoords(adjustCoordForPadding(*coordAnimation->helperCoord, targetZoom), targetZoom);
        coordAnimation->endValue = adjPosition;
        if (zoomAnimation) {
            zoomAnimation->endValue = adjZoom;
        }
    } else {
        clampCenterToPaddingCorrectedBounds();
    }
}

void MapCamera3d::setPaddingTop(float padding) {
    paddingTop = padding;
    std::lock_guard<std::recursive_mutex> lock(animationMutex);
    if (coordAnimation && coordAnimation->helperCoord.has_value()) {
        double targetZoom = (zoomAnimation) ? zoomAnimation->endValue : getZoom();
        const auto [adjPosition, adjZoom] =
                getBoundsCorrectedCoords(adjustCoordForPadding(*coordAnimation->helperCoord, targetZoom), targetZoom);
        coordAnimation->endValue = adjPosition;
        if (zoomAnimation) {
            zoomAnimation->endValue = adjZoom;
        }
    } else {
        clampCenterToPaddingCorrectedBounds();
    }
}

void MapCamera3d::setPaddingBottom(float padding) {
    paddingBottom = padding;
    std::lock_guard<std::recursive_mutex> lock(animationMutex);
    if (coordAnimation && coordAnimation->helperCoord.has_value()) {
        double targetZoom = (zoomAnimation) ? zoomAnimation->endValue : getZoom();
        const auto [adjPosition, adjZoom] =
                getBoundsCorrectedCoords(adjustCoordForPadding(*coordAnimation->helperCoord, targetZoom), targetZoom);
        coordAnimation->endValue = adjPosition;
        if (zoomAnimation) {
            zoomAnimation->endValue = adjZoom;
        }
    } else {
        clampCenterToPaddingCorrectedBounds();
    }
}

void MapCamera3d::addListener(const std::shared_ptr<MapCameraListenerInterface> &listener) {
    std::lock_guard<std::recursive_mutex> lock(listenerMutex);
    if (listeners.count(listener) == 0) {
        listeners.insert(listener);
    }
}

void MapCamera3d::removeListener(const std::shared_ptr<MapCameraListenerInterface> &listener) {
    std::lock_guard<std::recursive_mutex> lock(listenerMutex);
    if (listeners.count(listener) > 0) {
        listeners.erase(listener);
    }
}

std::shared_ptr<::CameraInterface> MapCamera3d::asCameraInterface() { return shared_from_this(); }

std::vector<float> MapCamera3d::getVpMatrix() {
    {
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        if (zoomAnimation)
            std::static_pointer_cast<AnimationInterface>(zoomAnimation)->update();
        if (rotationAnimation)
            std::static_pointer_cast<AnimationInterface>(rotationAnimation)->update();
        if (coordAnimation)
            std::static_pointer_cast<AnimationInterface>(coordAnimation)->update();
    }

    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
    double currentRotation = angle;
    double currentZoom = zoom;
    RectCoord viewBounds = getRectFromViewport(sizeViewport, focusPointPosition);

    std::vector<float> newViewMatrix(16, 0.0);
    std::vector<float> newProjectionMatrix(16, 0.0);

    float R = 6378137.0;
    float longitude = focusPointPosition.x; //  px / R;
    float latitude = focusPointPosition.y; // 2*atan(exp(py / R)) - 3.1415926 / 2;

    focusPointAltitude = focusPointPosition.z;
    cameraDistance = getCameraDistance();
    cameraPitch = getCameraPitch();
    cameraVerticalDisplacement = getCameraVerticalDisplacement();
    float maxD = cameraDistance / R + 1;
    float minD = std::max(cameraDistance / R - 1, 0.00001);

    float fovy = FIELD_OF_VIEW; // 45 // zoom / 70800;

    // aspect ratio
    float vpr = (float) sizeViewport.x / (float) sizeViewport.y;
    if (vpr > 1.0) {
        fovy /= vpr;
    }
    float fovyRad = fovy * M_PI / 180.0;

    Matrix::perspectiveM(newProjectionMatrix, 0, fovy, vpr, minD, maxD);
    float contentHeight = ((double) sizeViewport.y) - paddingBottom - paddingTop;
    float offsetY = -paddingBottom / 2.0 / (double) sizeViewport.y + cameraVerticalDisplacement * contentHeight * 0.5 / (double) sizeViewport.y;
    offsetY = cameraDistance / R * tan(fovyRad / 2.0) * offsetY; // view space to world space
    Matrix::translateM(newProjectionMatrix, 0, 0.0, -offsetY, 0);

    Matrix::setIdentityM(newViewMatrix, 0);


    Matrix::translateM(newViewMatrix, 0, 0.0, 0, -cameraDistance / R);
    Matrix::rotateM(newViewMatrix, 0, -cameraPitch, 1.0, 0.0, 0.0);
    Matrix::rotateM(newViewMatrix, 0, -angle, 0.0, 0.0, 1.0);

    Matrix::translateM(newViewMatrix, 0, 0, 0, -1 - focusPointAltitude / R);

    Matrix::rotateM(newViewMatrix, 0.0, latitude, 1.0, 0.0, 0.0);
    Matrix::rotateM(newViewMatrix, 0.0, -longitude, 0.0, 1.0, 0.0);
    Matrix::rotateM(newViewMatrix, 0.0, -90, 0.0, 1.0, 0.0); // zero longitude in London

    std::vector<float> newVpMatrix(16, 0.0);
    Matrix::multiplyMM(newVpMatrix, 0, newProjectionMatrix, 0, newViewMatrix, 0);

    std::lock_guard<std::recursive_mutex> lock(vpDataMutex);
    lastVpBounds = viewBounds;
    lastVpRotation = currentRotation;
    lastVpZoom = currentZoom;
    vpMatrix = newVpMatrix;
    viewMatrix = newViewMatrix;
    projectionMatrix = newProjectionMatrix;
    verticalFov = fovy;
    horizontalFov = fovy * vpr;
    validVpMatrix = true;

    return newVpMatrix;
}

std::optional<std::vector<float>> MapCamera3d::getLastVpMatrix() {
    if (!lastVpBounds) {
        return std::nullopt;
    }
    std::vector<float> vpCopy;
    std::copy(vpMatrix.begin(), vpMatrix.end(), std::back_inserter(vpCopy));
    return vpCopy;
}

std::optional<::RectCoord> MapCamera3d::getLastVpMatrixViewBounds() {
    std::lock_guard<std::recursive_mutex> lock(vpDataMutex);
    return lastVpBounds;
}

std::optional<float> MapCamera3d::getLastVpMatrixRotation() {
    std::lock_guard<std::recursive_mutex> lock(vpDataMutex);
    return lastVpRotation;
}

std::optional<float> MapCamera3d::getLastVpMatrixZoom() {
    std::lock_guard<std::recursive_mutex> lock(vpDataMutex);
    return lastVpZoom;
}

/** this method is called just before the update methods on all layers */
void MapCamera3d::update() { inertiaStep(); }

std::vector<float> MapCamera3d::getInvariantModelMatrix(const ::Coord &coordinate, bool scaleInvariant, bool rotationInvariant) {
    Coord renderCoord = conversionHelper->convertToRenderSystem(coordinate);
    std::vector<float> newMatrix(16, 0);

    Matrix::setIdentityM(newMatrix, 0);
    Matrix::translateM(newMatrix, 0, renderCoord.x, renderCoord.y, renderCoord.z);

    if (scaleInvariant) {
        double zoomFactor = mapUnitsFromPixels(1.0);
        Matrix::scaleM(newMatrix, 0.0, zoomFactor, zoomFactor, 1.0);
    }

    if (rotationInvariant) {
        Matrix::rotateM(newMatrix, 0.0, -angle, 0.0, 0.0, 1.0);
    }

    Matrix::translateM(newMatrix, 0, -renderCoord.x, -renderCoord.y, -renderCoord.z);

    return newMatrix;
}

RectCoord MapCamera3d::getVisibleRect() {
    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
    return getRectFromViewport(sizeViewport, focusPointPosition);
}

RectCoord MapCamera3d::getPaddingAdjustedVisibleRect() {
    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();

    // adjust viewport
    sizeViewport.x -= (paddingLeft + paddingRight);
    sizeViewport.y -= (paddingTop + paddingBottom);

    // also use the padding adjusted center position
    return getRectFromViewport(sizeViewport, getCenterPosition());
}

RectCoord MapCamera3d::getRectFromViewport(const Vec2I &sizeViewport, const Coord &center) {
    double zoomFactor = screenPixelAsRealMeterFactor * zoom;

    double halfWidth = sizeViewport.x * 0.5 * zoomFactor;
    double halfHeight = sizeViewport.y * 0.5 * zoomFactor;

    double sinAngle = sin(angle * M_PI / 180.0);
    double cosAngle = cos(angle * M_PI / 180.0);

    double deltaX = std::abs(halfWidth * cosAngle) + std::abs(halfHeight * sinAngle);
    double deltaY = std::abs(halfWidth * sinAngle) + std::abs(halfHeight * cosAngle);

    double topLeftX = center.x - deltaX;
    double topLeftY = center.y + deltaY;
    double bottomRightX = center.x + deltaX;
    double bottomRightY = center.y - deltaY;

    Coord topLeft = Coord(mapCoordinateSystem.identifier, topLeftX, topLeftY, center.z);
    Coord bottomRight = Coord(mapCoordinateSystem.identifier, bottomRightX, bottomRightY, center.z);
    return RectCoord(topLeft, bottomRight);
}

void MapCamera3d::notifyListeners(const int &listenerType) {
    std::optional<RectCoord> visibleRect =
        (listenerType & ListenerType::BOUNDS) ? std::optional<RectCoord>(getVisibleRect()) : std::nullopt;

    double angle = this->angle;
    double zoom = this->zoom;

    std::vector<float> viewMatrix;
    std::vector<float> projectionMatrix;
    float width = 0.0;
    float height = 0.0;
    float horizontalFov = 0.0;
    float verticalFov = 0.0;
    float focusPointAltitude = 0.0;
    auto focusPointPosition = Coord(0, 0.0, 0.0, 0.0);

    if (listenerType & ListenerType::BOUNDS) {
        bool validVpMatrix = false;
        {
            std::lock_guard<std::recursive_mutex> lock(vpDataMutex);
            validVpMatrix = this->validVpMatrix;
        }
        if (!validVpMatrix) {
            getVpMatrix(); // update matrices
        }
        {
            std::lock_guard<std::recursive_mutex> lock(vpDataMutex);

            viewMatrix = this->viewMatrix;
            projectionMatrix = this->projectionMatrix;
            horizontalFov = this->horizontalFov;
            verticalFov = this->verticalFov;
            focusPointAltitude = this->focusPointAltitude;
            focusPointPosition = this->focusPointPosition;
        }

        Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
        width = sizeViewport.x;
        height = sizeViewport.y;
    }

    std::lock_guard<std::recursive_mutex> lock(listenerMutex);
    for (auto listener : listeners) {
        if (listenerType & ListenerType::BOUNDS) {
            listener->onCameraChange(viewMatrix, projectionMatrix, verticalFov, horizontalFov, width, height, focusPointAltitude, getCenterPosition());
        }
        if (listenerType & ListenerType::ROTATION) {
            listener->onRotationChanged(angle);
        }
        if (listenerType & ListenerType::MAP_INTERACTION) {
            listener->onMapInteraction();
        }
    }
}

bool MapCamera3d::onTouchDown(const ::Vec2F &posScreen) {
    inertia = std::nullopt;
    auto pos = coordFromScreenPosition(posScreen);
    if (pos.systemIdentifier == -1) {
        lastOnTouchDownPoint = std::nullopt;
        lastOnTouchDownCoord = std::nullopt;
        return false;
    }
    else {
        lastOnTouchDownPoint = posScreen;
        initialTouchDownPoint = posScreen;
        lastOnTouchDownCoord = pos;
        return true;
    }
}

bool MapCamera3d::onMove(const Vec2F &deltaScreen, bool confirmed, bool doubleClick) {
    if (!config.moveEnabled || cameraFrozen || (lastOnTouchDownPoint == std::nullopt) || (lastOnTouchDownCoord == std::nullopt)) {
        return false;
    }

    inertia = std::nullopt;

    if (doubleClick) {
        double newZoom = zoom * (1.0 - (deltaScreen.y * 0.003));
        updateZoom(newZoom);

        if (initialTouchDownPoint) {
            // Force update of matrices for coordFromScreenPosition-call, ...
            getVpMatrix();

            // ..., then find coordinate, that would be below middle-point
            auto newTouchDownCoord = coordFromScreenPosition(initialTouchDownPoint.value());

            // Rotate globe to keep initial coordinate at middle-point
            if (lastOnTouchDownCoord && lastOnTouchDownCoord->systemIdentifier != -1 && newTouchDownCoord.systemIdentifier != -1) {
                double dx = -(newTouchDownCoord.x - lastOnTouchDownCoord->x);
                double dy = -(newTouchDownCoord.y - lastOnTouchDownCoord->y);

                focusPointPosition.x = focusPointPosition.x + dx;
                focusPointPosition.y = focusPointPosition.y + dy;

                focusPointPosition.x = std::fmod((focusPointPosition.x + 180 + 360), 360.0) - 180;
                focusPointPosition.y = std::clamp(focusPointPosition.y, -90.0, 90.0);

            }
        }

        clampCenterToPaddingCorrectedBounds();

        notifyListeners(ListenerType::BOUNDS | ListenerType::MAP_INTERACTION);
        mapInterface->invalidate();
        return true;
    }

    Vec2F newScreenPos = lastOnTouchDownPoint.value() + deltaScreen;
    lastOnTouchDownPoint = newScreenPos;

    auto newTouchDownCoord = coordFromScreenPosition(newScreenPos);

    if (newTouchDownCoord.systemIdentifier == -1) {
        return false;
    }

    double dx = -(newTouchDownCoord.x - lastOnTouchDownCoord->x);
    double dy = -(newTouchDownCoord.y - lastOnTouchDownCoord->y);

    focusPointPosition.x = focusPointPosition.x + dx;
    focusPointPosition.y = focusPointPosition.y + dy;

    focusPointPosition.x = std::fmod((focusPointPosition.x + 180 + 360), 360.0) - 180;
    focusPointPosition.y = std::clamp(focusPointPosition.y, -90.0, 90.0);

    clampCenterToPaddingCorrectedBounds();

    if (currentDragTimestamp == 0) {
        currentDragTimestamp = DateHelper::currentTimeMicros();
        currentDragVelocity.x = 0;
        currentDragVelocity.y = 0;
    } else {
        long long newTimestamp = DateHelper::currentTimeMicros();
        long long deltaMcs = std::max(newTimestamp - currentDragTimestamp, 8000ll);
        float averageFactor = currentDragVelocity.x == 0 && currentDragVelocity.y == 0 ? 1.0 : 0.5;
        currentDragVelocity.x = (1 - averageFactor) * currentDragVelocity.x + averageFactor * dx / (deltaMcs / 16000.0);
        currentDragVelocity.y = (1 - averageFactor) * currentDragVelocity.y + averageFactor * dy / (deltaMcs / 16000.0);
        currentDragTimestamp = newTimestamp;
    }

    notifyListeners(ListenerType::BOUNDS | ListenerType::MAP_INTERACTION);
    mapInterface->invalidate();

    return true;
}

bool MapCamera3d::onMoveComplete() {
    if (cameraFrozen)
        return false;
    setupInertia();
    return true;
}

void MapCamera3d::setupInertia() {
    float vel = std::sqrt(currentDragVelocity.x * currentDragVelocity.x + currentDragVelocity.y * currentDragVelocity.y);
    double t1 = vel >= 0.4 ? 30.0 : 0.0;
    double t2 = vel >= 0.01 ? 200.0 : 0.0;
    inertia = Inertia(DateHelper::currentTimeMicros(), currentDragVelocity, t1, t2);
    currentDragVelocity = {0, 0};
    currentDragTimestamp = 0;
}

void MapCamera3d::inertiaStep() {
    if (inertia == std::nullopt) {
        return;
    }

    long long now = DateHelper::currentTimeMicros();
    double delta = (now - inertia->timestampStart) / 16000.0;
    double deltaPrev = (now - inertia->timestampUpdate) / 16000.0;

    if (delta >= inertia->t1 + inertia->t2) {
        inertia = std::nullopt;
        return;
    }

    float factor = std::pow(0.9, delta);
    if (delta > inertia->t1) {
        factor *= std::pow(0.6, delta - inertia->t1);
    }

    float xDiffMap = (inertia->velocity.x) * factor * deltaPrev;
    float yDiffMap = (inertia->velocity.y) * factor * deltaPrev;
    inertia->timestampUpdate = now;

    const auto [adjustedPosition, adjustedZoom] = getBoundsCorrectedCoords(
            Coord(focusPointPosition.systemIdentifier,
                  focusPointPosition.x + xDiffMap,
                  focusPointPosition.y + yDiffMap,
                  focusPointPosition.z), zoom);


    focusPointPosition.x = std::fmod((adjustedPosition.x + 180 + 360), 360.0) - 180;
    focusPointPosition.y = std::clamp(adjustedPosition.y, -90.0, 90.0);

    zoom = adjustedZoom;

    notifyListeners(ListenerType::BOUNDS);
    mapInterface->invalidate();
}

bool MapCamera3d::onDoubleClick(const ::Vec2F &posScreen) {
    if (!config.doubleClickZoomEnabled || cameraFrozen) {
        return false;
    }

    inertia = std::nullopt;

    auto targetZoom = zoom / 2;
    targetZoom = std::max(std::min(targetZoom, zoomMin), zoomMax);

    // Get initial coordinate at touch
    auto centerCoordBefore = coordFromScreenPosition(posScreen);

    // Force update of matrices with new zoom for coordFromScreenPosition-call, ...
    auto originalZoom = zoom;
    setZoom(targetZoom, false);
    getVpMatrix();

    // ..., then find coordinate, that would be at touch
    auto centerCoordAfter = coordFromScreenPosition(posScreen);

    // Reset zoom before animation
    setZoom(originalZoom, false);
    getVpMatrix();

    // Rotate globe to keep initial coordinate at touch
    if (centerCoordBefore.systemIdentifier != -1 && centerCoordAfter.systemIdentifier != -1) {
        double dx = (centerCoordBefore.x - centerCoordAfter.x);
        double dy = (centerCoordBefore.y - centerCoordAfter.y);

        auto position = focusPointPosition;
        position.x += dx;
        position.y += dy;

        position.x = std::fmod((position.x + 180 + 360), 360.0) - 180;
        position.y = std::clamp(position.y, -90.0, 90.0);

        moveToCenterPositionZoom(position, targetZoom, true);

    }
    else {
        setZoom(targetZoom, true);
    }

    notifyListeners(ListenerType::MAP_INTERACTION);
    return true;
}

bool MapCamera3d::onTwoFingerClick(const ::Vec2F &posScreen1, const ::Vec2F &posScreen2) {
    if (!config.doubleClickZoomEnabled || cameraFrozen)
        return false;

    inertia = std::nullopt;

    auto targetZoom = zoom * 2;
    targetZoom = std::max(std::min(targetZoom, zoomMin), zoomMax);

    // Get initial coordinate below middle-point of fingers
    auto posScreen = (posScreen1 + posScreen2) / 2.0;
    auto centerCoordBefore = coordFromScreenPosition(posScreen);

    // Force update of matrices with new zoom for coordFromScreenPosition-call, ...
    auto originalZoom = zoom;
    setZoom(targetZoom, false);
    getVpMatrix();

    // ..., then find coordinate, that would be below middle-point
    auto centerCoordAfter = coordFromScreenPosition(posScreen);

    // Reset zoom before animation
    setZoom(originalZoom, false);
    getVpMatrix();

    // Rotate globe to keep initial coordinate at middle-point
    if (centerCoordBefore.systemIdentifier != -1 && centerCoordAfter.systemIdentifier != -1) {
        double dx = (centerCoordBefore.x - centerCoordAfter.x);
        double dy = (centerCoordBefore.y - centerCoordAfter.y);

        auto position = focusPointPosition;
        position.x += dx;
        position.y += dy;

        position.x = std::fmod((position.x + 180 + 360), 360.0) - 180;
        position.y = std::clamp(position.y, -90.0, 90.0);

        moveToCenterPositionZoom(position, targetZoom, true);

    }
    else {
        setZoom(targetZoom, true);
    }


    notifyListeners(ListenerType::MAP_INTERACTION);
    return true;
}

void MapCamera3d::clearTouch() {
    isRotationThresholdReached = false;
    rotationPossible = true;
    tempAngle = angle;
    startZoom = 0;
    lastOnTouchDownPoint = std::nullopt;
    lastOnTouchDownCoord = std::nullopt;
}

bool MapCamera3d::onTwoFingerMove(const std::vector<::Vec2F> &posScreenOld, const std::vector<::Vec2F> &posScreenNew) {
    if (!config.twoFingerZoomEnabled || cameraFrozen)
        return false;

    inertia = std::nullopt;


    if (posScreenOld.size() >= 2) {

        double scaleFactor =
            Vec2FHelper::distance(posScreenNew[0], posScreenNew[1]) / Vec2FHelper::distance(posScreenOld[0], posScreenOld[1]);
        double newZoom = std::clamp(zoom / scaleFactor, zoomMax, zoomMin);

        if (startZoom == 0) {
            startZoom = zoom;

            // Compute initial middle-point
            // Coordinate at middle-point of fingers should stay at middle-point during zoom gesture
            auto screenCenterOld = (posScreenOld[0] + posScreenOld[1]) / 2.0;
            lastOnTouchDownCoord = coordFromScreenPosition(screenCenterOld);
        }
        if (newZoom > startZoom * ROTATION_LOCKING_FACTOR || newZoom < startZoom / ROTATION_LOCKING_FACTOR) {
            rotationPossible = false;
        }

        // Set new zoom
        updateZoom(newZoom);

        // Force update of matrices for coordFromScreenPosition-call, ...
        getVpMatrix();

        // ..., then find coordinate, that would be below middle-point
        auto screenCenterNew = (posScreenNew[0] + posScreenNew[1]) / 2.0;
        auto newTouchDownCoord = coordFromScreenPosition(screenCenterNew);

        // Rotate globe to keep initial coordinate at middle-point
        if (lastOnTouchDownCoord && lastOnTouchDownCoord->systemIdentifier != -1 && newTouchDownCoord.systemIdentifier != -1) {
            double dx = -(newTouchDownCoord.x - lastOnTouchDownCoord->x);
            double dy = -(newTouchDownCoord.y - lastOnTouchDownCoord->y);

            focusPointPosition.x = focusPointPosition.x + dx;
            focusPointPosition.y = focusPointPosition.y + dy;

            focusPointPosition.x = std::fmod((focusPointPosition.x + 180 + 360), 360.0) - 180;
            focusPointPosition.y = std::clamp(focusPointPosition.y, -90.0, 90.0);

            if (currentDragTimestamp == 0) {
                currentDragTimestamp = DateHelper::currentTimeMicros();
                currentDragVelocity.x = 0;
                currentDragVelocity.y = 0;
            } else {
                long long newTimestamp = DateHelper::currentTimeMicros();
                long long deltaMcs = std::max(newTimestamp - currentDragTimestamp, 8000ll);
                float averageFactor = currentDragVelocity.x == 0 && currentDragVelocity.y == 0 ? 1.0 : 0.5;
                currentDragVelocity.x = (1 - averageFactor) * currentDragVelocity.x + averageFactor * dx / (deltaMcs / 16000.0);
                currentDragVelocity.y = (1 - averageFactor) * currentDragVelocity.y + averageFactor * dy / (deltaMcs / 16000.0);
                currentDragTimestamp = newTimestamp;
            }
        }


        const auto [adjPosition, adjZoom] = getBoundsCorrectedCoords(focusPointPosition, newZoom);
        focusPointPosition = adjPosition;
        zoom = adjZoom;

        auto listenerType = ListenerType::BOUNDS | ListenerType::MAP_INTERACTION;
        notifyListeners(listenerType);
        mapInterface->invalidate();
    }
    return true;
}

bool MapCamera3d::onTwoFingerMoveComplete() {
    if (config.snapToNorthEnabled && !cameraFrozen && (angle < ROTATION_LOCKING_ANGLE || angle > (360 - ROTATION_LOCKING_ANGLE))) {
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        rotationAnimation = std::make_shared<DoubleAnimation>(
            DEFAULT_ANIM_LENGTH, this->angle, angle < ROTATION_LOCKING_ANGLE ? 0 : 360, InterpolatorFunction::EaseInOut,
            [=](double angle) {
                this->angle = angle;
                mapInterface->invalidate();
                notifyListeners(ListenerType::ROTATION | ListenerType::BOUNDS);
            },
            [=] {
                this->angle = 0;
                this->rotationAnimation = nullptr;
                mapInterface->invalidate();
                notifyListeners(ListenerType::ROTATION | ListenerType::BOUNDS);
            });
        rotationAnimation->start();
        mapInterface->invalidate();
        return true;
    }

    return false;
}

Coord MapCamera3d::coordFromScreenPosition(const ::Vec2F &posScreen) {
    auto viewport = mapInterface->getRenderingContext()->getViewportSize();

    std::vector<double> vpMatrixD = {
        static_cast<double>(vpMatrix[0]),
        static_cast<double>(vpMatrix[1]),
        static_cast<double>(vpMatrix[2]),
        static_cast<double>(vpMatrix[3]),
        static_cast<double>(vpMatrix[4]),
        static_cast<double>(vpMatrix[5]),
        static_cast<double>(vpMatrix[6]),
        static_cast<double>(vpMatrix[7]),
        static_cast<double>(vpMatrix[8]),
        static_cast<double>(vpMatrix[9]),
        static_cast<double>(vpMatrix[10]),
        static_cast<double>(vpMatrix[11]),
        static_cast<double>(vpMatrix[12]),
        static_cast<double>(vpMatrix[13]),
        static_cast<double>(vpMatrix[14]),
        static_cast<double>(vpMatrix[15])
    };
    std::vector<double> inverseMatrix(16, 0.0);
    if(!gluInvertMatrix(vpMatrixD, inverseMatrix)) {
        return Coord(-1, 0, 0, 0);
    }

    std::vector<double> normalizedPosScreenFront = {
        (posScreen.x / (double)viewport.x * 2.0 - 1),
        -(posScreen.y / (double)viewport.y * 2.0 - 1),
        -1,
        1
    };
    std::vector<double> normalizedPosScreenBack = {
        (posScreen.x / (double)viewport.x * 2.0 - 1),
        -(posScreen.y / (double)viewport.y * 2.0 - 1),
        1,
        1
    };


    auto worldPosFrontVec = MatrixD::multiply(inverseMatrix, normalizedPosScreenFront);
    Vec3D worldPosFront(worldPosFrontVec[0] / worldPosFrontVec[3], worldPosFrontVec[1] / worldPosFrontVec[3], worldPosFrontVec[2] / worldPosFrontVec[3]);
    auto worldPosBackVec = MatrixD::multiply(inverseMatrix, normalizedPosScreenBack);
    Vec3D worldPosBack(worldPosBackVec[0] / worldPosBackVec[3], worldPosBackVec[1] / worldPosBackVec[3], worldPosBackVec[2] / worldPosBackVec[3]);

    bool didHit = false;
    auto point = MapCamera3DHelper::raySphereIntersection(worldPosFront, worldPosBack, Vec3D(0.0, 0.0, 0.0), 1.0, didHit);

    if (didHit) {
        float longitude = std::atan2(point.x, point.z) * 180 / M_PI - 90;
        float latitude = std::asin(point.y) * 180 / M_PI;
        return Coord(CoordinateSystemIdentifiers::EPSG4326(), longitude, latitude, 0);
    }
    else {
        return Coord(-1, 0, 0, 0);
    }
}

bool MapCamera3d::gluInvertMatrix(const std::vector<double> &m, std::vector<double> &invOut)
{

    if (m.size() != 16 || invOut.size() != 16) {
        return false;
    }

    double inv[16], det;
    int i;

    inv[0] = m[5]  * m[10] * m[15] -
    m[5]  * m[11] * m[14] -
    m[9]  * m[6]  * m[15] +
    m[9]  * m[7]  * m[14] +
    m[13] * m[6]  * m[11] -
    m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] +
    m[4]  * m[11] * m[14] +
    m[8]  * m[6]  * m[15] -
    m[8]  * m[7]  * m[14] -
    m[12] * m[6]  * m[11] +
    m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] -
    m[4]  * m[11] * m[13] -
    m[8]  * m[5] * m[15] +
    m[8]  * m[7] * m[13] +
    m[12] * m[5] * m[11] -
    m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] +
    m[4]  * m[10] * m[13] +
    m[8]  * m[5] * m[14] -
    m[8]  * m[6] * m[13] -
    m[12] * m[5] * m[10] +
    m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] +
    m[1]  * m[11] * m[14] +
    m[9]  * m[2] * m[15] -
    m[9]  * m[3] * m[14] -
    m[13] * m[2] * m[11] +
    m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] -
    m[0]  * m[11] * m[14] -
    m[8]  * m[2] * m[15] +
    m[8]  * m[3] * m[14] +
    m[12] * m[2] * m[11] -
    m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] +
    m[0]  * m[11] * m[13] +
    m[8]  * m[1] * m[15] -
    m[8]  * m[3] * m[13] -
    m[12] * m[1] * m[11] +
    m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] -
    m[0]  * m[10] * m[13] -
    m[8]  * m[1] * m[14] +
    m[8]  * m[2] * m[13] +
    m[12] * m[1] * m[10] -
    m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] -
    m[1]  * m[7] * m[14] -
    m[5]  * m[2] * m[15] +
    m[5]  * m[3] * m[14] +
    m[13] * m[2] * m[7] -
    m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] +
    m[0]  * m[7] * m[14] +
    m[4]  * m[2] * m[15] -
    m[4]  * m[3] * m[14] -
    m[12] * m[2] * m[7] +
    m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] -
    m[0]  * m[7] * m[13] -
    m[4]  * m[1] * m[15] +
    m[4]  * m[3] * m[13] +
    m[12] * m[1] * m[7] -
    m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] +
    m[0]  * m[6] * m[13] +
    m[4]  * m[1] * m[14] -
    m[4]  * m[2] * m[13] -
    m[12] * m[1] * m[6] +
    m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] +
    m[1] * m[7] * m[10] +
    m[5] * m[2] * m[11] -
    m[5] * m[3] * m[10] -
    m[9] * m[2] * m[7] +
    m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] -
    m[0] * m[7] * m[10] -
    m[4] * m[2] * m[11] +
    m[4] * m[3] * m[10] +
    m[8] * m[2] * m[7] -
    m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] +
    m[0] * m[7] * m[9] +
    m[4] * m[1] * m[11] -
    m[4] * m[3] * m[9] -
    m[8] * m[1] * m[7] +
    m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] -
    m[0] * m[6] * m[9] -
    m[4] * m[1] * m[10] +
    m[4] * m[2] * m[9] +
    m[8] * m[1] * m[6] -
    m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det == 0)
        return false;

    det = 1.0 / det;

    for (i = 0; i < 16; i++)
        invOut[i] = inv[i] * det;

    return true;
}

::Vec2F MapCamera3d::screenPosFromCoord(const Coord &coord) {
    const auto mapInterface = this->mapInterface;
    const auto conversionHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    const auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!conversionHelper || !renderingContext) {
        return Vec2F(0.0, 0.0);
    }

    const auto mapCoord = conversionHelper->convert(mapCoordinateSystem.identifier, coord);

    double angRad = -angle * M_PI / 180.0;
    double sinAng = std::sin(angRad);
    double cosAng = std::cos(angRad);

    double coordXDiffToCenter = mapCoord.x - focusPointPosition.x;
    double coordYDiffToCenter = mapCoord.y - focusPointPosition.y;

    double screenXDiffToCenter = coordXDiffToCenter * cosAng - coordYDiffToCenter * sinAng;
    double screenYDiffToCenter = coordXDiffToCenter * sinAng + coordYDiffToCenter * cosAng;

    const Vec2I sizeViewport = renderingContext->getViewportSize();

    double zoomFactor = screenPixelAsRealMeterFactor * zoom;
    double posScreenX = (screenXDiffToCenter / zoomFactor) + ((double)sizeViewport.x / 2.0);
    double posScreenY = ((double)sizeViewport.y / 2.0) - (screenYDiffToCenter / zoomFactor);

    // TODO: Ensure screenPosFromCoord is correct
    printf("Warning: screenPosFromCoord incomplete logic.\n");

    return Vec2F(posScreenX, posScreenY);
}

double MapCamera3d::mapUnitsFromPixels(double distancePx) {
    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
    if (validVpMatrix && sizeViewport.x != 0 && sizeViewport.y != 0) {
        Coord focusRenderCoord = conversionHelper->convertToRenderSystem(getCenterPosition());

        float sampleSize = M_PI / 180.0;
        std::vector<float> posOne = {(float) (focusRenderCoord.z * sin(focusRenderCoord.y) * cos(focusRenderCoord.x)),
            (float) (focusRenderCoord.z * cos(focusRenderCoord.y)),
            (float) (-focusRenderCoord.z * sin(focusRenderCoord.y) * sin(focusRenderCoord.x)),
            1.0};
        std::vector<float> posTwo = {(float) (focusRenderCoord.z * sin(focusRenderCoord.y + sampleSize) * cos(focusRenderCoord.x + sampleSize)),
            (float) (focusRenderCoord.z * cos(focusRenderCoord.y + sampleSize)),
            (float) (-focusRenderCoord.z * sin(focusRenderCoord.y + sampleSize) * sin(focusRenderCoord.x + sampleSize)),
            1.0};
        auto projectedOne = Matrix::multiply(vpMatrix, posOne);
        auto projectedTwo = Matrix::multiply(vpMatrix, posTwo);
        projectedOne[0] /= projectedOne[3];
        projectedOne[1] /= projectedOne[3];
        projectedOne[2] /= projectedOne[3];
        projectedOne[3] /= projectedOne[3];
        projectedTwo[0] /= projectedTwo[3];
        projectedTwo[1] /= projectedTwo[3];
        projectedTwo[2] /= projectedTwo[3];
        projectedTwo[3] /= projectedTwo[3];
        float projectedLength = Matrix::length((projectedTwo[0] - projectedOne[0]) * sizeViewport.x,
                                               (projectedTwo[1] - projectedOne[1]) * sizeViewport.y,
                                               0.0);
        return distancePx * 2.0 * sqrt(sampleSize * sampleSize * 2) / projectedLength;
    }
    return distancePx * screenPixelAsRealMeterFactor * zoom;
}

double MapCamera3d::getScalingFactor() { return mapUnitsFromPixels(1.0); }

void MapCamera3d::setMinZoom(double zoomMin) {
    this->zoomMin = zoomMin;
    if (zoom > zoomMin) {
        updateZoom(zoomMin);
    }
    mapInterface->invalidate();
}

void MapCamera3d::setMaxZoom(double zoomMax) {
    this->zoomMax = zoomMax;
    if (zoom < zoomMax) {
        updateZoom(zoomMax);
    }
    mapInterface->invalidate();
}

double MapCamera3d::getMinZoom() { return zoomMin; }

double MapCamera3d::getMaxZoom() { return zoomMax; }

void MapCamera3d::setBounds(const RectCoord &bounds) {
    RectCoord boundsMapSpace = mapInterface->getCoordinateConverterHelper()->convertRect(mapCoordinateSystem.identifier, bounds);
    this->bounds = boundsMapSpace;

    const auto [adjPosition, adjZoom] = getBoundsCorrectedCoords(focusPointPosition, zoom);
    focusPointPosition = adjPosition;
    zoom = adjZoom;

    mapInterface->invalidate();
}

RectCoord MapCamera3d::getBounds() { return bounds; }

bool MapCamera3d::isInBounds(const Coord &coords) {
    Coord mapCoords = mapInterface->getCoordinateConverterHelper()->convert(mapCoordinateSystem.identifier, coords);

    auto const bounds = this->bounds;

    double minHor = std::min(bounds.topLeft.x, bounds.bottomRight.x);
    double maxHor = std::max(bounds.topLeft.x, bounds.bottomRight.x);
    double minVert = std::min(bounds.topLeft.y, bounds.bottomRight.y);
    double maxVert = std::max(bounds.topLeft.y, bounds.bottomRight.y);

    return mapCoords.x <= maxHor && mapCoords.x >= minHor && mapCoords.y <= maxVert && mapCoords.y >= minVert;
}


Coord MapCamera3d::adjustCoordForPadding(const Coord &coords, double targetZoom) {
    Coord coordinates = mapInterface->getCoordinateConverterHelper()->convert(mapCoordinateSystem.identifier, coords);

    auto adjustedZoom = std::clamp(targetZoom, zoomMax, zoomMin);

    Vec2D padVec = Vec2D(0.5 * (paddingRight - paddingLeft) * screenPixelAsRealMeterFactor * adjustedZoom,
                         0.5 * (paddingTop - paddingBottom) * screenPixelAsRealMeterFactor * adjustedZoom);
    Vec2D rotPadVec = Vec2DHelper::rotate(padVec, Vec2D(0.0, 0.0), angle);
    coordinates.x += rotPadVec.x;
    coordinates.y += rotPadVec.y;

    return coordinates;
}

RectCoord MapCamera3d::getPaddingCorrectedBounds(double zoom) {
    double const factor = screenPixelAsRealMeterFactor * zoom;

    double const addRight = (mapSystemRtl ? -1.0 : 1.0) * paddingRight * factor;
    double const addLeft = (mapSystemRtl ? 1.0 : -1.0) * paddingLeft * factor;
    double const addTop = (mapSystemTtb ? -1.0 : 1.0) * paddingTop * factor;
    double const addBottom = (mapSystemTtb ? 1.0 : -1.0) * paddingBottom * factor;

    // new top left and bottom right
    const auto &id = bounds.topLeft.systemIdentifier;
    Coord tl = Coord(id, bounds.topLeft.x + addLeft, bounds.topLeft.y + addTop, bounds.topLeft.z);
    Coord br = Coord(id, bounds.bottomRight.x + addRight, bounds.bottomRight.y + addBottom, bounds.bottomRight.z);

    return RectCoord(tl, br);
}

std::tuple<Coord, double> MapCamera3d::getBoundsCorrectedCoords(const Coord &position, double zoom) {
    // TODO: Take into account 'boundsRestrictWholeVisibleRect', which not only
    // clamps the cameraCenter, but the entire viewport. First step for example without rotation

    auto const &paddingCorrectedBounds = getPaddingCorrectedBounds(zoom);
    const auto &id = position.systemIdentifier;

    Coord topLeft = mapInterface->getCoordinateConverterHelper()->convert(id, paddingCorrectedBounds.topLeft);
    Coord bottomRight = mapInterface->getCoordinateConverterHelper()->convert(id, paddingCorrectedBounds.bottomRight);

    Coord clampedPosition = Coord(id,
                                  std::clamp(position.x,
                                             std::min(topLeft.x, bottomRight.x),
                                             std::max(topLeft.x, bottomRight.x)),
                                  std::clamp(position.y,
                                             std::min(topLeft.y, bottomRight.y),
                                             std::max(topLeft.y, bottomRight.y)),
                                  position.z);


    return {clampedPosition, zoom};
}

void MapCamera3d::setRotationEnabled(bool enabled) { config.rotationEnabled = enabled; }

void MapCamera3d::setSnapToNorthEnabled(bool enabled) { config.snapToNorthEnabled = enabled; }

void MapCamera3d::setBoundsRestrictWholeVisibleRect(bool enabled) {
    config.boundsRestrictWholeVisibleRect = enabled;
    clampCenterToPaddingCorrectedBounds();
}

void MapCamera3d::clampCenterToPaddingCorrectedBounds() {
    const auto [newPosition, newZoom] = getBoundsCorrectedCoords(focusPointPosition, zoom);
    focusPointPosition = newPosition;
    zoom = newZoom;
}


float MapCamera3d::getScreenDensityPpi() { return screenDensityPpi; }

std::shared_ptr<MapCamera3dInterface> MapCamera3d::asMapCamera3d() {
    return shared_from_this();
}

void MapCamera3d::setCameraMode(CameraMode3d mode) {
    this->mode = mode;
}

void MapCamera3d::notifyListenerBoundsChange() {
    notifyListeners(ListenerType::BOUNDS);
}

void MapCamera3d::updateZoom(double zoom_) {
    auto zoomMin = getMinZoom();
    auto zoomMax = getMaxZoom();

    zoom = std::clamp(zoom_, zoomMax, zoomMin);

    cameraDistance = getCameraDistance();
    cameraVerticalDisplacement = getCameraVerticalDisplacement();
    cameraPitch = getCameraPitch();
}

double MapCamera3d::getCameraVerticalDisplacement() {
    auto out = 0;
    auto in = 1;
    return in + (zoom * (out - in));
}

double MapCamera3d::getCameraPitch() {
    auto out = 5;
    auto in = 25;
    return in + (zoom * (out - in));
}

double MapCamera3d::getCameraDistance() {
    float R = 6378137.0;
    auto out = 8.0 * R; // earth-radii in m, above ground
    auto in = 100; // m above ground
    return in + (zoom * (out - in));
}

double MapCamera3d::getCenterYOffset() {
    double d = getCameraDistance();
    return -(d * d * -9.2987162 + 34.635713 * d - 50.0); // TODO: Fix constants, compute from camera parameters or use raycast from screen center to determine position, when available
}
