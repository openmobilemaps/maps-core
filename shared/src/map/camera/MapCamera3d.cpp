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

#define GLOBE_MIN_ZOOM      200'000'000
#define GLOBE_INITIAL_ZOOM   46'000'000
#define GLOBE_RESET_ZOOM     25'000'000
#define GLOBE_MAX_ZOOM       5'000'000
#define LOCAL_MIN_ZOOM       20'000'000
#define LOCAL_INITIAL_ZOOM    3'000'000
#define LOCAL_MAX_ZOOM          100'000
#define RUBBER_BAND_WINDOW    0 // Disabled for now

MapCamera3d::MapCamera3d(const std::shared_ptr<MapInterface> &mapInterface, float screenDensityPpi)
    : mapInterface(mapInterface)
    , conversionHelper(mapInterface->getCoordinateConverterHelper())
    , mapCoordinateSystem(mapInterface->getMapConfig().mapCoordinateSystem)
    , screenDensityPpi(screenDensityPpi)
    , screenPixelAsRealMeterFactor(0.0254 / screenDensityPpi * mapCoordinateSystem.unitToScreenMeterFactor),
      focusPointPosition(CoordinateSystemIdentifiers::EPSG4326(), 0, 0, 0),
      cameraPitch(0),
      zoomMin(GLOBE_MIN_ZOOM),
      zoomMax(GLOBE_MAX_ZOOM),
      mode(CameraMode3d::GLOBAL),
      lastOnTouchDownPoint(std::nullopt)
    , bounds(mapCoordinateSystem.bounds) {
    mapSystemRtl = mapCoordinateSystem.bounds.bottomRight.x > mapCoordinateSystem.bounds.topLeft.x;
    mapSystemTtb = mapCoordinateSystem.bounds.bottomRight.y > mapCoordinateSystem.bounds.topLeft.y;
    updateZoom(GLOBE_INITIAL_ZOOM);
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
        if (pitchAnimation)
            pitchAnimation->cancel();
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
        if (pitchAnimation)
            std::static_pointer_cast<AnimationInterface>(pitchAnimation)->update();
        if (verticalDisplacementAnimation)
            std::static_pointer_cast<AnimationInterface>(verticalDisplacementAnimation)->update();
    }

    if (mode == CameraMode3d::ONBOARDING_ROTATING_GLOBE || mode == CameraMode3d::ONBOARDING_ROTATING_SEMI_GLOBE || mode == CameraMode3d::ONBOARDING_CLOSE_ORBITAL) {
        focusPointPosition.y = 42;
        focusPointPosition.x = fmod(DateHelper::currentTimeMicros() * 0.000003 + 180.0, 360.0) - 180.0;
        mapInterface->invalidate();
    }

    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();

    double fovy = getCameraFieldOfView();
    double vpr = (double) sizeViewport.x / (double) sizeViewport.y;
    // RectCoord viewBounds = getRectFromViewport(sizeViewport, focusPointPosition);

    const auto [newVpMatrix, newViewMatrix, newProjectionMatrix, newInverseMatrix] = getVpMatrix(focusPointPosition, true);

    return newVpMatrix;
}

