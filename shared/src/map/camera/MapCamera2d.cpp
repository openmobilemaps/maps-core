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
#include "Logger.h"

#define DEFAULT_ANIM_LENGTH 300
#define ROTATION_THRESHOLD 20
#define ROTATION_LOCKING_ANGLE 10
#define ROTATION_LOCKING_FACTOR 1.5

MapCamera2d::MapCamera2d(const std::shared_ptr<MapInterface> &mapInterface, float screenDensityPpi)
    : mapInterface(mapInterface)
    , conversionHelper(mapInterface->getCoordinateConverterHelper())
    , mapCoordinateSystem(mapInterface->getMapConfig().mapCoordinateSystem)
    , screenDensityPpi(screenDensityPpi)
    , screenPixelAsRealMeterFactor(0.0254 / screenDensityPpi * mapCoordinateSystem.unitToScreenMeterFactor)
    , centerPosition(mapCoordinateSystem.identifier, 0, 0, 0)
    , bounds(mapCoordinateSystem.bounds) {
    auto mapConfig = mapInterface->getMapConfig();
    mapCoordinateSystem = mapConfig.mapCoordinateSystem;
    mapSystemRtl = mapCoordinateSystem.bounds.topLeft.x > mapCoordinateSystem.bounds.bottomRight.x;
    mapSystemTtb = mapCoordinateSystem.bounds.bottomRight.y > mapCoordinateSystem.bounds.topLeft.y;
    centerPosition.x = bounds.topLeft.x + 0.5 * (bounds.bottomRight.x - bounds.topLeft.x);
    centerPosition.y = bounds.topLeft.y + 0.5 * (bounds.bottomRight.y - bounds.topLeft.y);
    zoom = zoomMax;
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
    }

    notifyListeners(ListenerType::BOUNDS);
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
    Coord currentCenter = this->centerPosition;
    inertia = std::nullopt;
    const auto [targetPosition, adjustedZoom] = getBoundsCorrectedCoords(adjustCoordForPadding(centerPosition, zoom), zoom);
    if (animated) {
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        coordAnimation = std::make_shared<CoordAnimation>(
            DEFAULT_ANIM_LENGTH, currentCenter, targetPosition, centerPosition, InterpolatorFunction::EaseInOut,
            [=](Coord positionMapSystem) {
                this->centerPosition.x = positionMapSystem.x;
                this->centerPosition.y = positionMapSystem.y;
                notifyListeners(ListenerType::BOUNDS);
                mapInterface->invalidate();
            },
            [=] {
                this->centerPosition.x = this->coordAnimation->endValue.x;
                this->centerPosition.y = this->coordAnimation->endValue.y;
                notifyListeners(ListenerType::BOUNDS);
                mapInterface->invalidate();
                this->coordAnimation = nullptr;
            });
        coordAnimation->start();
        setZoom(adjustedZoom, true);
        mapInterface->invalidate();
    } else {
        this->centerPosition = targetPosition;
        this->zoom = adjustedZoom;
        notifyListeners(ListenerType::BOUNDS);
        mapInterface->invalidate();
    }
}

void MapCamera2d::moveToCenterPosition(const ::Coord &centerPosition, bool animated) {
    if (cameraFrozen)
        return;
    Coord currentCenter = this->centerPosition;
    inertia = std::nullopt;
    const auto [targetPosition, adjustedZoom] = getBoundsCorrectedCoords(adjustCoordForPadding(centerPosition, zoom), zoom);
    if (animated) {
        std::lock_guard<std::recursive_mutex> lock(animationMutex);
        coordAnimation = std::make_shared<CoordAnimation>(
            DEFAULT_ANIM_LENGTH, currentCenter, targetPosition, centerPosition, InterpolatorFunction::EaseInOut,
            [=](Coord positionMapSystem) {
                this->centerPosition.x = positionMapSystem.x;
                this->centerPosition.y = positionMapSystem.y;
                notifyListeners(ListenerType::BOUNDS);
                mapInterface->invalidate();
            },
            [=] {
                this->centerPosition.x = this->coordAnimation->endValue.x;
                this->centerPosition.y = this->coordAnimation->endValue.y;
                notifyListeners(ListenerType::BOUNDS);
                mapInterface->invalidate();
                this->coordAnimation = nullptr;
            });
        coordAnimation->start();
        mapInterface->invalidate();
    } else {
        this->centerPosition.x = targetPosition.x;
        this->centerPosition.y = targetPosition.y;
        notifyListeners(ListenerType::BOUNDS);
        mapInterface->invalidate();
    }
}

