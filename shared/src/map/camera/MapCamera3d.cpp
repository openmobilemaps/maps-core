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
#include "Camera3dConfigFactory.h"
#include "Coord.h"
#include "CoordAnimation.h"
#include "CoordHelper.h"
#include "CoordinateSystemIdentifiers.h"
#include "DateHelper.h"
#include "DoubleAnimation.h"
#include "Logger.h"
#include "MapConfig.h"
#include "MapInterface.h"
#include "Matrix.h"
#include "MatrixD.h"
#include "Vec2D.h"
#include "Vec2DHelper.h"
#include "Vec2FHelper.h"
#include "Vec3DHelper.h"
#include "VectorHelper.h"

#include "Camera3dConfig.h"
#include "MapCamera3DHelper.h"

#define ROTATION_THRESHOLD 20
#define ROTATION_LOCKING_ANGLE 10
#define ROTATION_LOCKING_FACTOR 1.5

#define GLOBE_MIN_ZOOM 200'000'000
#define GLOBE_MAX_ZOOM 5'000'000

MapCamera3d::MapCamera3d(const std::shared_ptr<MapInterface> &mapInterface, float screenDensityPpi)
        : mapInterface(mapInterface),
          conversionHelper(mapInterface->getCoordinateConverterHelper()),
          mapCoordinateSystem(mapInterface->getMapConfig().mapCoordinateSystem),
          screenDensityPpi(screenDensityPpi),
          screenPixelAsRealMeterFactor(0.0254 / screenDensityPpi * mapCoordinateSystem.unitToScreenMeterFactor),
          focusPointPosition(CoordinateSystemIdentifiers::EPSG4326(), 0, 0, 0),
          cameraPitch(0),
          zoomMin(GLOBE_MIN_ZOOM),
          zoomMax(GLOBE_MAX_ZOOM),
          lastOnTouchDownPoint(std::nullopt),
          bounds(mapCoordinateSystem.bounds),
          origin(0, 0, 0),
          cameraZoomConfig(Camera3dConfigFactory::getBasicConfig()) {
    mapSystemRtl = mapCoordinateSystem.bounds.bottomRight.x > mapCoordinateSystem.bounds.topLeft.x;
    mapSystemTtb = mapCoordinateSystem.bounds.bottomRight.y > mapCoordinateSystem.bounds.topLeft.y;
    updateZoom(GLOBE_MIN_ZOOM);
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

    updateMatrices();

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
    auto [focusPosition, focusZoom] =
            getBoundsCorrectedCoords(conversionHelper->convert(focusPointPosition.systemIdentifier, centerPosition), zoom);

    if (animated && bounds.topLeft.x == mapCoordinateSystem.bounds.topLeft.x &&
        bounds.bottomRight.x == mapCoordinateSystem.bounds.bottomRight.x) {
        // wrappable on longitude
        if (focusPosition.x - focusPointPosition.x > 180.0) {
            focusPosition.x -= 360.0;
        } else if (focusPosition.x - focusPointPosition.x < -180.0) {
            focusPosition.x += 360.0;
        }
    }

    if (animated) {
        std::weak_ptr<MapCamera3d> weakSelf = std::static_pointer_cast<MapCamera3d>(shared_from_this());
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        coordAnimation = std::make_shared<CoordAnimation>(
                cameraZoomConfig.animationDurationMs, focusPointPosition, focusPosition, centerPosition,
                InterpolatorFunction::EaseInOut,
                [weakSelf](Coord positionMapSystem) {
                    if (auto selfPtr = weakSelf.lock()) {
                        assert(positionMapSystem.systemIdentifier == 4326);
                        std::lock_guard<std::recursive_mutex> lock(selfPtr->paramMutex);
                        selfPtr->focusPointPosition = positionMapSystem;
                        selfPtr->clampCenterToPaddingCorrectedBounds();
                        selfPtr->notifyListeners(ListenerType::BOUNDS);
                        auto mapInterface = selfPtr->mapInterface;
                        if (mapInterface) {
                            mapInterface->invalidate();
                        }
                    }
                },
                [weakSelf] {
                    if (auto selfPtr = weakSelf.lock()) {
                        assert(selfPtr->coordAnimation->endValue.systemIdentifier == 4326);
                        std::lock_guard<std::recursive_mutex> lock(selfPtr->paramMutex);
                        selfPtr->focusPointPosition = selfPtr->coordAnimation->endValue;
                        selfPtr->clampCenterToPaddingCorrectedBounds();
                        selfPtr->notifyListeners(ListenerType::BOUNDS);
                        auto mapInterface = selfPtr->mapInterface;
                        if (mapInterface) {
                            mapInterface->invalidate();
                        }
                        selfPtr->coordAnimation = nullptr;
                    }
                });
        coordAnimation->start();
        setZoom(focusZoom, true);
        mapInterface->invalidate();
    } else {
        assert(focusPosition.systemIdentifier == 4326);
        std::lock_guard<std::recursive_mutex> lock(paramMutex);
        this->focusPointPosition = focusPosition;
        updateZoom(focusZoom);
        validVpMatrix = false;
        notifyListeners(ListenerType::BOUNDS);
        mapInterface->invalidate();
    }
}

void MapCamera3d::moveToCenterPosition(const ::Coord &centerPosition, bool animated) {
    if (cameraFrozen)
        return;
    inertia = std::nullopt;
    auto [focusPosition, focusZoom] =
            getBoundsCorrectedCoords(conversionHelper->convert(focusPointPosition.systemIdentifier, centerPosition), zoom);

    if (animated && bounds.topLeft.x == mapCoordinateSystem.bounds.topLeft.x &&
        bounds.bottomRight.x == mapCoordinateSystem.bounds.bottomRight.x) {
        // wrappable on longitude
        if (focusPosition.x - focusPointPosition.x > 180.0) {
            focusPosition.x -= 360.0;
        } else if (focusPosition.x - focusPointPosition.x < -180.0) {
            focusPosition.x += 360.0;
        }
    }

    if (animated) {
        std::weak_ptr<MapCamera3d> weakSelf = std::static_pointer_cast<MapCamera3d>(shared_from_this());
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        coordAnimation = std::make_shared<CoordAnimation>(
                cameraZoomConfig.animationDurationMs, focusPointPosition, focusPosition, centerPosition,
                InterpolatorFunction::EaseInOut,
                [weakSelf](Coord positionMapSystem) {
                    if (auto selfPtr = weakSelf.lock()) {
                        assert(positionMapSystem.systemIdentifier == 4326);
                        selfPtr->focusPointPosition = positionMapSystem;
                        selfPtr->clampCenterToPaddingCorrectedBounds();
                        selfPtr->notifyListeners(ListenerType::BOUNDS);
                        auto mapInterface = selfPtr->mapInterface;
                        if (mapInterface) {
                            mapInterface->invalidate();
                        }
                    }
                },
                [weakSelf] {
                    if (auto selfPtr = weakSelf.lock()) {
                        assert(selfPtr->coordAnimation->endValue.systemIdentifier == 4326);
                        selfPtr->focusPointPosition = selfPtr->coordAnimation->endValue;
                        selfPtr->clampCenterToPaddingCorrectedBounds();
                        selfPtr->notifyListeners(ListenerType::BOUNDS);
                        auto mapInterface = selfPtr->mapInterface;
                        if (mapInterface) {
                            mapInterface->invalidate();
                        }
                        selfPtr->coordAnimation = nullptr;
                    }
                });
        coordAnimation->start();
        mapInterface->invalidate();
    } else {
        assert(focusPosition.systemIdentifier == 4326);
        this->focusPointPosition = focusPosition;
        validVpMatrix = false;
        notifyListeners(ListenerType::BOUNDS);
        mapInterface->invalidate();
    }
}