std::tuple<std::vector<float>, std::vector<float>, std::vector<float>, std::vector<double>> MapCamera3d::getVpMatrix(const Coord &focusCoord, bool updateVariables) {
    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();

    std::vector<float> newViewMatrix(16, 0.0);
    std::vector<float> newProjectionMatrix(16, 0.0);

    const float R = 6378137.0;
    float longitude = focusCoord.x; //  px / R;
    float latitude = focusCoord.y; // 2*atan(exp(py / R)) - 3.1415926 / 2;

    double focusPointAltitude = focusCoord.z;
    double cameraDistance = getCameraDistance();
    double fovy = getCameraFieldOfView(); // 45 // zoom / 70800;
    const double minCameraDistance = 1.05;
    if (cameraDistance < minCameraDistance) {
        double d = minCameraDistance * R;
        double pixelsPerMeter =  this->screenDensityPpi / 0.0254;
        double w = (double)sizeViewport.y;
        fovy = atan((zoom * w / pixelsPerMeter / 2.0) / d) * 2.0 / M_PI * 180.0;
        cameraDistance = minCameraDistance;
    }

    double maxD = cameraDistance + 1.0;
    double minD = cameraDistance - 1.0;

    // aspect ratio
    double vpr = (double) sizeViewport.x / (double) sizeViewport.y;
    if (vpr > 1.0) {
        fovy /= vpr;
    }
    double fovyRad = fovy * M_PI / 180.0;

    // initial perspective projection
    Matrix::perspectiveM(newProjectionMatrix, 0, fovy, vpr, minD, maxD);

    // modify projection
    // translate anchor point based on padding and offset
    // TODO: horizontal translation
    double contentHeight = ((double) sizeViewport.y) - paddingBottom - paddingTop;
    double offsetY = -paddingBottom / 2.0 / (double) sizeViewport.y + cameraVerticalDisplacement * contentHeight * 0.5 / (double) sizeViewport.y;
    offsetY = cameraDistance * tan(fovyRad / 2.0) * offsetY; // view space to world space
    Matrix::translateM(newProjectionMatrix, 0, 0.0, -offsetY, 0);

    // view matrix
    // remember: read from bottom to top
    Matrix::setIdentityM(newViewMatrix, 0);

    Matrix::translateM(newViewMatrix, 0, 0.0, 0, -cameraDistance);
    Matrix::rotateM(newViewMatrix, 0, -cameraPitch, 1.0, 0.0, 0.0);
    Matrix::rotateM(newViewMatrix, 0, -angle, 0.0, 0.0, 1.0);

    Matrix::translateM(newViewMatrix, 0, 0, 0, -1 - focusPointAltitude / R);

    Matrix::rotateM(newViewMatrix, 0.0, latitude, 1.0, 0.0, 0.0);
    Matrix::rotateM(newViewMatrix, 0.0, -longitude, 0.0, 1.0, 0.0);
    Matrix::rotateM(newViewMatrix, 0.0, -90, 0.0, 1.0, 0.0); // zero longitude in London


    std::vector<float> newVpMatrix(16, 0.0);
    Matrix::multiplyMM(newVpMatrix, 0, newProjectionMatrix, 0, newViewMatrix, 0);

    std::vector<double> vpMatrixD = {
            static_cast<double>(newVpMatrix[0]),
            static_cast<double>(newVpMatrix[1]),
            static_cast<double>(newVpMatrix[2]),
            static_cast<double>(newVpMatrix[3]),
            static_cast<double>(newVpMatrix[4]),
            static_cast<double>(newVpMatrix[5]),
            static_cast<double>(newVpMatrix[6]),
            static_cast<double>(newVpMatrix[7]),
            static_cast<double>(newVpMatrix[8]),
            static_cast<double>(newVpMatrix[9]),
            static_cast<double>(newVpMatrix[10]),
            static_cast<double>(newVpMatrix[11]),
            static_cast<double>(newVpMatrix[12]),
            static_cast<double>(newVpMatrix[13]),
            static_cast<double>(newVpMatrix[14]),
            static_cast<double>(newVpMatrix[15])
    };
    std::vector<double> newInverseMatrix(16, 0.0);
    gluInvertMatrix(vpMatrixD, newInverseMatrix);

    

    if (updateVariables) {
        std::lock_guard<std::recursive_mutex> lock(vpDataMutex);
        double currentRotation = angle;
        double currentZoom = zoom;
        lastVpRotation = currentRotation;
        lastVpZoom = currentZoom;
        vpMatrix = newVpMatrix;
        inverseVPMatrix = newInverseMatrix;
        viewMatrix = newViewMatrix;
        projectionMatrix = newProjectionMatrix;
        verticalFov = fovy;
        horizontalFov = fovy * vpr;
        validVpMatrix = true;
    }
    return std::make_tuple(newVpMatrix, newViewMatrix, newProjectionMatrix, newInverseMatrix);
}


// Funktion zur Berechnung der Koeffizienten der projizierten Ellipse
std::vector<double> MapCamera3d::computeEllipseCoefficients() {
    return inverseVPMatrix;
}

std::optional<std::vector<float>> MapCamera3d::getLastVpMatrix() {
    // TODO: Add back as soon as visiblerect calculation is done
//    if (!lastVpBounds) {
//        return std::nullopt;
//    }
    std::lock_guard<std::recursive_mutex> lock(vpDataMutex);
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
    // TODO: Implement for Camera3D
//    printf("Warning: getPaddingAdjustedVisibleRect incomplete logic.\n");

    return RectCoord(Coord(3857, 0, 0, 0), Coord(3857, 0, 0, 0));
//    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
//
//    // adjust viewport
//    sizeViewport.x -= (paddingLeft + paddingRight);
//    sizeViewport.y -= (paddingTop + paddingBottom);
//
//    // also use the padding adjusted center position
//    return getRectFromViewport(sizeViewport, getCenterPosition());
}

