/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "MapCamera2d.h"
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
#include "CoordinateSystemIdentifiers.h"

#define DEFAULT_ANIM_LENGTH 300
#define ROTATION_THRESHOLD 20
#define ROTATION_LOCKING_ANGLE 10
#define ROTATION_LOCKING_FACTOR 1.5

MapCamera2d::MapCamera2d(const std::shared_ptr<MapInterface> &mapInterface, float screenDensityPpi)
    : mapInterface(mapInterface)
    , conversionHelper(mapInterface->getCoordinateConverterHelper())
    , mapCoordinateSystem(mapInterface->getMapConfig().mapCoordinateSystem)
    , screenDensityPpi(screenDensityPpi)
    , screenPixelAsRealMeterFactor(0.0254 / screenDensityPpi)
    , bounds(mapCoordinateSystem.bounds)
    , focusPointPosition(CoordinateSystemIdentifiers::EPSG4326(), 0, 0, 0)
    , focusPointAltitude(0)
    , cameraDistance(10000)
    , cameraPitch(0)
    , cameraRoll(0)
    , cameraYaw(0)
{
    auto mapConfig = mapInterface->getMapConfig();
    mapCoordinateSystem = mapConfig.mapCoordinateSystem;
    mapSystemRtl = mapCoordinateSystem.bounds.bottomRight.x > mapCoordinateSystem.bounds.topLeft.x;
    mapSystemTtb = mapCoordinateSystem.bounds.bottomRight.y > mapCoordinateSystem.bounds.topLeft.y;
    focusPointPosition = Coord(CoordinateSystemIdentifiers::EPSG4326(), 46.5326, 8.0447, 0);
    zoom = zoomMax;
    cameraDistance = zoom / 50;
}

void MapCamera2d::viewportSizeChanged() {
    Vec2I viewportSize = mapInterface->getRenderingContext()->getViewportSize();
    if (viewportSize.x > 0 && viewportSize.y > 0 && zoomMin < 0) {
        double boundsWidthM = std::abs(bounds.topLeft.x - bounds.bottomRight.x);
        double boundsHeightM = std::abs(bounds.topLeft.y - bounds.bottomRight.y);
        double widthDeviceM = screenPixelAsRealMeterFactor * viewportSize.x;
        double heightDeviceM = screenPixelAsRealMeterFactor * viewportSize.y;
        zoomMin = std::max(boundsHeightM / heightDeviceM, boundsWidthM / widthDeviceM);
        zoom = std::clamp(zoom, zoomMax, zoomMin);
        cameraDistance = zoom / 70;
    }
    // this->validVpMatrix = false;

    notifyListeners(ListenerType::BOUNDS | ListenerType::CAMERA);
}