void MapCamera3d::moveToBoundingBox(const RectCoord &boundingBox, float paddingPc, bool animated, std::optional<double> minZoom, std::optional<double> maxZoom) {
    if (cameraFrozen) {
        return;
    }

    assert(boundingBox.topLeft.systemIdentifier == 4326);

    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();

    auto distance = calculateDistance(boundingBox.topLeft.y, boundingBox.topLeft.x, boundingBox.bottomRight.y, boundingBox.bottomRight.x);
    auto pFactor = 1.0 + 2.0 * paddingPc;
    distance = Vec2F(distance.x * pFactor, distance.y * pFactor);
    auto zoom = zoomForMeterWidth(sizeViewport, distance);

    // the user of moveToBoundingBox wants that the bounding box is centered,
    // if you have camera verticaldisplacement, this is not the case, this should
    // be fixed for this function as it only works with 0
    assert(valueForZoom(cameraZoomConfig.verticalDisplacementInterpolationValues, zoom) == 0);

    auto x = 0.5 * boundingBox.topLeft.x + 0.5 * boundingBox.bottomRight.x;
    auto y = 0.5 * boundingBox.topLeft.y + 0.5 * boundingBox.bottomRight.y;
    auto z = 0.5 * boundingBox.topLeft.z + 0.5 * boundingBox.bottomRight.z;

    auto center = Coord(boundingBox.topLeft.systemIdentifier, x, y, z);
    moveToCenterPositionZoom(center, zoom, animated);
}

::Coord MapCamera3d::getCenterPosition() {
    assert(focusPointPosition.systemIdentifier == 4326);
    return focusPointPosition;
}