RectCoord MapCamera3d::getRectFromViewport(const Vec2I &sizeViewport, const Coord &center) {
    // TODO: Implement for Camera3D
//    printf("Warning: getRectFromViewport incomplete logic.\n");
    return RectCoord(Coord(3857, 0, 0, 0), Coord(3857, 0, 0, 0));;
//    double zoomFactor = screenPixelAsRealMeterFactor * zoom;
//
//    double halfWidth = sizeViewport.x * 0.5 * zoomFactor;
//    double halfHeight = sizeViewport.y * 0.5 * zoomFactor;
//
//    double sinAngle = sin(angle * M_PI / 180.0);
//    double cosAngle = cos(angle * M_PI / 180.0);
//
//    double deltaX = std::abs(halfWidth * cosAngle) + std::abs(halfHeight * sinAngle);
//    double deltaY = std::abs(halfWidth * sinAngle) + std::abs(halfHeight * cosAngle);
//
//    double topLeftX = center.x - deltaX;
//    double topLeftY = center.y + deltaY;
//    double bottomRightX = center.x + deltaX;
//    double bottomRightY = center.y - deltaY;
//
//    Coord topLeft = Coord(center.systemIdentifier, topLeftX, topLeftY, center.z);
//    Coord bottomRight = Coord(center.systemIdentifier, bottomRightX, bottomRightY, center.z);
//    return RectCoord(topLeft, bottomRight);
}

void MapCamera3d::notifyListeners(const int &listenerType) {
    // TODO: Add back bounds listener as soon as visibleRect is implemented correctly
    std::optional<RectCoord> visibleRect =
        (listenerType & ListenerType::BOUNDS) ? std::optional<RectCoord>(getVisibleRect()) : std::nullopt;

//    std::optional<RectCoord> visibleRect = std::nullopt;

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
            focusPointPosition = this->focusPointPosition;
        }

        Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
        width = sizeViewport.x;
        height = sizeViewport.y;
    }

    std::lock_guard<std::recursive_mutex> lock(listenerMutex);
    for (auto listener : listeners) {
        if (listenerType & (ListenerType::BOUNDS | ListenerType::CAMERA_MODE)) {
            listener->onCameraChange(viewMatrix, projectionMatrix, verticalFov, horizontalFov, width, height, focusPointAltitude, getCenterPosition(), getZoom(), getCameraMode());
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
        lastOnTouchDownFocusCoord = std::nullopt;
        lastOnTouchDownInverseVPMatrix.clear();
        return false;
    }
    else {
        lastOnTouchDownPoint = posScreen;
        initialTouchDownPoint = posScreen;
        lastOnTouchDownFocusCoord = focusPointPosition;
#ifdef ANDROID
        {
            std::lock_guard<std::recursive_mutex> lock(vpDataMutex);
            const auto [zeroVPMatrix, zeroViewMatrix, zeroProjectionMatrix, zeroInverseVPMatrix] = getVpMatrix(
                    Coord(CoordinateSystemIdentifiers::EPSG4326(), 0.0, 0.0, lastOnTouchDownFocusCoord->z), false);
            lastOnTouchDownInverseVPMatrix = zeroInverseVPMatrix;
        }
        lastOnTouchDownCoord = coordFromScreenPosition(lastOnTouchDownInverseVPMatrix, posScreen);
        lastOnMoveCoord = lastOnTouchDownCoord;
#else
        lastOnTouchDownCoord = coordFromScreenPosition(posScreen);
#endif
        return true;
    }
}

