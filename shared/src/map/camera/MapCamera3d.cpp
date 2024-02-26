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
#include "Vec2D.h"
#include "Vec2DHelper.h"
#include "Vec2FHelper.h"
#include "Logger.h"
#include "CoordinateSystemIdentifiers.h"

#define DEFAULT_ANIM_LENGTH 300
#define ROTATION_THRESHOLD 20
#define ROTATION_LOCKING_ANGLE 10
#define ROTATION_LOCKING_FACTOR 1.5

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
      cameraRoll(0),
      cameraYaw(0)
    , bounds(mapCoordinateSystem.bounds) {
    mapSystemRtl = mapCoordinateSystem.bounds.bottomRight.x > mapCoordinateSystem.bounds.topLeft.x;
    mapSystemTtb = mapCoordinateSystem.bounds.bottomRight.y > mapCoordinateSystem.bounds.topLeft.y;
}

void MapCamera3d::viewportSizeChanged() {
    Vec2I viewportSize = mapInterface->getRenderingContext()->getViewportSize();
    if (viewportSize.x > 0 && viewportSize.y > 0 && zoomMin < 0) {
        double boundsWidthM = std::abs(bounds.topLeft.x - bounds.bottomRight.x);
        double boundsHeightM = std::abs(bounds.topLeft.y - bounds.bottomRight.y);
        double widthDeviceM = screenPixelAsRealMeterFactor * viewportSize.x;
        double heightDeviceM = screenPixelAsRealMeterFactor * viewportSize.y;
        zoomMin = std::max(boundsHeightM / heightDeviceM, boundsWidthM / widthDeviceM);
        zoom = std::clamp(zoom, zoomMax, zoomMin);
        cameraDistance = getCameraDistanceFromZoom(zoom);
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
    Coord currentFocusPointPosition = this->focusPointPosition;
    inertia = std::nullopt;
    Coord focusPosition = getBoundsCorrectedCoords(mapInterface->getCoordinateConverterHelper()->convert(CoordinateSystemIdentifiers::EPSG4326(), centerPosition));
    if (animated) {
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        coordAnimation = std::make_shared<CoordAnimation>(
            DEFAULT_ANIM_LENGTH, currentFocusPointPosition, focusPosition, centerPosition, InterpolatorFunction::EaseInOut,
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
        this->zoom = zoom;
        notifyListeners(ListenerType::BOUNDS);
        mapInterface->invalidate();
    }
}

void MapCamera3d::moveToCenterPosition(const ::Coord &centerPosition, bool animated) {
    if (cameraFrozen)
        return;
    Coord currentFocusPointPosition = this->focusPointPosition;
    inertia = std::nullopt;
    Coord focusPosition = getBoundsCorrectedCoords(mapInterface->getCoordinateConverterHelper()->convert(CoordinateSystemIdentifiers::EPSG4326(), centerPosition));
    if (animated) {
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        coordAnimation = std::make_shared<CoordAnimation>(
            DEFAULT_ANIM_LENGTH, currentFocusPointPosition, focusPosition, centerPosition, InterpolatorFunction::EaseInOut,
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
    if (cameraFrozen)
        return;
    double targetZoom = std::clamp(zoom, zoomMax, zoomMin);

    if (animated) {
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        zoomAnimation = std::make_shared<DoubleAnimation>(
            DEFAULT_ANIM_LENGTH, this->zoom, targetZoom, InterpolatorFunction::EaseIn,
            [=](double zoom) { this->setZoom(zoom, false); },
            [=] {
                this->setZoom(targetZoom, false);
                this->cameraDistance = getCameraDistanceFromZoom(this->zoom);
                this->zoomAnimation = nullptr;
            });
        zoomAnimation->start();
        mapInterface->invalidate();
    } else {
        this->zoom = targetZoom;
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
        const auto adjPosition =
                getBoundsCorrectedCoords(*coordAnimation->helperCoord);
        coordAnimation->endValue = adjPosition;
    } else {
        clampCenterToBounds();
    }
}

void MapCamera3d::setPaddingRight(float padding) {
    paddingRight = padding;
    std::lock_guard<std::recursive_mutex> lock(animationMutex);
    if (coordAnimation && coordAnimation->helperCoord.has_value()) {
        double targetZoom = (zoomAnimation) ? zoomAnimation->endValue : getZoom();
        const auto adjPosition =
                getBoundsCorrectedCoords(*coordAnimation->helperCoord);
        coordAnimation->endValue = adjPosition;
    } else {
        clampCenterToBounds();
    }
}

void MapCamera3d::setPaddingTop(float padding) {
    paddingTop = padding;
    std::lock_guard<std::recursive_mutex> lock(animationMutex);
    if (coordAnimation && coordAnimation->helperCoord.has_value()) {
        double targetZoom = (zoomAnimation) ? zoomAnimation->endValue : getZoom();
        const auto adjPosition =
                getBoundsCorrectedCoords(*coordAnimation->helperCoord);
        coordAnimation->endValue = adjPosition;
    } else {
        clampCenterToBounds();
    }
}

void MapCamera3d::setPaddingBottom(float padding) {
    paddingBottom = padding;
    std::lock_guard<std::recursive_mutex> lock(animationMutex);
    if (coordAnimation && coordAnimation->helperCoord.has_value()) {
        double targetZoom = (zoomAnimation) ? zoomAnimation->endValue : getZoom();
        const auto adjPosition =
                getBoundsCorrectedCoords(*coordAnimation->helperCoord);
        coordAnimation->endValue = adjPosition;
    } else {
        clampCenterToBounds();
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
    cameraPitch = 0.0;//90.0;

    focusPointAltitude = focusPointPosition.z;
    cameraDistance = currentZoom;
    float maxD = cameraDistance * 1.3;
    float minD = 100.0 / R;

    float fov = 60.0; // 45 // zoom / 70800;

    // aspect ratio
    float vpr = (float) sizeViewport.x / (float) sizeViewport.y;
    if (vpr > 1.0) {
        fov /= vpr;
    }

    Matrix::setIdentityM(newProjectionMatrix, 0);
    Matrix::perspectiveM(newProjectionMatrix, 0, fov, vpr, minD, maxD);

    Matrix::setIdentityM(newViewMatrix, 0);

    if (mode == CameraMode3d::GLOBE) {

        Matrix::translateM(newViewMatrix, 0, 0, 0, -cameraDistance);
        Matrix::rotateM(newViewMatrix, 0, -cameraPitch, 1.0, 0.0, 0.0);
        Matrix::rotateM(newViewMatrix, 0, -angle, 0, 0, 1);
        Matrix::translateM(newViewMatrix, 0, 0, 0, 0 - 1 - focusPointAltitude / R);
        Matrix::rotateM(newViewMatrix, 0.0, latitude, 1.0, 0.0, 0.0);
        Matrix::rotateM(newViewMatrix, 0.0, -longitude - 90.0, 0.0, 1.0, 0.0);
        std::vector<float> newVpMatrix(16, 0.0);
        Matrix::multiplyMM(newVpMatrix, 0, newProjectionMatrix, 0, newViewMatrix, 0);

    } else if (mode == CameraMode3d::TILTED_ORBITAL) {

        Matrix::rotateM(newViewMatrix, 0, -17.5, 1.0, 0.0, 0.0);
        double vCameraSpace = (4000.0 / (R / 1000.0)); // km
        double hCameraSpace = atan(4.5 / 180.0 * M_PI); // degr
        Matrix::translateM(newViewMatrix, 0, 0, hCameraSpace, -(1.0 + vCameraSpace));
        Matrix::rotateM(newViewMatrix, 0.0, latitude, 1.0, 0.0, 0.0);
        Matrix::rotateM(newViewMatrix, 0.0, -longitude - 90.0, 0.0, 1.0, 0.0);

    }

    std::vector<float> newVpMatrix(16, 0.0);
    Matrix::multiplyMM(newVpMatrix, 0, newProjectionMatrix, 0, newViewMatrix, 0);

    std::lock_guard<std::recursive_mutex> lock(vpDataMutex);
    lastVpBounds = viewBounds;
    lastVpRotation = currentRotation;
    lastVpZoom = currentZoom;
    vpMatrix = newVpMatrix;
    viewMatrix = newViewMatrix;
    projectionMatrix = newProjectionMatrix;
    verticalFov = fov;
    horizontalFov = fov * vpr;
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
        double zoomFactor = screenPixelAsRealMeterFactor * zoom;
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
        }

        Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
        width = sizeViewport.x;
        height = sizeViewport.y;
    }

    std::lock_guard<std::recursive_mutex> lock(listenerMutex);
    for (auto listener : listeners) {
        if (listenerType & ListenerType::BOUNDS) {
            listener->onCameraChange(viewMatrix, projectionMatrix, verticalFov, horizontalFov, width, height, focusPointAltitude);
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
    return true;
}

bool MapCamera3d::onMove(const Vec2F &deltaScreen, bool confirmed, bool doubleClick) {
    if (!config.moveEnabled || cameraFrozen)
        return false;

    inertia = std::nullopt;

    if (doubleClick) {
        double newZoom = zoom * (1.0 - (deltaScreen.y * 0.003));

        newZoom = std::max(std::min(newZoom, zoomMin), zoomMax);
        zoom = newZoom;
        cameraDistance = getCameraDistanceFromZoom(newZoom);

        notifyListeners(ListenerType::BOUNDS | ListenerType::MAP_INTERACTION);
        mapInterface->invalidate();
        return true;
    }

    float dx = deltaScreen.x;
    float dy = deltaScreen.y;

    float sinAngle = sin(angle * M_PI / 180.0);
    float cosAngle = cos(angle * M_PI / 180.0);

    float xDiff = (cosAngle * dx + sinAngle * dy);
    float yDiff = (-sinAngle * dx + cosAngle * dy);

    float xDiffMap = xDiff * zoom * screenPixelAsRealMeterFactor * (mapSystemRtl ? -1 : 1);
    float yDiffMap = yDiff * zoom * screenPixelAsRealMeterFactor * (mapSystemTtb ? -1 : 1);

    focusPointPosition.x += xDiffMap * 0.00001;
    focusPointPosition.y += yDiffMap * 0.00001;

    focusPointPosition.y = std::clamp(focusPointPosition.y, -90.0, 90.0);

    if (currentDragTimestamp == 0) {
        currentDragTimestamp = DateHelper::currentTimeMicros();
        currentDragVelocity.x = 0;
        currentDragVelocity.y = 0;
    } else {
        long long newTimestamp = DateHelper::currentTimeMicros();
        long long deltaMcs = std::max(newTimestamp - currentDragTimestamp, 8000ll);
        float averageFactor = currentDragVelocity.x == 0 && currentDragVelocity.y == 0 ? 1.0 : 0.5;
        float fasterIneratia = 10.0f;
        currentDragVelocity.x = (1 - averageFactor) * currentDragVelocity.x + averageFactor * xDiffMap / (deltaMcs / 16000.0) * fasterIneratia;
        currentDragVelocity.y = (1 - averageFactor) * currentDragVelocity.y + averageFactor * yDiffMap / (deltaMcs / 16000.0) * fasterIneratia;
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
    float vel = sqrt(currentDragVelocity.x * currentDragVelocity.x + currentDragVelocity.y * currentDragVelocity.y);
    double t1 = vel >= 1.0 ? -19.4957 * std::log(1.0 / vel) : 0.0;
    double t2 = vel >= 0.01 ? -1.95762 * std::log(0.01 / 1.0) : 0.0;
    inertia = Inertia(DateHelper::currentTimeMicros(), currentDragVelocity, t1, t2);
    currentDragVelocity = {0, 0};
    currentDragTimestamp = 0;
}

void MapCamera3d::inertiaStep() {
    if (inertia == std::nullopt)
        return;

    long long now = DateHelper::currentTimeMicros();
    double delta = (now - inertia->timestampStart) / 16000.0;
    double deltaPrev = (now - inertia->timestampUpdate) / 16000.0;

    if (delta >= inertia->t1 + inertia->t2) {
        inertia = std::nullopt;
        return;
    }

    float factor = std::pow(0.99, delta);
    if (delta > inertia->t1) {
        factor *= std::pow(0.6, delta - inertia->t1);
    }
    float xDiffMap = (inertia->velocity.x) * factor * deltaPrev;
    float yDiffMap = (inertia->velocity.y) * factor * deltaPrev;
    inertia->timestampUpdate = now;

    focusPointPosition = Coord(CoordinateSystemIdentifiers::EPSG4326(),
                               focusPointPosition.x + xDiffMap * 0.000001,
                               focusPointPosition.y + yDiffMap * 0.000001,
                               focusPointPosition.z);

    focusPointPosition.y = std::clamp(focusPointPosition.y, -90.0, 90.0);

    notifyListeners(ListenerType::BOUNDS);
    mapInterface->invalidate();
}

bool MapCamera3d::onDoubleClick(const ::Vec2F &posScreen) {
    if (!config.doubleClickZoomEnabled || cameraFrozen)
        return false;

    inertia = std::nullopt;

    auto targetZoom = zoom / 2;

    targetZoom = std::max(std::min(targetZoom, zoomMin), zoomMax);

    auto position = coordFromScreenPosition(posScreen);

    auto config = mapInterface->getMapConfig();
    auto bottomRight = bounds.bottomRight;
    auto topLeft = bounds.topLeft;

    position.x = std::min(position.x, bottomRight.x);
    position.x = std::max(position.x, topLeft.x);

    position.y = std::max(position.y, bottomRight.y);
    position.y = std::min(position.y, topLeft.y);

    moveToCenterPositionZoom(position, targetZoom, true);

    notifyListeners(ListenerType::MAP_INTERACTION);
    return true;
}

bool MapCamera3d::onTwoFingerClick(const ::Vec2F &posScreen1, const ::Vec2F &posScreen2) {
    if (!config.doubleClickZoomEnabled || cameraFrozen)
        return false;

    inertia = std::nullopt;

    auto targetZoom = zoom * 2;

    targetZoom = std::max(std::min(targetZoom, zoomMin), zoomMax);

    auto position = coordFromScreenPosition(Vec2FHelper::midpoint(posScreen1, posScreen2));

    auto config = mapInterface->getMapConfig();
    auto bottomRight = bounds.bottomRight;
    auto topLeft = bounds.topLeft;

    position.x = std::min(position.x, bottomRight.x);
    position.x = std::max(position.x, topLeft.x);

    position.y = std::max(position.y, bottomRight.y);
    position.y = std::min(position.y, topLeft.y);

    moveToCenterPositionZoom(position, targetZoom, true);

    notifyListeners(ListenerType::MAP_INTERACTION);
    return true;
}

void MapCamera3d::clearTouch() {
    isRotationThresholdReached = false;
    rotationPossible = true;
    tempAngle = angle;
    startZoom = 0;
}

bool MapCamera3d::onTwoFingerMove(const std::vector<::Vec2F> &posScreenOld, const std::vector<::Vec2F> &posScreenNew) {
    if (!config.twoFingerZoomEnabled || cameraFrozen)
        return false;

    inertia = std::nullopt;

    if (startZoom == 0) {
        startZoom = zoom;
    }

    if (posScreenOld.size() >= 2) {
        auto listenerType = ListenerType::BOUNDS | ListenerType::MAP_INTERACTION;

        double scaleFactor =
            Vec2FHelper::distance(posScreenNew[0], posScreenNew[1]) / Vec2FHelper::distance(posScreenOld[0], posScreenOld[1]);
        double newZoom = zoom / scaleFactor;

        newZoom = std::clamp(newZoom, zoomMax, zoomMin);
        cameraDistance = newZoom / 70;

        if (newZoom > startZoom * ROTATION_LOCKING_FACTOR || newZoom < startZoom / ROTATION_LOCKING_FACTOR) {
            rotationPossible = false;
        }

        auto midpoint = Vec2FHelper::midpoint(posScreenNew[0], posScreenNew[1]);
        auto oldMidpoint = Vec2FHelper::midpoint(posScreenOld[0], posScreenOld[1]);
        Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
        auto centerScreen = Vec2F(sizeViewport.x * 0.5f, sizeViewport.y * 0.5f);

        float dx = -(scaleFactor - 1) * (midpoint.x - centerScreen.x) + (midpoint.x - oldMidpoint.x);
        float dy = -(scaleFactor - 1) * (midpoint.y - centerScreen.y) + (midpoint.y - oldMidpoint.y);

        float sinAngle = sin(angle * M_PI / 180.0);
        float cosAngle = cos(angle * M_PI / 180.0);

        float leftDiff = (cosAngle * dx + sinAngle * dy);
        float topDiff = (-sinAngle * dx + cosAngle * dy);

        double diffCenterX = -leftDiff * newZoom * screenPixelAsRealMeterFactor;
        double diffCenterY = topDiff * newZoom * screenPixelAsRealMeterFactor;

        /*focusPointPosition.y += diffCenterX;
        focusPointPosition.x += diffCenterY;*/
        this->zoom = newZoom;

        if (config.rotationEnabled) {
            float olda = atan2(posScreenOld[0].x - posScreenOld[1].x, posScreenOld[0].y - posScreenOld[1].y);
            float newa = atan2(posScreenNew[0].x - posScreenNew[1].x, posScreenNew[0].y - posScreenNew[1].y);
            if (isRotationThresholdReached) {
                angle = fmod((angle + (olda - newa) / M_PI * 180.0) + 360.0, 360.0);

                // Update focusPointPosition such that the midpoint is the rotation center
                double centerXDiff = (centerScreen.x - midpoint.x) * cos(newa - olda) -
                                     (centerScreen.y - midpoint.y) * sin(newa - olda) + midpoint.x - centerScreen.x;
                double centerYDiff = (centerScreen.y - midpoint.y) * cos(newa - olda) -
                                     (centerScreen.x - midpoint.x) * sin(newa - olda) + midpoint.y - centerScreen.y;
                double rotDiffX = (cosAngle * centerXDiff - sinAngle * centerYDiff);
                double rotDiffY = (cosAngle * centerYDiff + sinAngle * centerXDiff);
                focusPointPosition.x += rotDiffX * zoom * screenPixelAsRealMeterFactor;
                focusPointPosition.y += rotDiffY * zoom * screenPixelAsRealMeterFactor;

                listenerType |= ListenerType::ROTATION;
            } else {
                tempAngle = fmod((tempAngle + (olda - newa) / M_PI * 180.0) + 360.0, 360.0);
                auto diff = std::min(std::abs(tempAngle - angle), std::abs(360.0 - (tempAngle - angle)));
                if (diff >= ROTATION_THRESHOLD && rotationPossible) {
                    isRotationThresholdReached = true;
                }
            }
        }

        auto mapConfig = mapInterface->getMapConfig();

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
    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
    double zoomFactor = screenPixelAsRealMeterFactor * zoom;

    double xDiffToCenter = zoomFactor * (posScreen.x - ((double)sizeViewport.x / 2.0));
    double yDiffToCenter = zoomFactor * (posScreen.y - ((double)sizeViewport.y / 2.0));

    double angRad = -angle * M_PI / 180.0;
    double sinAng = std::sin(angRad);
    double cosAng = std::cos(angRad);

    double adjXDiff = xDiffToCenter * cosAng - yDiffToCenter * sinAng;
    double adjYDiff = xDiffToCenter * sinAng + yDiffToCenter * cosAng;

    return Coord(focusPointPosition.systemIdentifier, focusPointPosition.x + adjXDiff, focusPointPosition.y - adjYDiff,
                 focusPointPosition.z);
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

    return Vec2F(posScreenX, posScreenY);
}

double MapCamera3d::mapUnitsFromPixels(double distancePx) { return distancePx * screenPixelAsRealMeterFactor * zoom; }

double MapCamera3d::getScalingFactor() { return mapUnitsFromPixels(1.0); }

void MapCamera3d::setMinZoom(double zoomMin) {
    this->zoomMin = zoomMin;
    if (zoom > zoomMin) {
        zoom = zoomMin;
        cameraDistance = getCameraDistanceFromZoom(zoomMin);
    }
    mapInterface->invalidate();
}

void MapCamera3d::setMaxZoom(double zoomMax) {
    this->zoomMax = zoomMax;
    if (zoom < zoomMax) {
        zoom = zoomMax;
        cameraDistance = getCameraDistanceFromZoom(zoomMax);
    }
    mapInterface->invalidate();
}

double MapCamera3d::getMinZoom() { return zoomMin; }

double MapCamera3d::getMaxZoom() { return zoomMax; }

void MapCamera3d::setBounds(const RectCoord &bounds) {
    RectCoord boundsMapSpace = mapInterface->getCoordinateConverterHelper()->convertRect(mapCoordinateSystem.identifier, bounds);
    this->bounds = boundsMapSpace;

    const auto adjPosition = getBoundsCorrectedCoords(focusPointPosition);
    focusPointPosition = adjPosition;

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

Coord MapCamera3d::getBoundsCorrectedCoords(const Coord &position) {
    /*auto const bounds = this->bounds;

    Coord newPosition = Coord(position.systemIdentifier,
                              std::clamp(position.x,
                                         std::min(bounds.topLeft.x, bounds.bottomRight.x),
                                         std::max(bounds.topLeft.x, bounds.bottomRight.x)),
                              std::clamp(position.y,
                                         std::min(bounds.topLeft.y, bounds.bottomRight.y),
                                         std::max(bounds.topLeft.y, bounds.bottomRight.y)),
                              position.z);
    return newPosition;*/ // TODO: not needed for circular system (4326), otherwise fix
    return position;
}

void MapCamera3d::clampCenterToBounds() {
    /*const auto newPosition = getBoundsCorrectedCoords(focusPointPosition);
    focusPointPosition = newPosition;*/
}

void MapCamera3d::setRotationEnabled(bool enabled) { config.rotationEnabled = enabled; }

void MapCamera3d::setSnapToNorthEnabled(bool enabled) { config.snapToNorthEnabled = enabled; }

void MapCamera3d::setBoundsRestrictWholeVisibleRect(bool enabled) {
    return;
}

float MapCamera3d::getScreenDensityPpi() { return screenDensityPpi; }

double MapCamera3d::getCameraDistanceFromZoom(double zoom) {
    return zoom / 70.0;
}

std::shared_ptr<MapCamera3dInterface> MapCamera3d::asMapCamera3d() {
    return shared_from_this();
}

void MapCamera3d::setCameraMode(CameraMode3d mode) {
    this->mode = mode;
}