void MapCamera3d::setZoom(double zoom, bool animated) {
    if (cameraFrozen) {
        return;
    }

    double targetZoom = std::clamp(zoom, zoomMax, zoomMin);

    if (animated) {
        std::weak_ptr<MapCamera3d> weakSelf = std::static_pointer_cast<MapCamera3d>(shared_from_this());
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        zoomAnimation = std::make_shared<DoubleAnimation>(
                cameraZoomConfig.animationDurationMs, this->zoom, targetZoom, InterpolatorFunction::EaseIn,
                [weakSelf](double zoom) {
                    if (auto selfPtr = weakSelf.lock()) {
                        selfPtr->setZoom(zoom, false);
                    }
                },
                [weakSelf, targetZoom] {
                    if (auto selfPtr = weakSelf.lock()) {
                        selfPtr->setZoom(targetZoom, false);
                        selfPtr->zoomAnimation = nullptr;
                    }
                });
        zoomAnimation->start();
        mapInterface->invalidate();
    } else {
        updateZoom(zoom);
        validVpMatrix = false;
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
        if (std::abs(currentAngle - newAngle) > std::abs(currentAngle - (newAngle + 360.0))) {
            newAngle += 360.0;
        } else if (std::abs(currentAngle - newAngle) > std::abs(currentAngle - (newAngle - 360.0))) {
            newAngle -= 360.0;
        }
        std::weak_ptr<MapCamera3d> weakSelf = std::static_pointer_cast<MapCamera3d>(shared_from_this());
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        rotationAnimation = std::make_shared<DoubleAnimation>(
                cameraZoomConfig.animationDurationMs, currentAngle, newAngle, InterpolatorFunction::Linear,
                [weakSelf](double angle) {
                    if (auto selfPtr = weakSelf.lock()) {
                        selfPtr->setRotation(angle, false);
                    }
                },
                [weakSelf, newAngle] {
                    if (auto selfPtr = weakSelf.lock()) {
                        selfPtr->setRotation(newAngle, false);
                        selfPtr->rotationAnimation = nullptr;
                    }
                });
        rotationAnimation->start();
        mapInterface->invalidate();
    } else {
        std::lock_guard<std::recursive_mutex> lock(paramMutex);
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
    std::lock_guard<std::recursive_mutex> lock(matrixMutex);
    return VectorHelper::clone(vpMatrix);
}

void MapCamera3d::updateMatrices() {
    std::lock_guard<std::recursive_mutex> lock(paramMutex);
    computeMatrices(focusPointPosition, false);
}

std::optional<std::tuple<std::vector<double>, std::vector<double>, Vec3D>> MapCamera3d::computeMatrices(const Coord &focusCoord, bool onlyReturnResult) {
    std::lock_guard<std::recursive_mutex> lock(paramMutex);

    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();

    std::vector<double> newViewMatrix(16, 0.0);
    std::vector<double> newProjectionMatrix(16, 0.0);

    const float R = 6378137.0;
    const double longitude = focusCoord.x; //  px / R;
    const double latitude = focusCoord.y;  // 2*atan(exp(py / R)) - 3.1415926 / 2;

    const double focusPointAltitude = focusCoord.z;
    double cameraDistance = getCameraDistance(sizeViewport, zoom);
    double fovy = getCameraFieldOfView(); // 45 // zoom / 70800;
    const double minCameraDistance = 1.05;
    if (cameraDistance < minCameraDistance) {
        double d = minCameraDistance * R;
        double pixelsPerMeter = this->screenDensityPpi / 0.0254;
        double w = (double) sizeViewport.y;
        fovy = atan2(zoom * w / pixelsPerMeter / 2.0, d) * 2.0 / M_PI * 180.0;

        cameraDistance = minCameraDistance;
    }

    const double maxD = cameraDistance + 1.0;
    const double minD = cameraDistance - 1.0;

    // aspect ratio
    const double vpr = (double) sizeViewport.x / (double) sizeViewport.y;
    if (vpr > 1.0) {
        fovy /= vpr;
    }
    const double fovyRad = fovy * M_PI / 180.0;

    // initial perspective projection
    MatrixD::perspectiveM(newProjectionMatrix, 0, fovy, vpr, minD, maxD);

    //    MatrixD::setIdentityM(newProjectionMatrix, 0);

    // modify projection
    // translate anchor point based on padding and vertical displacement
    const double relPaddingOffsetX = (paddingLeft - paddingRight) / 2.0;
    const double relPaddingOffsetY = (paddingBottom - paddingTop) / 2.0;
    const double relDisplacementOffsetY = -cameraVerticalDisplacement * ((sizeViewport.y - paddingBottom - paddingTop) / 2.0);
    const double relTotalOffsetY = relPaddingOffsetY + relDisplacementOffsetY;

    const double viewportXHalf = sizeViewport.x / 2.0; // Clip space is [-1.0, 1.0]
    const double viewportYHalf = sizeViewport.y / 2.0;
    MatrixD::mTranslated(newProjectionMatrix, 0, relPaddingOffsetX / viewportXHalf, relTotalOffsetY / viewportYHalf, 0);

    // view matrix
    // remember: read from bottom to top as camera movement relative to fixed globe
    //           read from top to bottom as vertex movement relative to fixed camera
    MatrixD::setIdentityM(newViewMatrix, 0);

    MatrixD::translateM(newViewMatrix, 0, 0.0, 0, -cameraDistance);
    MatrixD::rotateM(newViewMatrix, 0, -cameraPitch, 1.0, 0.0, 0.0);
    MatrixD::rotateM(newViewMatrix, 0, -angle, 0.0, 0.0, 1.0);

    MatrixD::translateM(newViewMatrix, 0, 0, 0, -1 - focusPointAltitude / R);

    MatrixD::rotateM(newViewMatrix, 0.0, latitude, 1.0, 0.0, 0.0);
    MatrixD::rotateM(newViewMatrix, 0.0, -longitude, 0.0, 1.0, 0.0);
    MatrixD::rotateM(newViewMatrix, 0.0, -90, 0.0, 1.0, 0.0); // zero longitude in London

    const double lo = (longitude - 180.0) * M_PI / 180.0; // [-2 * pi, 0) X
    const double la = (latitude - 90.0) * M_PI / 180.0;   // [0, -pi] Y
    const double x = (1.0 * sin(la) * cos(lo));
    const double y = (1.0 * cos(la));
    const double z = -(1.0 * sin(la) * sin(lo));

    const Vec3D newOrigin = Vec3D(x, y, z);

    MatrixD::translateM(newViewMatrix, 0, x, y, z);

    std::vector<double> newVpMatrix(16, 0.0);
    MatrixD::multiplyMM(newVpMatrix, 0, newProjectionMatrix, 0, newViewMatrix, 0);

    std::vector<double> newInverseMatrix(16, 0.0);
    gluInvertMatrix(newVpMatrix, newInverseMatrix);

    std::vector<float> newVpMatrixF = VectorHelper::convertToFloat(newVpMatrix);
    std::vector<float> newProjectionMatrixF = VectorHelper::convertToFloat(newProjectionMatrix);
    std::vector<float> newViewMatrixF = VectorHelper::convertToFloat(newViewMatrix);

    if (onlyReturnResult) {
        return std::tuple{newVpMatrix, newInverseMatrix, newOrigin};
    } else {
        std::lock_guard<std::recursive_mutex> writeLock(matrixMutex);
        lastVpRotation = angle;
        lastVpZoom = zoom;
        vpMatrix = newVpMatrixF;
        vpMatrixD = newVpMatrix;
        inverseVPMatrix = newInverseMatrix;
        viewMatrix = newViewMatrixF;
        projectionMatrix = newProjectionMatrixF;
        verticalFov = fovy;
        horizontalFov = fovy * vpr;
        validVpMatrix = true;
        origin = newOrigin;
        lastScalingFactor = mapUnitsFromPixels(1.0);

        return std::nullopt;
    }
}

Vec3D MapCamera3d::getOrigin() {
    std::lock_guard<std::recursive_mutex> writeLock(matrixMutex);
    return origin;
}

// Funktion zur Berechnung der Koeffizienten der projizierten Ellipse
void MapCamera3d::computeEllipseCoefficients(std::vector<double> &coefficients) {
    std::vector<double> tmp = VectorHelper::clone(vpMatrixD);
    MatrixD::translateM(tmp, 0, -origin.x, -origin.y, -origin.z);
    gluInvertMatrix(tmp, coefficients);
}

std::optional<std::vector<double>> MapCamera3d::getLastVpMatrixD() {
    std::lock_guard<std::recursive_mutex> lock(matrixMutex);
    return VectorHelper::clone(vpMatrixD);
}

std::optional<std::vector<float>> MapCamera3d::getLastVpMatrix() {
    // TODO: Add back as soon as visiblerect calculation is done
    //    if (!lastVpBounds) {
    //        return std::nullopt;
    //    }
    std::lock_guard<std::recursive_mutex> lock(matrixMutex);
    std::vector<float> vpCopy;
    std::copy(vpMatrix.begin(), vpMatrix.end(), std::back_inserter(vpCopy));
    return vpCopy;
}

std::optional<::RectCoord> MapCamera3d::getLastVpMatrixViewBounds() {
    std::lock_guard<std::recursive_mutex> lock(matrixMutex);
    return lastVpBounds;
}

std::optional<float> MapCamera3d::getLastVpMatrixRotation() {
    std::lock_guard<std::recursive_mutex> lock(matrixMutex);
    return lastVpRotation;
}

std::optional<float> MapCamera3d::getLastVpMatrixZoom() {
    std::lock_guard<std::recursive_mutex> lock(matrixMutex);
    return lastVpZoom;
}

/** this method is called just before the update methods on all layers */
void MapCamera3d::update() {
    inertiaStep();
    if (cameraZoomConfig.rotationSpeed) {
        std::lock_guard<std::recursive_mutex> lock(paramMutex);
        double speed = *(cameraZoomConfig.rotationSpeed);
        focusPointPosition.x = fmod(DateHelper::currentTimeMicros() * speed * 0.000003 + 180.0, 360.0) - 180.0;
        mapInterface->invalidate();
    }
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
    updateMatrices();
}

std::vector<float> MapCamera3d::getInvariantModelMatrix(const ::Coord &coordinate, bool scaleInvariant, bool rotationInvariant) {
    Coord renderCoord = conversionHelper->convertToRenderSystem(coordinate);
    std::vector<float> newMatrix(16, 0);

    Matrix::setIdentityM(newMatrix, 0);
    Matrix::translateM(newMatrix, 0, renderCoord.x, renderCoord.y, renderCoord.z);

    if (scaleInvariant) {
        double zoomFactor = getScalingFactor();
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
    std::lock_guard<std::recursive_mutex> lock(paramMutex);
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
    return RectCoord(Coord(3857, 0, 0, 0), Coord(3857, 0, 0, 0));
    ;
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
            std::lock_guard<std::recursive_mutex> lock(matrixMutex);
            validVpMatrix = this->validVpMatrix;
        }
        if (!validVpMatrix) {
            updateMatrices(); // update matrices
        }
        {
            std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(matrixMutex, paramMutex);

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
    for (auto listener: listeners) {
        if (listenerType & (ListenerType::BOUNDS)) {

            std::vector<float> viewMatrixF = VectorHelper::clone(viewMatrix);
            std::vector<float> projectionMatrixF = VectorHelper::clone(projectionMatrix);

            listener->onCameraChange(viewMatrixF, projectionMatrixF, origin, verticalFov, horizontalFov, width, height,
                                     focusPointAltitude, getCenterPosition(), getZoom());
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
    } else {
        std::lock_guard<std::recursive_mutex> lock(paramMutex);
        lastOnTouchDownPoint = posScreen;
        initialTouchDownPoint = posScreen;
        lastOnTouchDownFocusCoord = focusPointPosition;
#ifdef ANDROID
        {
            const auto [zeroVPMatrix, zeroInverseVPMatrix, zeroOrigin] =
                    *computeMatrices(Coord(CoordinateSystemIdentifiers::EPSG4326(), 0.0, 0.0, lastOnTouchDownFocusCoord->z), true);
            lastOnTouchDownInverseVPMatrix = zeroInverseVPMatrix;
            lastOnTouchDownVPOrigin = zeroOrigin;
        }
        lastOnTouchDownCoord = coordFromScreenPosition(posScreen);
        lastOnMoveCoord = coordFromScreenPosition(lastOnTouchDownInverseVPMatrix, posScreen, lastOnTouchDownVPOrigin);
#else
        lastOnTouchDownCoord = coordFromScreenPosition(posScreen);
#endif
        return true;
    }
}

bool MapCamera3d::onMove(const Vec2F &deltaScreen, bool confirmed, bool doubleClick) {
    if (!config.moveEnabled || cameraFrozen || (lastOnTouchDownPoint == std::nullopt) || !lastOnTouchDownCoord ||
        !lastOnTouchDownFocusCoord) {
        return false;
    }

    inertia = std::nullopt;

    if (doubleClick) {
        double newZoom = zoom * (1.0 - (deltaScreen.y * 0.003));
        updateZoom(newZoom);

        if (initialTouchDownPoint) {
            // Force update of matrices for coordFromScreenPosition-call, ...
            updateMatrices();

            // ..., then find coordinate, that would be below middle-point
            auto newTouchDownCoord = coordFromScreenPosition(initialTouchDownPoint.value());

            // Rotate globe to keep initial coordinate at middle-point
            if (lastOnTouchDownCoord && lastOnTouchDownCoord->systemIdentifier != -1 && newTouchDownCoord.systemIdentifier != -1) {
                double dx = -(newTouchDownCoord.x - lastOnTouchDownCoord->x);
                double dy = -(newTouchDownCoord.y - lastOnTouchDownCoord->y);

                std::lock_guard<std::recursive_mutex> lock(paramMutex);

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
    if (!initialTouchDownPoint.has_value()) {
        return false;
    }
    auto newTouchDownCoord = coordFromScreenPosition(lastOnTouchDownInverseVPMatrix, newScreenPos, lastOnTouchDownVPOrigin);
    auto lastOnTouchDownZeroCoord =
            coordFromScreenPosition(lastOnTouchDownInverseVPMatrix, initialTouchDownPoint.value(), lastOnTouchDownVPOrigin);
#else
    auto newTouchDownCoord = coordFromScreenPosition(newScreenPos);
#endif

    if (newTouchDownCoord.systemIdentifier == -1) {
        return false;
    }

    std::lock_guard<std::recursive_mutex> lock(paramMutex);

#ifdef ANDROID
    double dx = -(newTouchDownCoord.x - lastOnTouchDownZeroCoord.x);
    double dy = -(newTouchDownCoord.y - lastOnTouchDownZeroCoord.y);

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
    double dx = -(newTouchDownCoord.x - lastOnTouchDownCoord->x);
    double dy = -(newTouchDownCoord.y - lastOnTouchDownCoord->y);

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

    return true;
}

void MapCamera3d::setupInertia() {
    float velocityFactor =
            std::sqrt(currentDragVelocity.x * currentDragVelocity.x + currentDragVelocity.y * currentDragVelocity.y) / zoom;

    const double t1Factor = 1e-8;
    const double t2Factor = t1Factor / 100.0;

    double t1 = velocityFactor >= t1Factor ? 30.0 : 0.0;
    double t2 = velocityFactor >= t2Factor ? 200.0 : 0.0;

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

    const auto [adjustedPosition, adjustedZoom] =
            getBoundsCorrectedCoords(Coord(focusPointPosition.systemIdentifier, focusPointPosition.x + xDiffMap,
                                           focusPointPosition.y + yDiffMap, focusPointPosition.z),
                                     zoom);

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

    {
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        inertia = std::nullopt;
    }

    auto targetZoom = zoom / 2;
    targetZoom = std::max(std::min(targetZoom, zoomMin), zoomMax);

    // Get initial coordinate at touch
    auto centerCoordBefore = coordFromScreenPosition(posScreen);

    // Force update of matrices with new zoom for coordFromScreenPosition-call, ...
    auto originalZoom = zoom;
    setZoom(targetZoom, false);
    updateMatrices();

    // ..., then find coordinate, that would be at touch
    auto centerCoordAfter = coordFromScreenPosition(posScreen);

    // Reset zoom before animation
    {
        std::lock_guard<std::recursive_mutex> lock(paramMutex);
        setZoom(originalZoom, false);
    }
    updateMatrices();

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

    } else {
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
    updateMatrices();

    // ..., then find coordinate, that would be below middle-point
    auto centerCoordAfter = coordFromScreenPosition(posScreen);

    // Reset zoom before animation
    setZoom(originalZoom, false);
    updateMatrices();

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

    } else {
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
        updateMatrices();

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

    if (config.snapToNorthEnabled && !cameraFrozen && (angle < ROTATION_LOCKING_ANGLE || angle > (360 - ROTATION_LOCKING_ANGLE))) {
        std::weak_ptr<MapCamera3d> weakSelf = std::static_pointer_cast<MapCamera3d>(shared_from_this());
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        rotationAnimation = std::make_shared<DoubleAnimation>(
                cameraZoomConfig.animationDurationMs, this->angle, angle < ROTATION_LOCKING_ANGLE ? 0 : 360,
                InterpolatorFunction::EaseInOut,
                [weakSelf](double angle) {
                    if (auto selfPtr = weakSelf.lock()) {
                        selfPtr->angle = angle;
                        auto mapInterface = selfPtr->mapInterface;
                        if (mapInterface) {
                            mapInterface->invalidate();
                        }
                        selfPtr->notifyListeners(ListenerType::ROTATION | ListenerType::BOUNDS);
                    }
                },
                [weakSelf] {
                    if (auto selfPtr = weakSelf.lock()) {
                        selfPtr->angle = 0;
                        selfPtr->rotationAnimation = nullptr;
                        auto mapInterface = selfPtr->mapInterface;
                        if (mapInterface) {
                            mapInterface->invalidate();
                        }
                        selfPtr->notifyListeners(ListenerType::ROTATION | ListenerType::BOUNDS);
                    }
                });
        rotationAnimation->start();
        mapInterface->invalidate();
        return true;
    }

    return false;
}

Coord MapCamera3d::coordFromScreenPosition(const ::Vec2F &posScreen) {
    std::lock_guard<std::recursive_mutex> lock(matrixMutex);
    return coordFromScreenPosition(inverseVPMatrix, posScreen);
}

::Coord MapCamera3d::coordFromScreenPositionZoom(const ::Vec2F &posScreen, float zoom) {
    // TODO: fix
    std::lock_guard<std::recursive_mutex> lock(matrixMutex);
    return coordFromScreenPosition(inverseVPMatrix, posScreen);
}

Coord MapCamera3d::coordFromScreenPosition(const std::vector<double> &inverseVPMatrix, const ::Vec2F &posScreen) {
    return coordFromScreenPosition(inverseVPMatrix, posScreen, origin);
}

Coord MapCamera3d::coordFromScreenPosition(const std::vector<double> &inverseVPMatrix, const ::Vec2F &posScreen,
                                           const Vec3D &origin) {
    auto viewport = mapInterface->getRenderingContext()->getViewportSize();

    auto worldPosFrontVec =
        Vec4D(((double)posScreen.x / (double)viewport.x * 2.0 - 1), -((double)posScreen.y / (double)viewport.y * 2.0 - 1), -1, 1);

    auto worldPosBackVec =
        Vec4D(((double)posScreen.x / (double)viewport.x * 2.0 - 1), -((double)posScreen.y / (double)viewport.y * 2.0 - 1), 1, 1);

    const double rx = origin.x;
    const double ry = origin.y;
    const double rz = origin.z;

    worldPosFrontVec = MatrixD::multiply(inverseVPMatrix, worldPosFrontVec);
    auto worldPosFront = Vec3D((worldPosFrontVec.x / worldPosFrontVec.w) + rx, (worldPosFrontVec.y / worldPosFrontVec.w) + ry,
                               (worldPosFrontVec.z / worldPosFrontVec.w) + rz);

    worldPosBackVec = MatrixD::multiply(inverseVPMatrix, worldPosBackVec);
    auto worldPosBack = Vec3D((worldPosBackVec.x / worldPosBackVec.w) + rx, (worldPosBackVec.y / worldPosBackVec.w) + ry,
                              (worldPosBackVec.z / worldPosBackVec.w) + rz);

    bool didHit = false;
    auto point = MapCamera3DHelper::raySphereIntersection(worldPosFront, worldPosBack, Vec3D(0.0, 0.0, 0.0), 1.0, didHit);

    if (didHit) {
        double longitude = std::atan2(point.x, point.z) * 180 / M_PI - 90;
        if (longitude < -180) {
            longitude += 360;
        }
        double latitude = std::asin(point.y) * 180 / M_PI;
        return Coord(CoordinateSystemIdentifiers::EPSG4326(), longitude, latitude, 0);
    } else {
        return Coord(-1, 0, 0, 0);
    }
}

bool MapCamera3d::gluInvertMatrix(const std::vector<double> &m, std::vector<double> &invOut) {

    if (m.size() != 16 || invOut.size() != 16) {
        return false;
    }

    double inv[16], det;
    int i;

    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] -
             m[13] * m[7] * m[10];

    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] +
             m[12] * m[7] * m[10];

    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] -
             m[12] * m[7] * m[9];

    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] +
              m[12] * m[6] * m[9];

    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] +
             m[13] * m[3] * m[10];

    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] -
             m[12] * m[3] * m[10];

    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] +
             m[12] * m[3] * m[9];

    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] -
              m[12] * m[2] * m[9];

    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] -
             m[13] * m[3] * m[6];

    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] +
             m[12] * m[3] * m[6];

    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] -
              m[12] * m[3] * m[5];

    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] +
              m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] +
             m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] -
             m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] +
              m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] -
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
    std::lock_guard<std::recursive_mutex> lock(matrixMutex);
    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
    if (validVpMatrix && sizeViewport.x != 0 && sizeViewport.y != 0) {
        auto coordCartesian = convertToCartesianCoordinates(coord);
        return screenPosFromCartesianCoord(coordCartesian, sizeViewport);
    }

    return Vec2F(0.0, 0.0);
}