void MapCamera2d::freeze(bool freeze) {
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

void MapCamera2d::moveToCenterPositionZoom(const ::Coord &centerPosition, double zoom, bool animated) {
    if (cameraFrozen)
        return;
    Coord currentFocusPointPosition = this->focusPointPosition;
    inertia = std::nullopt;
    // TODO: bounds correction
//    Coord positionMapSystem = getBoundsCorrectedCoords(adjustCoordForPadding(focusPointPosition, zoom));
    Coord focusPosition = mapInterface->getCoordinateConverterHelper()->convert(CoordinateSystemIdentifiers::EPSG4326(), centerPosition);
    if (animated) {
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        coordAnimation = std::make_shared<CoordAnimation>(
            DEFAULT_ANIM_LENGTH, currentFocusPointPosition, focusPosition, std::nullopt, InterpolatorFunction::EaseInOut,
            [=](Coord focusPosition) {
                this->focusPointPosition = focusPosition;
                // this->validVpMatrix = false;
                notifyListeners(ListenerType::BOUNDS | ListenerType::CAMERA);
                mapInterface->invalidate();
            },
            [=] {
                this->focusPointPosition = focusPosition;
                // this->validVpMatrix = false;
                notifyListeners(ListenerType::BOUNDS | ListenerType::CAMERA);
                mapInterface->invalidate();
                this->coordAnimation = nullptr;
            });
        coordAnimation->start();
        setZoom(zoom, true);
        mapInterface->invalidate();
    } else {
        this->focusPointPosition = focusPosition;
        setZoom(zoom, false);
        // this->validVpMatrix = false;
        notifyListeners(ListenerType::BOUNDS | ListenerType::CAMERA);
        mapInterface->invalidate();
    }
}

void MapCamera2d::moveToCenterPosition(const ::Coord &centerPosition, bool animated) {
    if (cameraFrozen)
        return;
    Coord currentFocusPointPosition = this->focusPointPosition;
    inertia = std::nullopt;
    Coord focusPosition = mapInterface->getCoordinateConverterHelper()->convert(CoordinateSystemIdentifiers::EPSG4326(), focusPointPosition);
    // TODO: bounds correction and third coordinate parameter
//    Coord positionMapSystem = getBoundsCorrectedCoords(adjustCoordForPadding(focusPointPosition, zoom));
    if (animated) {
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        coordAnimation = std::make_shared<CoordAnimation>(
            DEFAULT_ANIM_LENGTH, currentFocusPointPosition, focusPosition, std::nullopt, InterpolatorFunction::EaseInOut,
            [=](Coord focusPosition) {
                this->focusPointPosition = focusPosition;
                // this->validVpMatrix = false;
                notifyListeners(ListenerType::BOUNDS | ListenerType::CAMERA);
                mapInterface->invalidate();
            },
            [=] {
                this->focusPointPosition = focusPosition;
                // this->validVpMatrix = false;
                notifyListeners(ListenerType::BOUNDS | ListenerType::CAMERA);
                mapInterface->invalidate();
                this->coordAnimation = nullptr;
            });
        coordAnimation->start();
        mapInterface->invalidate();
    } else {
        this->focusPointPosition = focusPosition;
        // this->validVpMatrix = false;
        notifyListeners(ListenerType::BOUNDS | ListenerType::CAMERA);
        mapInterface->invalidate();
    }
}

void MapCamera2d::moveToBoundingBox(const RectCoord &boundingBox, float paddingPc, bool animated, std::optional<double> maxZoom) {
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

    if (maxZoom.has_value()) {
        targetZoom = std::min(targetZoom, *maxZoom);
    }

    moveToCenterPositionZoom(targetCenterNotBC, targetZoom, animated);
}

::Coord MapCamera2d::getCenterPosition() {
    return focusPointPosition;
    // TODO
//    Coord center = focusPointPosition;
//
//    Vec2D padVec = Vec2D(0.5 * (paddingLeft - paddingRight) * screenPixelAsRealMeterFactor * zoom,
//                         0.5 * (paddingBottom - paddingTop) * screenPixelAsRealMeterFactor * zoom);
//    Vec2D rotPadVec = Vec2DHelper::rotate(padVec, Vec2D(0.0, 0.0), angle);
//    center.x += rotPadVec.x;
//    center.y += rotPadVec.y;
//
//    return center;
}

void MapCamera2d::setZoom(double zoom, bool animated) {
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
                this->zoomAnimation = nullptr;
            });
        zoomAnimation->start();
        mapInterface->invalidate();
    } else {
        this->zoom = targetZoom;
        this->cameraDistance = this->zoom / 70;
        // this->validVpMatrix = false;
        notifyListeners(ListenerType::BOUNDS | ListenerType::CAMERA);
        mapInterface->invalidate();
    }
}

double MapCamera2d::getZoom() { return zoom; }

void MapCamera2d::setRotation(float angle, bool animated) {
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
        Coord centerScreen = focusPointPosition; // TODO maybe wrong
        Coord realCenter = getCenterPosition();
        Vec2D rotatedDiff =
            Vec2DHelper::rotate(Vec2D(centerScreen.x - realCenter.x, centerScreen.y - realCenter.y), Vec2D(0.0, 0.0), angleDiff);
        focusPointPosition.x = realCenter.x + rotatedDiff.x;
        focusPointPosition.y = realCenter.y + rotatedDiff.y;

        this->angle = newAngle;
        // this->validVpMatrix = false;
        notifyListeners(ListenerType::ROTATION | ListenerType::BOUNDS | ListenerType::CAMERA);
        mapInterface->invalidate();
    }
}

float MapCamera2d::getRotation() { return angle; }

void MapCamera2d::setPaddingLeft(float padding) {
    paddingLeft = padding;
    std::lock_guard<std::recursive_mutex> lock(animationMutex);
    if (coordAnimation && coordAnimation->helperCoord.has_value()) {
        double targetZoom = (zoomAnimation) ? zoomAnimation->endValue : getZoom();
        coordAnimation->endValue = getBoundsCorrectedCoords(adjustCoordForPadding(*coordAnimation->helperCoord, targetZoom));
    }
}