bool MapCamera3d::onMove(const Vec2F &deltaScreen, bool confirmed, bool doubleClick) {
    if (!config.moveEnabled || cameraFrozen || (lastOnTouchDownPoint == std::nullopt) || !lastOnTouchDownCoord || !lastOnTouchDownFocusCoord) {
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
#ifdef ANDROID
    // TOOD: Current wip solution for more stable rotation on Android
    auto newTouchDownCoord = coordFromScreenPosition(lastOnTouchDownInverseVPMatrix, newScreenPos);
#else
    auto newTouchDownCoord = coordFromScreenPosition(newScreenPos);
#endif

    if (newTouchDownCoord.systemIdentifier == -1) {
        return false;
    }

    double dx = -(newTouchDownCoord.x - lastOnTouchDownCoord->x);
    double dy = -(newTouchDownCoord.y - lastOnTouchDownCoord->y);

#ifdef ANDROID
    focusPointPosition.x = lastOnTouchDownFocusCoord->x + dx;
    focusPointPosition.y = lastOnTouchDownFocusCoord->y + dy;

    auto moveCoordDX = 0.0;
    auto moveCoordDY = 0.0;
    if (lastOnMoveCoord) {
        moveCoordDX = -(newTouchDownCoord.x - lastOnMoveCoord->x);
        moveCoordDY = -(newTouchDownCoord.y - lastOnMoveCoord->y);
    }
    lastOnMoveCoord = newTouchDownCoord;
#else
    focusPointPosition.x = focusPointPosition.x + dx;
    focusPointPosition.y = focusPointPosition.y + dy;
#endif

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
#ifdef ANDROID
        currentDragVelocity.x = (1 - averageFactor) * currentDragVelocity.x + averageFactor * moveCoordDX / (deltaMcs / 16000.0);
        currentDragVelocity.y = (1 - averageFactor) * currentDragVelocity.y + averageFactor * moveCoordDY / (deltaMcs / 16000.0);
#else
        currentDragVelocity.x = (1 - averageFactor) * currentDragVelocity.x + averageFactor * dx / (deltaMcs / 16000.0);
        currentDragVelocity.y = (1 - averageFactor) * currentDragVelocity.y + averageFactor * dy / (deltaMcs / 16000.0);
#endif
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

bool MapCamera3d::onOneFingerDoubleClickMoveComplete() {
    if (cameraFrozen)
        return false;
    
    checkForRubberBandEffect();

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

        double scaleFactor = Vec2FHelper::distance(posScreenNew[0], posScreenNew[1]) / Vec2FHelper::distance(posScreenOld[0], posScreenOld[1]);

        double newZoom = zoom / scaleFactor;

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


        const auto [adjPosition, adjZoom] = getBoundsCorrectedCoords(focusPointPosition, zoom);
        focusPointPosition = adjPosition;
        updateZoom(adjZoom);

        auto listenerType = ListenerType::BOUNDS | ListenerType::MAP_INTERACTION;
        notifyListeners(listenerType);
        mapInterface->invalidate();
    }
    return true;
}

bool MapCamera3d::onTwoFingerMoveComplete() {
    checkForRubberBandEffect();

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
    std::lock_guard<std::recursive_mutex> lock(vpDataMutex);
    return coordFromScreenPosition(inverseVPMatrix, posScreen);
}

Coord MapCamera3d::coordFromScreenPosition(const std::vector<double> &inverseVPMatrix, const ::Vec2F &posScreen) {
    auto viewport = mapInterface->getRenderingContext()->getViewportSize();

    std::vector<double> worldPosFrontVec = {
        (posScreen.x / (double)viewport.x * 2.0 - 1),
        -(posScreen.y / (double)viewport.y * 2.0 - 1),
        -1,
        1
    };
    std::vector<double> worldPosBackVec = {
        (posScreen.x / (double)viewport.x * 2.0 - 1),
        -(posScreen.y / (double)viewport.y * 2.0 - 1),
        1,
        1
    };

    worldPosFrontVec = MatrixD::multiply(inverseVPMatrix, worldPosFrontVec);
    Vec3D worldPosFront{worldPosFrontVec[0] / worldPosFrontVec[3], worldPosFrontVec[1] / worldPosFrontVec[3],
                                worldPosFrontVec[2] / worldPosFrontVec[3]};
    worldPosBackVec = MatrixD::multiply(inverseVPMatrix, worldPosBackVec);
    Vec3D worldPosBack{worldPosBackVec[0] / worldPosBackVec[3], worldPosBackVec[1] / worldPosBackVec[3],
                               worldPosBackVec[2] / worldPosBackVec[3]};

    bool didHit = false;
    auto point = MapCamera3DHelper::raySphereIntersection(worldPosFront, worldPosBack, Vec3D(0.0, 0.0, 0.0), 1.0, didHit);

    if (didHit) {
        float longitude = std::atan2(point.x, point.z) * 180 / M_PI - 90;
        if (longitude < -180) {
            longitude += 360;
        }
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
    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
    if (validVpMatrix && sizeViewport.x != 0 && sizeViewport.y != 0) {
        auto coordCartesian = convertToCartesianCoordinates(coord);
        auto projected = projectedPoint(coordCartesian);

        // Map from [-1, 1] to screenPixels, with (0,0) being the top left corner
        double screenXDiffToCenter = projected[0] * sizeViewport.x / 2.0;
        double screenYDiffToCenter = projected[1] * sizeViewport.y / 2.0;

        double posScreenX = screenXDiffToCenter + ((double)sizeViewport.x / 2.0);
        double posScreenY = ((double)sizeViewport.y / 2.0) - screenYDiffToCenter;

        return Vec2F(posScreenX, posScreenY);
    }

    return Vec2F(0.0, 0.0);
}

// padding in percentage, where 1.0 = rect is half of full width and height
bool MapCamera3d::coordIsVisibleOnScreen(const ::Coord & coord, float paddingPc) {
    // 1. Check that coordinate is not on the back of the globe
    if (coordIsOnFrontHalfOfGlobe(coord) == false) {
        return false;
    }

    // 2. Check that coordinate is in bounds of viewport
    auto screenPos = screenPosFromCoord(coord);

    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
    auto minX = (sizeViewport.x + paddingLeft) * paddingPc * 0.5;
    auto maxX = (sizeViewport.x - paddingRight) - minX;
    auto minY = (sizeViewport.y + paddingTop) * paddingPc * 0.5;
    auto maxY = (sizeViewport.y - paddingBottom) - minY;

    if (sizeViewport.x != 0 && sizeViewport.y != 0) {
        return screenPos.x >= minX && screenPos.x <= maxX && screenPos.y >= minY && screenPos.y <= maxY;
    } else {
        return false;
    }
}

bool MapCamera3d::coordIsOnFrontHalfOfGlobe(Coord coord) {
    // Coordinate is on front half of the globe if the projected z-value is in front
    // of the projected z-value of the globe center

    auto coordCartesian = convertToCartesianCoordinates(coord);
    auto projectedCoord = projectedPoint(coordCartesian);

    auto projectedCenter = projectedPoint({0,0,0,1});

    bool isInFront = projectedCoord[2] <= projectedCenter[2];

    return isInFront;
}

std::vector<float> MapCamera3d::convertToCartesianCoordinates(Coord coord) {
    Coord renderCoord = conversionHelper->convertToRenderSystem(coord);

    return {(float) (renderCoord.z * sin(renderCoord.y) * cos(renderCoord.x)),
        (float) (renderCoord.z * cos(renderCoord.y)),
        (float) (-renderCoord.z * sin(renderCoord.y) * sin(renderCoord.x)),
        1.0};
}


// Point given in cartesian coordinates, where (0,0,0) is the center of the globe
std::vector<float> MapCamera3d::projectedPoint(std::vector<float> point) {
    auto projected = Matrix::multiply(vpMatrix, point);
    projected[0] /= projected[3]; // percentage in x direction in [-1, 1], 0 being the center of the screen)
    projected[1] /= projected[3]; // percentage in y direction in [-1, 1], 0 being the center of the screen)
    projected[2] /= projected[3]; // percentage in z direction in [-1, 1], 0 being the center of the screen)
    projected[3] /= projected[3];

    return projected;
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


void MapCamera3d::checkForRubberBandEffect() {
    double diff;
    if (getCameraMode() == CameraMode3d::LOCAL) {
        diff = zoom - zoomMin;
    } else {
        diff = zoomMax - zoom;
    }

    if (diff > 0) {
        auto factor = diff / RUBBER_BAND_WINDOW;
        if (factor > 0.55) {
            if (getCameraMode() == CameraMode3d::LOCAL) {
                // Check if we are overzooming, and if overzooming enough, we change to global mode
                setCameraMode(CameraMode3d::GLOBAL);
            } else {
                // Check if we are overzooming, and if overzooming enough, we change to local mode
                setCameraMode(CameraMode3d::LOCAL);
            }
        } else {
            // Reset zoom
            setZoom(LOCAL_MIN_ZOOM, true);
        }
    }
}

CameraMode3d MapCamera3d::getCameraMode() {
    return this->mode;
}

void MapCamera3d::setCameraMode(CameraMode3d mode) {
    if (mode == this->mode) {
        return;
    }
    this->mode = mode;

    float initialZoom = zoom;
    float targetZoom;
    float initialPitch = cameraPitch;
    float initialVerticalDisplacement = cameraVerticalDisplacement;

    float duration = DEFAULT_ANIM_LENGTH;

    std::optional<Coord> targetCoordinate;

    switch (mode) {
        case CameraMode3d::ONBOARDING_ROTATING_GLOBE:
            this->zoomMin = GLOBE_MIN_ZOOM;
            this->zoomMax = GLOBE_MIN_ZOOM;
            targetZoom = GLOBE_MIN_ZOOM;
            duration = 0;
            break;
        case CameraMode3d::ONBOARDING_ROTATING_SEMI_GLOBE:
            this->zoomMin = 100'000'000;
            this->zoomMax = 100'000'000;
            targetZoom = 100'000'000;
            duration = 2000;
            break;
        case CameraMode3d::ONBOARDING_CLOSE_ORBITAL:
            this->zoomMin = 70'000'000;
            this->zoomMax = 70'000'000;
            targetZoom = 70'000'000;
            duration = 2000;
            break;
        case CameraMode3d::ONBOARDING_FOCUS_ZURICH:
            this->zoomMin = LOCAL_MIN_ZOOM;
            this->zoomMax = LOCAL_MIN_ZOOM;
            targetZoom = LOCAL_MIN_ZOOM;
            duration = 4000;
            targetCoordinate = Coord(CoordinateSystemIdentifiers::EPSG4326(), 8.5417, 47.3769, 0.0);
            break;
        case CameraMode3d::GLOBAL:
            this->zoomMin = GLOBE_MIN_ZOOM;
            this->zoomMax = GLOBE_MAX_ZOOM;
            targetZoom = GLOBE_RESET_ZOOM;
            break;

        case CameraMode3d::LOCAL:
            this->zoomMin = LOCAL_MIN_ZOOM;
            this->zoomMax = LOCAL_MAX_ZOOM;
            if (this->zoom == this->zoomMin) {
                targetZoom = LOCAL_MIN_ZOOM;
            } else {
                targetZoom = LOCAL_INITIAL_ZOOM;
            }
            break;
    }

    // temporarily set target zoom to get target pitch
    this->zoom = targetZoom;
    float targetPitch = getCameraPitch();
    float targetVerticalDisplacement = getCameraVerticalDisplacement();

    std::lock_guard<std::recursive_mutex> lock(animationMutex);

    zoomAnimation = std::make_shared<DoubleAnimation>(duration, initialZoom, targetZoom, InterpolatorFunction::EaseInOut,
                                                      [=](double zoom) {
                                                          this->zoom = zoom;
                                                          mapInterface->invalidate();
                                                      },
                                                       [=] {
                                                           this->zoom = targetZoom;
                                                           this->zoomAnimation = nullptr;
                                                       });
    zoomAnimation->start();

    pitchAnimation = std::make_shared<DoubleAnimation>(
                                                       duration, initialPitch, targetPitch, InterpolatorFunction::EaseInOut,
                                                       [=](double pitch) {
                                                           this->cameraPitch = pitch;
                                                           mapInterface->invalidate();
                                                       },
                                                          [=] {
                                                              this->cameraPitch = targetPitch;
                                                              this->pitchAnimation = nullptr;
                                                          });
    pitchAnimation->start();

    verticalDisplacementAnimation = std::make_shared<DoubleAnimation>(
                                                       duration, initialVerticalDisplacement, targetVerticalDisplacement, InterpolatorFunction::EaseInOut,
                                                       [=](double dis) {
                                                           this->cameraVerticalDisplacement = dis;
                                                           mapInterface->invalidate();
                                                       },
                                                       [=] {
                                                           this->cameraVerticalDisplacement = targetVerticalDisplacement;
                                                           this->verticalDisplacementAnimation = nullptr;
                                                       });
    verticalDisplacementAnimation->start();

    if (targetCoordinate) {
        Coord startPosition = mapInterface->getCoordinateConverterHelper()->convert(CoordinateSystemIdentifiers::EPSG4326(), focusPointPosition);

        coordAnimation = std::make_shared<CoordAnimation>(
                                                          duration, startPosition, *targetCoordinate, std::nullopt, InterpolatorFunction::EaseInOut,
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
    }


    mapInterface->invalidate();

    notifyListeners(ListenerType::CAMERA_MODE);
}

void MapCamera3d::notifyListenerBoundsChange() {
    notifyListeners(ListenerType::BOUNDS);
}

void MapCamera3d::updateZoom(double zoom_) {
    auto zoomMin = getMinZoom();
    auto zoomMax = getMaxZoom();

    double overZooming;
    if (getCameraMode() == CameraMode3d::LOCAL) {
        overZooming = zoom_ - LOCAL_MIN_ZOOM;
    } else {
        overZooming = GLOBE_MAX_ZOOM - zoom_;
    }

    double newZoom = 0;

    if (overZooming > 0) {
        double normalizedDiff = std::min(overZooming / RUBBER_BAND_WINDOW, 1.0); // Normalize to [0, 1]
        double mapped = 1 / (1 + normalizedDiff) * 2 - 1; // Rubberband so that medium to large values are all squashed towards the end

        if (getCameraMode() == CameraMode3d::LOCAL) {
            newZoom = zoom + (zoom_ - zoom) * mapped;
        } else {
            newZoom = zoom - (zoom - zoom_) * mapped;
        }
    } else {
        newZoom = std::clamp(zoom_, zoomMax, zoomMin);
    }

    
    zoom = newZoom;

    cameraVerticalDisplacement = getCameraVerticalDisplacement();
    cameraPitch = getCameraPitch();
}

double MapCamera3d::getCameraVerticalDisplacement() {
    if (mode == CameraMode3d::ONBOARDING_ROTATING_GLOBE || mode == CameraMode3d::ONBOARDING_ROTATING_SEMI_GLOBE) {
        return 0;
    }
    double z, from, to;
    double maxPitch = GLOBE_INITIAL_ZOOM;
    if (zoom >= maxPitch) {
        z = 1.0 - (zoom - maxPitch) / (GLOBE_MIN_ZOOM - maxPitch);
        from = 0;
        to = 1.0;
    }
    else {
        z = 1.0 - (zoom - LOCAL_MAX_ZOOM) / (maxPitch - LOCAL_MAX_ZOOM);
        from = 1.0;
        to = 0;
    }
    double p = from + (z * (to - from));
    return p;
}

double MapCamera3d::getCameraPitch() {
    if (mode == CameraMode3d::ONBOARDING_ROTATING_GLOBE || mode == CameraMode3d::ONBOARDING_ROTATING_SEMI_GLOBE) {
        return 0;
    }
    double z, from, to;
    double maxPitch = GLOBE_INITIAL_ZOOM;
    if (zoom >= maxPitch) {
        z = 1.0 - (zoom - maxPitch) / (GLOBE_MIN_ZOOM - maxPitch);
        from = 0;
        to = 25;
    }
    else {
        z = 1.0 - (zoom - LOCAL_MAX_ZOOM) / (maxPitch - LOCAL_MAX_ZOOM);
        from = 25;
        to = 0;
    }
//    switch (mode) {
//        case CameraMode3d::GLOBE:
//            z = 1.0 - (zoom - LOCAL_MIN_ZOOM) / (GLOBE_MIN_ZOOM - LOCAL_MIN_ZOOM);
//            from = 0;
//            to = 40;
//            break;
//
//        case CameraMode3d::TILTED_ORBITAL: // abused as local
//            z = 1.0 - (zoom - LOCAL_MAX_ZOOM) / (LOCAL_MIN_ZOOM - LOCAL_MAX_ZOOM);
//            from = 0;
//            to = 0;
//            break;
//    }   
    double p = from + (z * (to - from));
    return p;
}

double MapCamera3d::getCameraFieldOfView() {
    return 42;
//    fieldOfView -= 0.01;
//    mapInterface->invalidate();
//    return fieldOfView;
}

double MapCamera3d::getCameraDistance() {
    double f = getCameraFieldOfView();
    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
    double w = (double)sizeViewport.y;
    double pixelsPerMeter =  this->screenDensityPpi / 0.0254;
    float d = (zoom * w / pixelsPerMeter / 2.0) / tan(f / 2.0 * M_PI / 180.0);
    float R = 6378137.0;
    return d / R;
}