Vec2F MapCamera3d::screenPosFromCartesianCoord(const Vec3D &coord, const Vec2I &sizeViewport) {
    const auto &cc = Vec4D(coord.x - origin.x, coord.y - origin.y, coord.z - origin.z, 1.0);
    return screenPosFromCartesianCoord(cc, sizeViewport);
}

Vec2F MapCamera3d::screenPosFromCartesianCoord(const Vec4D &coord, const Vec2I &sizeViewport) {
    std::lock_guard<std::recursive_mutex> lock(matrixMutex);
    if (validVpMatrix) {
        const auto &projected = projectedPoint(coord);

        // Map from [-1, 1] to screenPixels, with (0,0) being the top left corner
        double screenXDiffToCenter = projected.x * sizeViewport.x / 2.0;
        double screenYDiffToCenter = projected.y * sizeViewport.y / 2.0;

        double posScreenX = screenXDiffToCenter + ((double) sizeViewport.x / 2.0);
        double posScreenY = ((double) sizeViewport.y / 2.0) - screenYDiffToCenter;

        return Vec2F(posScreenX, posScreenY);
    }

    return Vec2F(0.0, 0.0);
}

::Vec2F MapCamera3d::screenPosFromCoordZoom(const ::Coord &coord, float zoom) {
    // TODO: fix
    return screenPosFromCoord(coord);
}