void MapCamera2d::setPaddingRight(float padding) {
    paddingRight = padding;
    std::lock_guard<std::recursive_mutex> lock(animationMutex);
    if (coordAnimation && coordAnimation->helperCoord.has_value()) {
        double targetZoom = (zoomAnimation) ? zoomAnimation->endValue : getZoom();
        coordAnimation->endValue = getBoundsCorrectedCoords(adjustCoordForPadding(*coordAnimation->helperCoord, targetZoom));
    }
}

void MapCamera2d::setPaddingTop(float padding) {
    paddingTop = padding;
    std::lock_guard<std::recursive_mutex> lock(animationMutex);
    if (coordAnimation && coordAnimation->helperCoord.has_value()) {
        double targetZoom = (zoomAnimation) ? zoomAnimation->endValue : getZoom();
        coordAnimation->endValue = getBoundsCorrectedCoords(adjustCoordForPadding(*coordAnimation->helperCoord, targetZoom));
    }
}

void MapCamera2d::setPaddingBottom(float padding) {
    paddingBottom = padding;
    std::lock_guard<std::recursive_mutex> lock(animationMutex);
    if (coordAnimation && coordAnimation->helperCoord.has_value()) {
        double targetZoom = (zoomAnimation) ? zoomAnimation->endValue : getZoom();
        coordAnimation->endValue = getBoundsCorrectedCoords(adjustCoordForPadding(*coordAnimation->helperCoord, targetZoom));
    }
}

void MapCamera2d::addListener(const std::shared_ptr<MapCamera2dListenerInterface> &listener) {
    {
        std::lock_guard<std::recursive_mutex> lock(listenerMutex);
        if (listeners.count(listener) == 0) {
            listeners.insert(listener);
        }
    }
    notifyListeners(ListenerType::CAMERA);
}

void MapCamera2d::removeListener(const std::shared_ptr<MapCamera2dListenerInterface> &listener) {
    std::lock_guard<std::recursive_mutex> lock(listenerMutex);
    if (listeners.count(listener) > 0) {
        listeners.erase(listener);
    }
}

std::shared_ptr<::CameraInterface> MapCamera2d::asCameraInterface() { return shared_from_this(); }


::Vec2D MapCamera2d::project(const ::Coord & position) {
//    auto matrix = getVpMatrix();
    auto mapCoord = mapInterface->getCoordinateConverterHelper()->convert(CoordinateSystemIdentifiers::UNITSPHERE(), position);
    std::vector<float> inVec; // TODO: Performance, how to build a vec
    inVec.push_back(mapCoord.x);
    inVec.push_back(mapCoord.y);
    inVec.push_back(mapCoord.z);
    inVec.push_back(1.0);
    std::vector<float> outVec = {0, 0, 0, 0};

    std::lock_guard<std::recursive_mutex> lock(vpDataMutex);
    Matrix::multiply(vpMatrix, inVec, outVec);

//    printf("%f, %f, %f -> %f, %f\n", position.x, position.y, position.z, outVec[0], outVec[1]);
    return Vec2D(outVec[0] / outVec[3], outVec[1] / outVec[3]);
}