void MapCamera2d::moveToBoundingBox(const RectCoord &boundingBox, float paddingPc, bool animated, std::optional<double> minZoom, std::optional<double> maxZoom) {
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

::Coord MapCamera2d::getCenterPosition() {
    Coord center = centerPosition;

    Vec2D padVec = Vec2D(0.5 * (paddingLeft - paddingRight) * screenPixelAsRealMeterFactor * zoom,
                         0.5 * (paddingBottom - paddingTop) * screenPixelAsRealMeterFactor * zoom);
    Vec2D rotPadVec = Vec2DHelper::rotate(padVec, Vec2D(0.0, 0.0), angle);
    center.x += rotPadVec.x;
    center.y += rotPadVec.y;

    return center;
}

void MapCamera2d::setZoom(double zoom, bool animated) {
    if (cameraFrozen)
        return;

    if (zoomMin == -1) {
        // Viewport has not beed set yet
        this->zoom = zoom;
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
        const auto [adjPosition, adjZoom] =
                getBoundsCorrectedCoords(adjustCoordForPadding(centerPosition, targetZoom), targetZoom);
        centerPosition = adjPosition;
        this->zoom = adjZoom;
        notifyListeners(ListenerType::BOUNDS);
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
        Coord centerScreen = centerPosition;
        Coord realCenter = getCenterPosition();
        Vec2D rotatedDiff =
            Vec2DHelper::rotate(Vec2D(centerScreen.x - realCenter.x, centerScreen.y - realCenter.y), Vec2D(0.0, 0.0), angleDiff);
        const auto [adjPosition, adjZoom] =
                getBoundsCorrectedCoords(adjustCoordForPadding(Coord(mapCoordinateSystem.identifier, realCenter.x + rotatedDiff.x, realCenter.y + rotatedDiff.y, centerPosition.z), zoom), zoom);
        centerPosition = adjPosition;
        zoom = adjZoom;

        this->angle = newAngle;
        notifyListeners(ListenerType::ROTATION | ListenerType::BOUNDS);
        mapInterface->invalidate();
    }
}

float MapCamera2d::getRotation() { return angle; }

void MapCamera2d::setPaddingLeft(float padding) {
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

void MapCamera2d::setPaddingRight(float padding) {
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

void MapCamera2d::setPaddingTop(float padding) {
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

void MapCamera2d::setPaddingBottom(float padding) {
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

void MapCamera2d::addListener(const std::shared_ptr<MapCamera2dListenerInterface> &listener) {
    std::lock_guard<std::recursive_mutex> lock(listenerMutex);
    if (listeners.count(listener) == 0) {
        listeners.insert(listener);
    }
}

void MapCamera2d::removeListener(const std::shared_ptr<MapCamera2dListenerInterface> &listener) {
    std::lock_guard<std::recursive_mutex> lock(listenerMutex);
    if (listeners.count(listener) > 0) {
        listeners.erase(listener);
    }
}

std::shared_ptr<::CameraInterface> MapCamera2d::asCameraInterface() { return shared_from_this(); }

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
    RectCoord viewBounds = getRectFromViewport(sizeViewport, centerPosition);

    Coord renderCoordCenter = conversionHelper->convertToRenderSystem(centerPosition);

    Matrix::setIdentityM(newVpMatrix, 0);

    Matrix::orthoM(newVpMatrix, 0, renderCoordCenter.x - 0.5 * sizeViewport.x, renderCoordCenter.x + 0.5 * sizeViewport.x,
                   renderCoordCenter.y + 0.5 * sizeViewport.y, renderCoordCenter.y - 0.5 * sizeViewport.y, -1, 1);

    Matrix::translateM(newVpMatrix, 0, renderCoordCenter.x, renderCoordCenter.y, 0);

    Matrix::scaleM(newVpMatrix, 0, 1 / zoomFactor, 1 / zoomFactor, 1);

    Matrix::rotateM(newVpMatrix, 0.0, currentRotation, 0.0, 0.0, 1.0);

    Matrix::translateM(newVpMatrix, 0, -renderCoordCenter.x, -renderCoordCenter.y, 0);

    std::lock_guard<std::recursive_mutex> lock(vpDataMutex);
    lastVpBounds = viewBounds;
    lastVpRotation = currentRotation;
    lastVpZoom = currentZoom;
    return newVpMatrix;
}

std::optional<std::vector<float>> MapCamera2d::getLastVpMatrix() {
    if (!lastVpBounds) {
        return std::nullopt;
    }
    std::vector<float> vpCopy;
    std::copy(newVpMatrix.begin(), newVpMatrix.end(), std::back_inserter(vpCopy));
    return vpCopy;
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
    return getRectFromViewport(sizeViewport, centerPosition);
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

        zoom = std::clamp(newZoom, zoomMax, zoomMin);
        clampCenterToPaddingCorrectedBounds();

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

    float xDiffMap = xDiff * zoom * screenPixelAsRealMeterFactor * (mapSystemRtl ? 1 : -1);
    float yDiffMap = yDiff * zoom * screenPixelAsRealMeterFactor * (mapSystemTtb ? -1 : 1);

    centerPosition.x += xDiffMap;
    centerPosition.y += yDiffMap;

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

    notifyListeners(ListenerType::BOUNDS | ListenerType::MAP_INTERACTION);
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

    const auto [adjustedPosition, adjustedZoom] = getBoundsCorrectedCoords(
            Coord(centerPosition.systemIdentifier,centerPosition.x + xDiffMap,
                  centerPosition.y + yDiffMap, centerPosition.z), zoom);
    centerPosition = adjustedPosition;
    zoom = adjustedZoom;

    notifyListeners(ListenerType::BOUNDS);
    mapInterface->invalidate();
}

bool MapCamera2d::onDoubleClick(const ::Vec2F &posScreen) {
    if (!config.doubleClickZoomEnabled || cameraFrozen)
        return false;

    inertia = std::nullopt;

    auto targetZoom = zoom / 2;

    targetZoom = std::clamp(targetZoom, zoomMax, zoomMin);

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

    targetZoom = std::clamp(targetZoom, zoomMax, zoomMin);

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
        double newZoom = zoom / scaleFactor;

        newZoom = std::clamp(newZoom, zoomMax, zoomMin);

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

        Coord newPos = Coord(centerPosition.systemIdentifier,
                             centerPosition.x + diffCenterX,
                             centerPosition.y + diffCenterY,
                             centerPosition.z);

        if (config.rotationEnabled) {
            float olda = atan2(posScreenOld[0].x - posScreenOld[1].x, posScreenOld[0].y - posScreenOld[1].y);
            float newa = atan2(posScreenNew[0].x - posScreenNew[1].x, posScreenNew[0].y - posScreenNew[1].y);
            if (isRotationThreasholdReached) {
                angle = fmod((angle + (olda - newa) / M_PI * 180.0) + 360.0, 360.0);

                // Update centerPosition such that the midpoint is the rotation center
                double centerXDiff = (centerScreen.x - midpoint.x) * cos(newa - olda) -
                                     (centerScreen.y - midpoint.y) * sin(newa - olda) + midpoint.x - centerScreen.x;
                double centerYDiff = (centerScreen.y - midpoint.y) * cos(newa - olda) -
                                     (centerScreen.x - midpoint.x) * sin(newa - olda) + midpoint.y - centerScreen.y;
                double rotDiffX = (cosAngle * centerXDiff - sinAngle * centerYDiff);
                double rotDiffY = (cosAngle * centerYDiff + sinAngle * centerXDiff);
                newPos.x += rotDiffX * zoom * screenPixelAsRealMeterFactor;
                newPos.y += rotDiffY * zoom * screenPixelAsRealMeterFactor;

                listenerType |= ListenerType::ROTATION;
            } else {
                tempAngle = fmod((tempAngle + (olda - newa) / M_PI * 180.0) + 360.0, 360.0);
                auto diff = std::min(std::abs(tempAngle - angle), std::abs(360.0 - (tempAngle - angle)));
                if (diff >= ROTATION_THRESHOLD && rotationPossible) {
                    isRotationThreasholdReached = true;
                }
            }
        }

        const auto [adjPosition, adjZoom] = getBoundsCorrectedCoords(newPos, newZoom);
        centerPosition = adjPosition;
        zoom = adjZoom;

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

    return Coord(centerPosition.systemIdentifier, centerPosition.x + adjXDiff, centerPosition.y - adjYDiff, centerPosition.z);
}

::Vec2F MapCamera2d::screenPosFromCoord(const Coord &coord) {
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

    double coordXDiffToCenter = mapCoord.x - centerPosition.x;
    double coordYDiffToCenter = mapCoord.y - centerPosition.y;

    double screenXDiffToCenter = coordXDiffToCenter * cosAng - coordYDiffToCenter * sinAng;
    double screenYDiffToCenter = coordXDiffToCenter * sinAng + coordYDiffToCenter * cosAng;

    const Vec2I sizeViewport = renderingContext->getViewportSize();

    double zoomFactor = screenPixelAsRealMeterFactor * zoom;
    double posScreenX = (screenXDiffToCenter / zoomFactor) + ((double)sizeViewport.x / 2.0);
    double posScreenY = ((double)sizeViewport.y / 2.0) - (screenYDiffToCenter / zoomFactor);

    return Vec2F(posScreenX, posScreenY);
}

double MapCamera2d::mapUnitsFromPixels(double distancePx) { return distancePx * screenPixelAsRealMeterFactor * zoom; }

double MapCamera2d::getScalingFactor() { return mapUnitsFromPixels(1.0); }

void MapCamera2d::setMinZoom(double zoomMin) {
    this->zoomMin = zoomMin;
    if (zoom > zoomMin)
        zoom = zoomMin;
    mapInterface->invalidate();
}

void MapCamera2d::setMaxZoom(double zoomMax) {
    this->zoomMax = zoomMax;
    if (zoom < zoomMax)
        zoom = zoomMax;
    mapInterface->invalidate();
}

double MapCamera2d::getMinZoom() { return zoomMin; }

double MapCamera2d::getMaxZoom() { return zoomMax; }

void MapCamera2d::setBounds(const RectCoord &bounds) {
    RectCoord boundsMapSpace = mapInterface->getCoordinateConverterHelper()->convertRect(mapCoordinateSystem.identifier, bounds);
    this->bounds = boundsMapSpace;

    const auto [adjPosition, adjZoom] = getBoundsCorrectedCoords(centerPosition, zoom);
    centerPosition = adjPosition;
    zoom = adjZoom;

    mapInterface->invalidate();
}

RectCoord MapCamera2d::getBounds() { return bounds; }

bool MapCamera2d::isInBounds(const Coord &coords) {
    Coord mapCoords = mapInterface->getCoordinateConverterHelper()->convert(mapCoordinateSystem.identifier, coords);

    auto const &paddingCorrectedBounds = getPaddingCorrectedBounds(zoom);

    double minHor = std::min(paddingCorrectedBounds.topLeft.x, paddingCorrectedBounds.bottomRight.x);
    double maxHor = std::max(paddingCorrectedBounds.topLeft.x, paddingCorrectedBounds.bottomRight.x);
    double minVert = std::min(paddingCorrectedBounds.topLeft.y, paddingCorrectedBounds.bottomRight.y);
    double maxVert = std::max(paddingCorrectedBounds.topLeft.y, paddingCorrectedBounds.bottomRight.y);

    return mapCoords.x <= maxHor && mapCoords.x >= minHor && mapCoords.y <= maxVert && mapCoords.y >= minVert;
}

std::tuple<Coord, double> MapCamera2d::getBoundsCorrectedCoords(const Coord &position, double zoom) {
    auto const &paddingCorrectedBounds = getPaddingCorrectedBounds(zoom);

    auto const clampedPosition = [&paddingCorrectedBounds](const Coord& position) -> Coord {
         return Coord(position.systemIdentifier,
                      std::clamp(position.x,
                                 std::min(paddingCorrectedBounds.topLeft.x, paddingCorrectedBounds.bottomRight.x),
                                 std::max(paddingCorrectedBounds.topLeft.x, paddingCorrectedBounds.bottomRight.x)),
                      std::clamp(position.y,
                                 std::min(paddingCorrectedBounds.topLeft.y, paddingCorrectedBounds.bottomRight.y),
                                 std::max(paddingCorrectedBounds.topLeft.y, paddingCorrectedBounds.bottomRight.y)),
                      position.z);
     };

    if (!config.boundsRestrictWholeVisibleRect) {
        return {clampedPosition(position), zoom};
    } else {
        Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();

        // fallback if the viewport size is not set yet
        if (sizeViewport.x == 0 && sizeViewport.y == 0) {
            return {clampedPosition(position), zoom};
        }

        double zoomFactor = screenPixelAsRealMeterFactor * zoom;

        double halfWidth = sizeViewport.x * 0.5 * zoomFactor;
        double halfHeight = sizeViewport.y * 0.5 * zoomFactor;

        double sinAngle = sin(angle * M_PI / 180.0);
        double cosAngle = cos(angle * M_PI / 180.0);

        double negSinAngle = sin(-angle * M_PI / 180.0);
        double negCosAngle = cos(-angle * M_PI / 180.0);

        double boundsHalfWidth = std::abs(paddingCorrectedBounds.bottomRight.x - paddingCorrectedBounds.topLeft.x) * 0.5;
        double boundsHalfHeight = std::abs(paddingCorrectedBounds.topLeft.y - paddingCorrectedBounds.bottomRight.y) * 0.5;
        double boundsDeltaXVp = std::abs(boundsHalfWidth * negCosAngle) + std::abs(boundsHalfHeight * negSinAngle);
        double boundsDeltaYVp = std::abs(boundsHalfWidth * negSinAngle) + std::abs(boundsHalfHeight * negCosAngle);

        double targetScaleDiffFactorX = boundsDeltaXVp / halfWidth;
        double targetScaleDiffFactorY = boundsDeltaYVp / halfHeight;
        double targetScaleDiffFactor = std::min(std::max(targetScaleDiffFactorX, targetScaleDiffFactorY), 1.0);

        double centerBBoxX = (paddingCorrectedBounds.topLeft.x + (paddingCorrectedBounds.bottomRight.x - paddingCorrectedBounds.topLeft.x) * 0.5);
        double centerBBoxY = (paddingCorrectedBounds.topLeft.y + (paddingCorrectedBounds.bottomRight.y - paddingCorrectedBounds.topLeft.y) * 0.5);
        double centerDiffsX = centerBBoxX - position.x;
        double centerDiffsY = centerBBoxY - position.y;
        double dotHor = centerDiffsX * cosAngle + centerDiffsY * sinAngle;
        double centerDiffHorX = dotHor * cosAngle;
        double centerDiffHorY = dotHor * sinAngle;
        double dotVert = centerDiffsX * -sinAngle + centerDiffsY * cosAngle;
        double centerDiffVertX = -(dotVert * sinAngle);
        double centerDiffVertY = dotVert * cosAngle;

        double positionVpX = negCosAngle * position.x - negSinAngle * position.y;
        double positionVpY = negSinAngle * position.x + negCosAngle * position.y;
        double centerBBoxVpX = negCosAngle * centerBBoxX - negSinAngle * centerBBoxY;
        double centerBBoxVpY = negSinAngle * centerBBoxX + negCosAngle * centerBBoxY;
        double diffLeftVp = (centerBBoxVpX - boundsDeltaXVp) - (positionVpX - halfWidth);
        double diffRightVp = (positionVpX + halfWidth) - (centerBBoxVpX + boundsDeltaXVp);
        double diffTopVp = (positionVpY + halfHeight) - (centerBBoxVpY + boundsDeltaYVp);
        double diffBottomVp = (centerBBoxVpY - boundsDeltaYVp) - (positionVpY - halfHeight);
        double shiftRightVp = std::max(0.0, diffLeftVp) - std::max(0.0, diffRightVp);
        double shiftTopVp = std::max(0.0, diffBottomVp) - std::max(0.0, diffTopVp);

        double shiftRightX = cosAngle * shiftRightVp;
        double shiftRightY = sinAngle * shiftRightVp;
        double shiftTopX = -(sinAngle * shiftTopVp);
        double shiftTopY = cosAngle * shiftTopVp;

        Coord newPosition = position;
        if (targetScaleDiffFactorX <= 1.0) {
            newPosition.x += centerDiffHorX;
            newPosition.y += centerDiffHorY;
        } else {
            newPosition.x += shiftRightX * targetScaleDiffFactor;
            newPosition.y += shiftRightY * targetScaleDiffFactor;
        }
        if (targetScaleDiffFactorY <= 1.0) {
            newPosition.x += centerDiffVertX;
            newPosition.y += centerDiffVertY;
        } else {
            newPosition.x += shiftTopX * targetScaleDiffFactor;
            newPosition.y += shiftTopY * targetScaleDiffFactor;
        }

        double newZoom = zoom * targetScaleDiffFactor;
        return {newPosition, newZoom};
    }
}

Coord MapCamera2d::adjustCoordForPadding(const Coord &coords, double targetZoom) {
    Coord coordinates = mapInterface->getCoordinateConverterHelper()->convert(mapCoordinateSystem.identifier, coords);

    auto adjustedZoom = std::clamp(targetZoom, zoomMax, zoomMin);

    Vec2D padVec = Vec2D(0.5 * (paddingRight - paddingLeft) * screenPixelAsRealMeterFactor * adjustedZoom,
                         0.5 * (paddingTop - paddingBottom) * screenPixelAsRealMeterFactor * adjustedZoom);
    Vec2D rotPadVec = Vec2DHelper::rotate(padVec, Vec2D(0.0, 0.0), angle);
    coordinates.x += rotPadVec.x;
    coordinates.y += rotPadVec.y;

    return coordinates;
}

RectCoord MapCamera2d::getPaddingCorrectedBounds(double zoom) {
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

void MapCamera2d::clampCenterToPaddingCorrectedBounds() {
    const auto [newPosition, newZoom] = getBoundsCorrectedCoords(centerPosition, zoom);
    centerPosition = newPosition;
    zoom = newZoom;
}


void MapCamera2d::setRotationEnabled(bool enabled) { config.rotationEnabled = enabled; }

void MapCamera2d::setSnapToNorthEnabled(bool enabled) { config.snapToNorthEnabled = enabled; }

void MapCamera2d::setBoundsRestrictWholeVisibleRect(bool enabled) {
    config.boundsRestrictWholeVisibleRect = enabled;
    clampCenterToPaddingCorrectedBounds();
}

float MapCamera2d::getScreenDensityPpi() { return screenDensityPpi; }