// padding in percentage, where 1.0 = rect is half of full width and height
bool MapCamera3d::coordIsVisibleOnScreen(const ::Coord &coord, float paddingPc) {
    // 1. Check that coordinate is not on the back of the globe
    if (!coordIsOnFrontHalfOfGlobe(coord) || coordIsFarAwayFromFocusPoint(coord)) {
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

bool MapCamera3d::coordIsFarAwayFromFocusPoint(const ::Coord &coord) {
    const auto coordinateConverter = CoordinateConversionHelperInterface::independentInstance();
    Coord wgsC1 = coordinateConverter->convert(CoordinateSystemIdentifiers::EPSG4326(), focusPointPosition);
    Coord wgsC2 = coordinateConverter->convert(CoordinateSystemIdentifiers::EPSG4326(), coord);

    const double R = 6371; // Radius of the earth in meters
    double latDistance = (wgsC2.y - wgsC1.y) * M_PI / 180.0;
    double lonDistance = (wgsC2.x - wgsC1.x) * M_PI / 180.0;
    double a = std::sin(latDistance / 2) * std::sin(latDistance / 2) + std::cos(wgsC1.y * M_PI / 180.0) *
                                                                       std::cos(wgsC2.y * M_PI / 180.0) *
                                                                       std::sin(lonDistance / 2) * std::sin(lonDistance / 2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    return R * c > 4000;
}

bool MapCamera3d::coordIsOnFrontHalfOfGlobe(Coord coord) {
    // Coordinate is on front half of the globe if the projected z-value is in front
    // of the projected z-value of the globe center

    const auto coordCartesian = convertToCartesianCoordinates(coord);
    const auto projectedCoord = projectedPoint(coordCartesian);

    const auto projectedCenter = projectedPoint({-origin.x, -origin.y, -origin.z, 1});

    bool isInFront = projectedCoord.z <= projectedCenter.z;

    return isInFront;
}

Vec4D MapCamera3d::convertToCartesianCoordinates(const Coord &coord) const {
    Coord renderCoord = conversionHelper->convertToRenderSystem(coord);

    return {(renderCoord.z * sin(renderCoord.y) * cos(renderCoord.x)) - origin.x, (renderCoord.z * cos(renderCoord.y)) - origin.y,
            (-renderCoord.z * sin(renderCoord.y) * sin(renderCoord.x)) - origin.z, 1.0};
}

// Point given in cartesian coordinates, where (0,0,0) is the center of the globe
Vec4D MapCamera3d::projectedPoint(const Vec4D &point) const {
    auto projected = MatrixD::multiply(vpMatrixD, point);

    auto w = projected.w;
    projected.x /= w; // percentage in x direction in [-1, 1], 0 being the center of the screen)
    projected.y /= w; // percentage in y direction in [-1, 1], 0 being the center of the screen)
    projected.z /= w; // percentage in z direction in [-1, 1], 0 being the center of the screen)
    projected.w = 1.0;

    return projected;
}

double MapCamera3d::mapUnitsFromPixels(double distancePx) {
    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
    if (validVpMatrix && sizeViewport.x != 0 && sizeViewport.y != 0) {
        Coord focusRenderCoord = conversionHelper->convertToRenderSystem(getCenterPosition());

        const double sampleSize = (M_PI / 180.0) * 0.5;
        const auto cartOne = convertToCartesianCoordinates(focusRenderCoord);
        const auto cartTwo = convertToCartesianCoordinates(Coord(focusRenderCoord.systemIdentifier, focusRenderCoord.x + sampleSize,
                                                                 focusRenderCoord.y + sampleSize * 0.5, focusRenderCoord.z));
        const auto projectedOne = projectedPoint(cartOne);
        const auto projectedTwo = projectedPoint(cartTwo);
        const auto sampleDistance =
                std::sqrt((cartOne.x - cartTwo.x) * (cartOne.x - cartTwo.x) + (cartOne.y - cartTwo.y) * (cartOne.y - cartTwo.y) +
                          (cartOne.z - cartTwo.z) * (cartOne.z - cartTwo.z));

        const auto x = (projectedTwo.x - projectedOne.x) * sizeViewport.x;
        const auto y = (projectedTwo.y - projectedOne.y) * sizeViewport.y;
        const double projectedLength = MatrixD::length(x, y, 0.0);

        // 1.4 is an empirically determined scaling factor, observed to align 2D and 3D measurements
        // based on visual comparisons of screenshots.
        // TODO: Investigate the origin of this 1.4 factor
        return 1.4 * distancePx * sqrt(sampleDistance * sampleDistance * 2) / projectedLength;
    }
    return distancePx * screenPixelAsRealMeterFactor * zoom;
}

double MapCamera3d::getScalingFactor() {
    std::lock_guard<std::recursive_mutex> writeLock(matrixMutex);
    return lastScalingFactor;
}

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
    RectCoord boundsMapSpace = conversionHelper->convertRect(mapCoordinateSystem.identifier, bounds);
    this->bounds = boundsMapSpace;

    const auto [adjPosition, adjZoom] = getBoundsCorrectedCoords(focusPointPosition, zoom);
    focusPointPosition = adjPosition;
    zoom = adjZoom;

    mapInterface->invalidate();
}

RectCoord MapCamera3d::getBounds() { return bounds; }

bool MapCamera3d::isInBounds(const Coord &coords) {
    Coord mapCoords = conversionHelper->convert(mapCoordinateSystem.identifier, coords);

    auto const bounds = this->bounds;

    double minHor = std::min(bounds.topLeft.x, bounds.bottomRight.x);
    double maxHor = std::max(bounds.topLeft.x, bounds.bottomRight.x);
    double minVert = std::min(bounds.topLeft.y, bounds.bottomRight.y);
    double maxVert = std::max(bounds.topLeft.y, bounds.bottomRight.y);

    return mapCoords.x <= maxHor && mapCoords.x >= minHor && mapCoords.y <= maxVert && mapCoords.y >= minVert;
}

Coord MapCamera3d::adjustCoordForPadding(const Coord &coords, double targetZoom) {
    Coord coordinates = conversionHelper->convert(focusPointPosition.systemIdentifier, coords);

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

    const auto &id = position.systemIdentifier;

    Coord topLeft = conversionHelper->convert(id, bounds.topLeft);
    Coord bottomRight = conversionHelper->convert(id, bounds.bottomRight);

    Coord clampedPosition =
            Coord(id, std::clamp(position.x, std::min(topLeft.x, bottomRight.x), std::max(topLeft.x, bottomRight.x)),
                  std::clamp(position.y, std::min(topLeft.y, bottomRight.y), std::max(topLeft.y, bottomRight.y)), position.z);

    assert(clampedPosition.systemIdentifier == 4326);
    return {clampedPosition, zoom};
}

void MapCamera3d::setRotationEnabled(bool enabled) { config.rotationEnabled = enabled; }

void MapCamera3d::setSnapToNorthEnabled(bool enabled) { config.snapToNorthEnabled = enabled; }

void MapCamera3d::setBoundsRestrictWholeVisibleRect(bool enabled) {
    config.boundsRestrictWholeVisibleRect = enabled;
    clampCenterToPaddingCorrectedBounds();
}

void MapCamera3d::clampCenterToPaddingCorrectedBounds() {
    const auto [newPosition, newZoom] =
        getBoundsCorrectedCoords(Coord(focusPointPosition.systemIdentifier, std::fmod(focusPointPosition.x + 540.0, 360.0) - 180.0,
                          focusPointPosition.y, focusPointPosition.z),
                    zoom);
    std::lock_guard<std::recursive_mutex> lock(paramMutex);
    focusPointPosition = newPosition;
    zoom = newZoom;
}

float MapCamera3d::getScreenDensityPpi() { return screenDensityPpi; }

std::shared_ptr<MapCamera3dInterface> MapCamera3d::asMapCamera3d() { return shared_from_this(); }

void MapCamera3d::setCameraConfig(const Camera3dConfig &config, std::optional<float> durationSeconds,
                                  std::optional<float> targetZoom, const std::optional<::Coord> &targetCoordinate) {
    {
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        if (pitchAnimation) {
            pitchAnimation->cancel();
            pitchAnimation = nullptr;
        }
        if (verticalDisplacementAnimation) {
            verticalDisplacementAnimation->cancel();
            verticalDisplacementAnimation = nullptr;
        }
        if (zoomAnimation) {
            zoomAnimation->cancel();
            zoomAnimation = nullptr;
        }
        if (coordAnimation) {
            coordAnimation->cancel();
            coordAnimation = nullptr;
        }
    }

    cameraZoomConfig = config;

    double initialZoom = zoom;

    zoomMin = cameraZoomConfig.minZoom;
    zoomMax = cameraZoomConfig.maxZoom;

    if (targetZoom) {
        // temporarily set target zoom to get target pitch
        this->zoom = *targetZoom;
    }

    double targetPitch = getCameraPitch();
    double targetVerticalDisplacement = getCameraVerticalDisplacement();
    this->zoom = initialZoom;

    if (durationSeconds) {
        long long duration = *durationSeconds * 1000;
        double initialPitch = cameraPitch;
        double initialVerticalDisplacement = cameraVerticalDisplacement;

        std::weak_ptr<MapCamera3d> weakSelf = std::static_pointer_cast<MapCamera3d>(shared_from_this());

        std::lock_guard<std::recursive_mutex> lock(animationMutex);

        pitchAnimation = std::make_shared<DoubleAnimation>(
                duration, initialPitch, targetPitch, InterpolatorFunction::EaseInOut,
                [weakSelf](double pitch) {
                    if (auto selfPtr = weakSelf.lock()) {
                        selfPtr->cameraPitch = pitch;
                        auto mapInterface = selfPtr->mapInterface;
                        if (mapInterface) {
                            mapInterface->invalidate();
                        }
                    }
                },
                [weakSelf, targetPitch] {
                    if (auto selfPtr = weakSelf.lock()) {
                        selfPtr->cameraPitch = targetPitch;
                        selfPtr->pitchAnimation = nullptr;
                    }
                });
        pitchAnimation->start();

        verticalDisplacementAnimation = std::make_shared<DoubleAnimation>(
                duration, initialVerticalDisplacement, targetVerticalDisplacement, InterpolatorFunction::EaseInOut,
                [weakSelf](double dis) {
                    if (auto selfPtr = weakSelf.lock()) {
                        selfPtr->cameraVerticalDisplacement = dis;
                        auto mapInterface = selfPtr->mapInterface;
                        if (mapInterface) {
                            mapInterface->invalidate();
                        }
                    }
                },
                [weakSelf, targetVerticalDisplacement] {
                    if (auto selfPtr = weakSelf.lock()) {
                        selfPtr->cameraVerticalDisplacement = targetVerticalDisplacement;
                        selfPtr->verticalDisplacementAnimation = nullptr;
                    }
                });
        verticalDisplacementAnimation->start();

        if (targetZoom) {
            zoomAnimation = std::make_shared<DoubleAnimation>(
                    duration, initialZoom, *targetZoom, InterpolatorFunction::EaseInOut,
                    [weakSelf](double zoom) {
                        if (auto selfPtr = weakSelf.lock()) {
                            selfPtr->zoom = zoom;
                            auto mapInterface = selfPtr->mapInterface;
                            if (mapInterface) {
                                mapInterface->invalidate();
                            }
                        }
                    },
                    [weakSelf, targetZoom] {
                        if (auto selfPtr = weakSelf.lock()) {
                            selfPtr->zoom = *targetZoom;
                            selfPtr->zoomAnimation = nullptr;
                        }
                    });
            zoomAnimation->start();
        }

        if (targetCoordinate) {
            Coord startPosition = conversionHelper->convert(CoordinateSystemIdentifiers::EPSG4326(), focusPointPosition);

            coordAnimation = std::make_shared<CoordAnimation>(
                    duration, startPosition, *targetCoordinate, std::nullopt, InterpolatorFunction::EaseInOut,
                    [weakSelf](Coord positionMapSystem) {
                        if (auto selfPtr = weakSelf.lock()) {
                            assert(positionMapSystem.systemIdentifier == 4326);
                            std::lock_guard<std::recursive_mutex> lock(selfPtr->paramMutex);
                            selfPtr->focusPointPosition = positionMapSystem;
                            selfPtr->notifyListeners(ListenerType::BOUNDS);
                            auto mapInterface = selfPtr->mapInterface;
                            if (mapInterface) {
                                mapInterface->invalidate();
                            }
                        }
                    },
                    [weakSelf] {
                        if (auto selfPtr = weakSelf.lock()) {
                            assert(selfPtr->coordAnimation->endValue.systemIdentifier == 4326);
                            std::lock_guard<std::recursive_mutex> lock(selfPtr->paramMutex);
                            selfPtr->focusPointPosition = selfPtr->coordAnimation->endValue;
                            selfPtr->notifyListeners(ListenerType::BOUNDS);
                            selfPtr->coordAnimation = nullptr;
                            auto mapInterface = selfPtr->mapInterface;
                            if (mapInterface) {
                                mapInterface->invalidate();
                            }
                        }
                    });
            coordAnimation->start();
        }
    } else {
        std::lock_guard<std::recursive_mutex> lock(paramMutex);
        this->cameraPitch = targetPitch;
        this->cameraVerticalDisplacement = targetVerticalDisplacement;
        if (targetZoom) {
            this->zoom = *targetZoom;
        }
        if (targetCoordinate) {
            this->focusPointPosition = *targetCoordinate;
        }
    }

    mapInterface->invalidate();
}

Camera3dConfig MapCamera3d::getCameraConfig() { return cameraZoomConfig; }

void MapCamera3d::notifyListenerBoundsChange() { notifyListeners(ListenerType::BOUNDS); }

void MapCamera3d::updateZoom(double zoom_) {
    auto zoomMin = getMinZoom();
    auto zoomMax = getMaxZoom();

    std::lock_guard<std::recursive_mutex> lock(paramMutex);
    zoom = std::clamp(zoom_, zoomMax, zoomMin);
    cameraVerticalDisplacement = getCameraVerticalDisplacement();
    cameraPitch = getCameraPitch();
}

double MapCamera3d::getCameraVerticalDisplacement() {
    return valueForZoom(cameraZoomConfig.verticalDisplacementInterpolationValues, zoom);
}

double MapCamera3d::getCameraPitch() {
    return valueForZoom(cameraZoomConfig.pitchInterpolationValues, zoom);
}

double MapCamera3d::getCameraFieldOfView() { return 42; }

double MapCamera3d::getCameraDistance(Vec2I sizeViewport, double zoom) {
    double f = getCameraFieldOfView();
    double w = (double) sizeViewport.y;
    double pixelsPerMeter = this->screenDensityPpi / 0.0254;
    float d = (zoom * w / pixelsPerMeter / 2.0) / tan(f / 2.0 * M_PI / 180.0);
    float R = 6378137.0;
    return d / R;
}

double MapCamera3d::zoomForMeterWidth(Vec2I sizeViewport, Vec2F sizeMeters) {
    double pixelsPerMeter = this->screenDensityPpi / 0.0254;
    double vpr = (double) sizeViewport.x / (double) sizeViewport.y;
    if(vpr < 1) {
        vpr = 1.0/vpr;
    }


    double vprX = 1.0;
    double vprY = 1.0;

    double fy = getCameraFieldOfView();
    double halfAngleRadianY = fy * 0.5 * M_PI / 180.0;

    double fx = 2 * atan(vpr * tan(halfAngleRadianY));
    double halfAngleRadianX = fx * 0.5 * M_PI / 180.0;

    double metersX = sizeMeters.x * 0.5 * vpr;
    double metersY = sizeMeters.y * 0.5 * vpr;
    double wX = sizeViewport.x;
    double wY = sizeViewport.y;

    double dx = metersX / tan(halfAngleRadianX);
    double zoomX = 2.0 * dx * pixelsPerMeter * std::tan(halfAngleRadianX) / wX;
    double dy = metersY / tan(halfAngleRadianY);
    double zoomY = 2.0 * dy * pixelsPerMeter * std::tan(halfAngleRadianY) / wY;

    return std::max(zoomX, zoomY);
}

float MapCamera3d::valueForZoom(const CameraInterpolation &interpolator, double zoom) {
    if (interpolator.stops.size() == 0) {
        return 0;
    }

    // 0 --> minZoom
    // 1 --> maxZoom
    auto t = zoomMax != zoomMin ? (zoomMin - zoom) / (zoomMin - zoomMax) : 0.0;
    const auto &values = interpolator.stops;

    if (t <= values.front().stop) {
        return values.front().value;
    }

    if (t >= values.back().stop) {
        return values.back().value;
    }

    // Find the correct segment where t lies
    for (size_t i = 1; i < values.size(); ++i) {
        if (t <= values[i].stop) {
            const auto &a = values[i - 1];
            const auto &b = values[i];

            // Linear interpolation
            float interp = (t - a.stop) / (b.stop - a.stop);
            return a.value + interp * (b.value - a.value);
        }
    }
#if __cplusplus >= 202302L
    std::unreachable();
#else
    __builtin_unreachable();
#endif
}

// Convert degrees to radians
double toRadians(double degrees) {
    return degrees * M_PI / 180.0;
}

// Haversine formula to calculate distance between two lat/lon points
double MapCamera3d::haversineDistance(double lat1, double lon1, double lat2, double lon2) {
    double lat1Rad = lat1 * M_PI / 180.0;
    double lon1Rad = lon1 * M_PI / 180.0;
    double lat2Rad = lat2 * M_PI / 180.0;
    double lon2Rad = lon2 * M_PI / 180.0;

    double deltaLat = lat2Rad - lat1Rad;
    double deltaLon = lon2Rad - lon1Rad;

    double a = std::sin(deltaLat / 2) * std::sin(deltaLat / 2) +
               std::cos(lat1Rad) * std::cos(lat2Rad) *
               std::sin(deltaLon / 2) * std::sin(deltaLon / 2);

    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    return 6378137.0 * c;
}

// Calculate the maximum distance for the bounding box
Vec2F MapCamera3d::calculateDistance(double latTopLeft, double lonTopLeft, double latBottomRight, double lonBottomRight) {
    // Width distance (horizontal distance)
    double width = haversineDistance(latTopLeft, lonTopLeft, latTopLeft, lonBottomRight);

    // Height distance (vertical distance)
    double height = haversineDistance(latTopLeft, lonTopLeft, latBottomRight, lonTopLeft);

    // Return the maximum of the two distances
    return Vec2F(width, height);
}