std::vector<float> MapCamera2d::getVpMatrix() {
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
    double zoomFactor = screenPixelAsRealMeterFactor * currentZoom;
    RectCoord viewBounds = getRectFromViewport(sizeViewport, focusPointPosition);


    if (mapInterface->getMapConfig().mapCoordinateSystem.identifier != CoordinateSystemIdentifiers::UNITSPHERE()) {

        std::vector<float> newViewMatrix(16, 0.0);
        std::vector<float> newProjectionMatrix(16, 0.0);

        Coord renderCoordCenter = conversionHelper->convertToRenderSystem(focusPointPosition);

        Matrix::orthoM(newProjectionMatrix, 0, - 0.5 * sizeViewport.x,  0.5 * sizeViewport.x,
                        0.5 * sizeViewport.y, - 0.5 * sizeViewport.y, -1, 1);

//        Matrix::orthoM(newViewMatrix, 0, renderCoordCenter.x - 0.5 * sizeViewport.x, renderCoordCenter.x + 0.5 * sizeViewport.x,
//                       renderCoordCenter.y + 0.5 * sizeViewport.y, renderCoordCenter.y - 0.5 * sizeViewport.y, -1, 1);

        Matrix::setIdentityM(newViewMatrix, 0);

//        Matrix::translateM(newViewMatrix, 0, renderCoordCenter.x, renderCoordCenter.y, 0);

        Matrix::scaleM(newViewMatrix, 0, 1 / zoomFactor, 1 / zoomFactor, 1);

        Matrix::rotateM(newViewMatrix, 0.0, currentRotation, 0.0, 0.0, 1.0);

        Matrix::translateM(newViewMatrix, 0, -renderCoordCenter.x, -renderCoordCenter.y, 0);

        std::vector<float> newVpMatrix(16, 0.0);
        Matrix::multiplyMM(newVpMatrix, 0, newProjectionMatrix, 0, newViewMatrix, 0);

        std::lock_guard<std::recursive_mutex> lock(vpDataMutex);
        lastVpBounds = viewBounds;
        lastVpRotation = currentRotation;
        lastVpZoom = currentZoom;
        vpMatrix = newVpMatrix;
        viewMatrix = newViewMatrix;
        projectionMatrix = newProjectionMatrix;
        verticalFov = 0;
        horizontalFov = 0;
        validVpMatrix = true;
        return newVpMatrix;
    }
    else {

        std::vector<float> newViewMatrix(16, 0.0);
        std::vector<float> newProjectionMatrix(16, 0.0);

        float R = 6371000;
        float longitude = focusPointPosition.x; //  px / R;
        float latitude = focusPointPosition.y; // 2*atan(exp(py / R)) - 3.1415926 / 2;
//
//        focusPointPosition.x -= 0.001;
//        focusPointPosition.y += 0.0015;

        float radius = 1.0;


        Matrix::setIdentityM(newProjectionMatrix, 0);

        float fov = 90; // zoom / 70800;

        float vpr = (float)sizeViewport.x / (float)sizeViewport.y;



        Matrix::perspectiveM(newProjectionMatrix, 0, fov, vpr, 0.0000001, 5.0);




        Matrix::setIdentityM(newViewMatrix, 0);


//        Matrix::translateM(newViewMatrix, 0, 0.0, +0.3, 0.0);

//        angle += 0.1;



        cameraPitch = 40;
        focusPointAltitude = 2000.0;


        Matrix::translateM(newViewMatrix, 0, 0, 0, -cameraDistance);

        Matrix::rotateM(newViewMatrix, 0, -cameraPitch, 1.0, 0.0, 0.0);
        Matrix::rotateM(newViewMatrix, 0, -angle, 0, 0, 1);


        Matrix::translateM(newViewMatrix, 0, 0, 0, -1 - focusPointAltitude / R);


        Matrix::rotateM(newViewMatrix, 0.0, longitude, 1.0, 0.0, 0.0);
        Matrix::rotateM(newViewMatrix, 0.0, -latitude , 0.0, 1.0, 0.0);

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


}

std::optional<::RectCoord> MapCamera2d::getLastVpMatrixViewBounds() {
    std::lock_guard<std::recursive_mutex> lock(vpDataMutex);
    return lastVpBounds;
}

std::optional<float> MapCamera2d::getLastVpMatrixRotation() {
    std::lock_guard<std::recursive_mutex> lock(vpDataMutex);
    return lastVpRotation;
}

std::optional<float> MapCamera2d::getLastVpMatrixZoom() {
    std::lock_guard<std::recursive_mutex> lock(vpDataMutex);
    return lastVpZoom;
}

/** this method is called just before the update methods on all layers */
void MapCamera2d::update() { inertiaStep(); }

std::vector<float> MapCamera2d::getInvariantModelMatrix(const ::Coord &coordinate, bool scaleInvariant, bool rotationInvariant) {
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

RectCoord MapCamera2d::getVisibleRect() {
    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
    return getRectFromViewport(sizeViewport, focusPointPosition);
}

RectCoord MapCamera2d::getPaddingAdjustedVisibleRect() {
    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();

    // adjust viewport
    sizeViewport.x -= (paddingLeft + paddingRight);
    sizeViewport.y -= (paddingTop + paddingBottom);

    // also use the padding adjusted center position
    return getRectFromViewport(sizeViewport, getCenterPosition());
}

RectCoord MapCamera2d::getRectFromViewport(const Vec2I &sizeViewport, const Coord &center) {
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

void MapCamera2d::notifyListeners(const int &listenerType) {
    std::optional<RectCoord> visibleRect =
        (listenerType & ListenerType::BOUNDS) ? std::optional<RectCoord>(getVisibleRect()) : std::nullopt;
    double angle = this->angle;
    double zoom = this->zoom;
    std::vector<float> viewMatrix;
    std::vector<float> projectionMatrix;
    float width;
    float height;
    float horizontalFov;
    float verticalFov;
    float focusPointAltitude;

    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
    width = sizeViewport.x;
    height = sizeViewport.y;
    if (width == 0 || height == 0) {
        return;
    }

    if (listenerType & ListenerType::CAMERA) {
        bool validVpMatrix;
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


    }


    std::lock_guard<std::recursive_mutex> lock(listenerMutex);
    for (auto listener : listeners) {
        if (listenerType & ListenerType::BOUNDS) {
            listener->onVisibleBoundsChanged(*visibleRect, zoom);
        }
        if (listenerType & ListenerType::ROTATION) {
            listener->onRotationChanged(angle);
        }
        if (listenerType & ListenerType::MAP_INTERACTION) {
            listener->onMapInteraction();
        }

        if (listenerType & ListenerType::CAMERA) {
            listener->onCameraChange(viewMatrix, projectionMatrix, verticalFov, horizontalFov, width, height, focusPointAltitude);
        }
    }
}

bool MapCamera2d::onTouchDown(const ::Vec2F &posScreen) {
    inertia = std::nullopt;
    return true;
}

bool MapCamera2d::onMove(const Vec2F &deltaScreen, bool confirmed, bool doubleClick) {
    if (!config.moveEnabled || cameraFrozen)
        return false;

    inertia = std::nullopt;

    if (doubleClick) {
        double newZoom = zoom * (1.0 - (deltaScreen.y * 0.003));

        zoom = std::max(std::min(newZoom, zoomMin), zoomMax);
        cameraDistance = zoom / 70;
        // this->validVpMatrix = false;

        notifyListeners(ListenerType::BOUNDS | ListenerType::MAP_INTERACTION | ListenerType::CAMERA);
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

    if (mapInterface->getMapConfig().mapCoordinateSystem.identifier != CoordinateSystemIdentifiers::UNITSPHERE()) {
        focusPointPosition.x += xDiffMap * 5 * 0.000001;
        focusPointPosition.y += yDiffMap * 5 * 0.000001;
    }
    else {
        focusPointPosition.y += xDiffMap * 5;
        focusPointPosition.x -= yDiffMap * 5;
    }


//    focusPointPosition = conversionHelper->convert(CoordinateSystemIdentifiers::EPSG4326(), focusPointPosition);

    clampCenterToPaddingCorrectedBounds();

    if (currentDragTimestamp == 0) {
        currentDragTimestamp = DateHelper::currentTimeMicros();
        currentDragVelocity.x = 0;
        currentDragVelocity.y = 0;
    } else {
        long long newTimestamp = DateHelper::currentTimeMicros();
        long long deltaMcs = std::max(newTimestamp - currentDragTimestamp, 8000ll);
        float averageFactor = currentDragVelocity.x == 0 && currentDragVelocity.y == 0 ? 1.0 : 0.5;
        currentDragVelocity.x = (1 - averageFactor) * currentDragVelocity.x + averageFactor * xDiffMap / (deltaMcs / 16000.0);
        currentDragVelocity.y = (1 - averageFactor) * currentDragVelocity.y + averageFactor * yDiffMap / (deltaMcs / 16000.0);
        currentDragTimestamp = newTimestamp;
    }
    // this->validVpMatrix = false;

    notifyListeners(ListenerType::BOUNDS | ListenerType::MAP_INTERACTION | ListenerType::CAMERA);
    mapInterface->invalidate();
    return true;
}

bool MapCamera2d::onMoveComplete() {
    if (cameraFrozen)
        return false;
    setupInertia();
    return true;
}

void MapCamera2d::setupInertia() {
    float vel = sqrt(currentDragVelocity.x * currentDragVelocity.x + currentDragVelocity.y * currentDragVelocity.y);
    double t1 = vel >= 1.0 ? -19.4957 * std::log(1.0 / vel) : 0.0;
    double t2 = vel >= 0.01 ? -1.95762 * std::log(0.01 / 1.0) : 0.0;
    inertia = Inertia(DateHelper::currentTimeMicros(), currentDragVelocity, t1, t2);
    currentDragVelocity = {0, 0};
    currentDragTimestamp = 0;
}

void MapCamera2d::inertiaStep() {
    if (inertia == std::nullopt)
        return;

    long long now = DateHelper::currentTimeMicros();
    double delta = (now - inertia->timestampStart) / 16000.0;
    double deltaPrev = (now - inertia->timestampUpdate) / 16000.0;

    if (delta >= inertia->t1 + inertia->t2) {
        inertia = std::nullopt;
        return;
    }

    float factor = std::pow(0.95, delta);
    if (delta > inertia->t1) {
        factor *= std::pow(0.6, delta - inertia->t1);
    }
    float xDiffMap = (inertia->velocity.x) * factor * deltaPrev;
    float yDiffMap = (inertia->velocity.y) * factor * deltaPrev;
    inertia->timestampUpdate = now;

    if (mapInterface->getMapConfig().mapCoordinateSystem.identifier != CoordinateSystemIdentifiers::UNITSPHERE()) {
        focusPointPosition.x += xDiffMap * 5 * 0.0000001;
        focusPointPosition.y += yDiffMap * 5 * 0.0000001;
    }
    else {
        focusPointPosition.y += xDiffMap * 5;
        focusPointPosition.x -= yDiffMap * 5;
    }
//    focusPointPosition = conversionHelper->convert(CoordinateSystemIdentifiers::EPSG4326(), focusPointPosition);

    clampCenterToPaddingCorrectedBounds();
    // this->validVpMatrix = false;

    notifyListeners(ListenerType::BOUNDS | ListenerType::CAMERA);
    mapInterface->invalidate();
}

bool MapCamera2d::onDoubleClick(const ::Vec2F &posScreen) {
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

bool MapCamera2d::onTwoFingerClick(const ::Vec2F &posScreen1, const ::Vec2F &posScreen2) {
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

void MapCamera2d::clearTouch() {
    isRotationThreasholdReached = false;
    rotationPossible = true;
    tempAngle = angle;
    startZoom = 0;
}

bool MapCamera2d::onTwoFingerMove(const std::vector<::Vec2F> &posScreenOld, const std::vector<::Vec2F> &posScreenNew) {
    if (!config.twoFingerZoomEnabled || cameraFrozen)
        return false;

    inertia = std::nullopt;

    if (startZoom == 0)
        startZoom = zoom;
    if (posScreenOld.size() >= 2) {
        auto listenerType = ListenerType::BOUNDS | ListenerType::MAP_INTERACTION;

        double scaleFactor =
            Vec2FHelper::distance(posScreenNew[0], posScreenNew[1]) / Vec2FHelper::distance(posScreenOld[0], posScreenOld[1]);
        zoom /= scaleFactor;

        zoom = std::clamp(zoom, zoomMax, zoomMin);
        cameraDistance = zoom / 70;

        if (zoom > startZoom * ROTATION_LOCKING_FACTOR || zoom < startZoom / ROTATION_LOCKING_FACTOR) {
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

        double diffCenterX = -leftDiff * zoom * screenPixelAsRealMeterFactor;
        double diffCenterY = topDiff * zoom * screenPixelAsRealMeterFactor;

        focusPointPosition.y += diffCenterX;
        focusPointPosition.x += diffCenterY;
//        focusPointPosition = conversionHelper->convert(CoordinateSystemIdentifiers::EPSG4326(), focusPointPosition);

        if (config.rotationEnabled) {
            float olda = atan2(posScreenOld[0].x - posScreenOld[1].x, posScreenOld[0].y - posScreenOld[1].y);
            float newa = atan2(posScreenNew[0].x - posScreenNew[1].x, posScreenNew[0].y - posScreenNew[1].y);
            if (isRotationThreasholdReached) {
                angle = fmod((angle + (olda - newa) / M_PI * 180.0) + 360.0, 360.0);

                // Update focusPointPosition such that the midpoint is the rotation center
                double centerXDiff = (centerScreen.x - midpoint.x) * cos(newa - olda) -
                                     (centerScreen.y - midpoint.y) * sin(newa - olda) + midpoint.x - centerScreen.x;
                double centerYDiff = (centerScreen.y - midpoint.y) * cos(newa - olda) -
                                     (centerScreen.x - midpoint.x) * sin(newa - olda) + midpoint.y - centerScreen.y;
                double rotDiffX = (cosAngle * centerXDiff - sinAngle * centerYDiff);
                double rotDiffY = (cosAngle * centerYDiff + sinAngle * centerXDiff);
                focusPointPosition.y += rotDiffX * zoom * screenPixelAsRealMeterFactor;
                focusPointPosition.x += rotDiffY * zoom * screenPixelAsRealMeterFactor;
//                focusPointPosition = conversionHelper->convert(CoordinateSystemIdentifiers::EPSG4326(), focusPointPosition);

                listenerType |= ListenerType::ROTATION;
            } else {
                tempAngle = fmod((tempAngle + (olda - newa) / M_PI * 180.0) + 360.0, 360.0);
                auto diff = std::min(std::abs(tempAngle - angle), std::abs(360.0 - (tempAngle - angle)));
                if (diff >= ROTATION_THRESHOLD && rotationPossible) {
                    isRotationThreasholdReached = true;
                }
            }
        }

        auto mapConfig = mapInterface->getMapConfig();

        clampCenterToPaddingCorrectedBounds();

        notifyListeners(listenerType);
        mapInterface->invalidate();
    }
    return true;
}

bool MapCamera2d::onTwoFingerMoveComplete() {
    if (config.snapToNorthEnabled && !cameraFrozen && (angle < ROTATION_LOCKING_ANGLE || angle > (360 - ROTATION_LOCKING_ANGLE))) {
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        rotationAnimation = std::make_shared<DoubleAnimation>(
            DEFAULT_ANIM_LENGTH, this->angle, angle < ROTATION_LOCKING_ANGLE ? 0 : 360, InterpolatorFunction::EaseInOut,
            [=](double angle) {
                this->angle = angle;
                // this->validVpMatrix = false;
                mapInterface->invalidate();
                notifyListeners(ListenerType::ROTATION | ListenerType::BOUNDS | ListenerType::CAMERA);
            },
            [=] {
                this->angle = 0;
                this->rotationAnimation = nullptr;
                // this->validVpMatrix = false;
                mapInterface->invalidate();
                notifyListeners(ListenerType::ROTATION | ListenerType::BOUNDS | ListenerType::CAMERA);
            });
        rotationAnimation->start();
        mapInterface->invalidate();
        return true;
    }

    return false;
}

Coord MapCamera2d::coordFromScreenPosition(const ::Vec2F &posScreen) {
    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
    double zoomFactor = screenPixelAsRealMeterFactor * zoom;

    double xDiffToCenter = zoomFactor * (posScreen.x - ((double)sizeViewport.x / 2.0));
    double yDiffToCenter = zoomFactor * (posScreen.y - ((double)sizeViewport.y / 2.0));

    double angRad = -angle * M_PI / 180.0;
    double sinAng = std::sin(angRad);
    double cosAng = std::cos(angRad);

    double adjXDiff = xDiffToCenter * cosAng - yDiffToCenter * sinAng;
    double adjYDiff = xDiffToCenter * sinAng + yDiffToCenter * cosAng;

    return Coord(focusPointPosition.systemIdentifier, focusPointPosition.x + adjXDiff, focusPointPosition.y - adjYDiff, focusPointPosition.z);
}

double MapCamera2d::mapUnitsFromPixels(double distancePx) { return distancePx * screenPixelAsRealMeterFactor * zoom; }

double MapCamera2d::getScalingFactor() { return mapUnitsFromPixels(1.0); }

void MapCamera2d::setMinZoom(double zoomMin) {
    this->zoomMin = zoomMin;
    if (zoom > zoomMin) {
        zoom = zoomMin;
        cameraDistance = zoom / 70;
    }
    mapInterface->invalidate();
}

void MapCamera2d::setMaxZoom(double zoomMax) {
    this->zoomMax = zoomMax;
    if (zoom < zoomMax) {
        zoom = zoomMax;
        cameraDistance = zoom / 70;
    }
    mapInterface->invalidate();
}

double MapCamera2d::getMinZoom() { return zoomMin; }

double MapCamera2d::getMaxZoom() { return zoomMax; }

void MapCamera2d::setBounds(const RectCoord &bounds) {
    RectCoord boundsMapSpace = mapInterface->getCoordinateConverterHelper()->convertRect(mapCoordinateSystem.identifier, bounds);
    this->bounds = boundsMapSpace;

    focusPointPosition = getBoundsCorrectedCoords(focusPointPosition);
    focusPointPosition = conversionHelper->convert(CoordinateSystemIdentifiers::EPSG4326(), focusPointPosition);

    mapInterface->invalidate();
}

RectCoord MapCamera2d::getBounds() { return bounds; }

bool MapCamera2d::isInBounds(const Coord &coords) {
    Coord mapCoords = mapInterface->getCoordinateConverterHelper()->convert(mapCoordinateSystem.identifier, coords);

    auto const &paddingCorrectedBounds = getPaddingCorrectedBounds();

    double minHor = std::min(paddingCorrectedBounds.topLeft.x, paddingCorrectedBounds.bottomRight.x);
    double maxHor = std::max(paddingCorrectedBounds.topLeft.x, paddingCorrectedBounds.bottomRight.x);
    double minVert = std::min(paddingCorrectedBounds.topLeft.y, paddingCorrectedBounds.bottomRight.y);
    double maxVert = std::max(paddingCorrectedBounds.topLeft.y, paddingCorrectedBounds.bottomRight.y);

    return mapCoords.x <= maxHor && mapCoords.x >= minHor && mapCoords.y <= maxVert && mapCoords.y >= minVert;
}

Coord MapCamera2d::getBoundsCorrectedCoords(const Coord &coords) {
    Coord mapCoords = mapInterface->getCoordinateConverterHelper()->convert(mapCoordinateSystem.identifier, coords);

    auto const &paddingCorrectedBounds = getPaddingCorrectedBounds();

    double minHor = std::min(paddingCorrectedBounds.topLeft.x, paddingCorrectedBounds.bottomRight.x);
    double maxHor = std::max(paddingCorrectedBounds.topLeft.x, paddingCorrectedBounds.bottomRight.x);
    double minVert = std::min(paddingCorrectedBounds.topLeft.y, paddingCorrectedBounds.bottomRight.y);
    double maxVert = std::max(paddingCorrectedBounds.topLeft.y, paddingCorrectedBounds.bottomRight.y);

    mapCoords.x = std::clamp(mapCoords.x, minHor, maxHor);
    mapCoords.y = std::clamp(mapCoords.y, minVert, maxVert);

    return mapCoords;
}

Coord MapCamera2d::adjustCoordForPadding(const Coord &coords, double targetZoom) {
    Coord coordinates = coords;

    auto adjustedZoom = std::clamp(targetZoom, zoomMax, zoomMin);

    Vec2D padVec = Vec2D(0.5 * (paddingRight - paddingLeft) * screenPixelAsRealMeterFactor * adjustedZoom,
                         0.5 * (paddingTop - paddingBottom) * screenPixelAsRealMeterFactor * adjustedZoom);
    Vec2D rotPadVec = Vec2DHelper::rotate(padVec, Vec2D(0.0, 0.0), angle);
    coordinates.x += rotPadVec.x;
    coordinates.y += rotPadVec.y;

    return coordinates;
}

RectCoord MapCamera2d::getPaddingCorrectedBounds() {
    double const factor = screenPixelAsRealMeterFactor * zoom;

    double const addRight = (mapSystemRtl ? 1.0 : -1.0) * paddingRight * factor;
    double const addLeft = (mapSystemRtl ? -1.0 : 1.0) * paddingLeft * factor;
    double const addTop = (mapSystemTtb ? -1.0 : 1.0) * paddingTop * factor;
    double const addBottom = (mapSystemTtb ? 1.0 : -1.0) * paddingBottom * factor;

    // new top left and bottom right
    const auto &id = bounds.topLeft.systemIdentifier;
    Coord tl = Coord(id, bounds.topLeft.x + addLeft, bounds.topLeft.y + addTop, bounds.topLeft.z);
    Coord br = Coord(id, bounds.bottomRight.x + addRight, bounds.bottomRight.y + addBottom, bounds.bottomRight.z);

    return RectCoord(tl, br);
}

void MapCamera2d::clampCenterToPaddingCorrectedBounds() {
    return; // TODO

    auto const &paddingCorrectedBounds = getPaddingCorrectedBounds();

    focusPointPosition.x =
        std::clamp(focusPointPosition.x, std::min(paddingCorrectedBounds.topLeft.x, paddingCorrectedBounds.bottomRight.x),
                   std::max(paddingCorrectedBounds.topLeft.x, paddingCorrectedBounds.bottomRight.x));
    focusPointPosition.y =
        std::clamp(focusPointPosition.y, std::min(paddingCorrectedBounds.topLeft.y, paddingCorrectedBounds.bottomRight.y),
                   std::max(paddingCorrectedBounds.topLeft.y, paddingCorrectedBounds.bottomRight.y));

    focusPointPosition = conversionHelper->convert(CoordinateSystemIdentifiers::EPSG4326(), focusPointPosition);
}

void MapCamera2d::setRotationEnabled(bool enabled) { config.rotationEnabled = enabled; }

void MapCamera2d::setSnapToNorthEnabled(bool enabled) { config.snapToNorthEnabled = enabled; }

float MapCamera2d::getScreenDensityPpi() { return screenDensityPpi; }
