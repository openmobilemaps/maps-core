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
#include "DateHelper.h"
#include "MapConfig.h"
#include "MapInterface.h"
#include "Matrix.h"
#include "Vec2D.h"
#include "Vec2FHelper.h"
#include "DoubleAnimation.h"
#include "CoordAnimation.h"

#define DEFAULT_ANIM_LENGTH 300
#define ROTATION_THRESHOLD 20

MapCamera2d::MapCamera2d(const std::shared_ptr<MapInterface> &mapInterface, float screenDensityPpi)
        : mapInterface(mapInterface), conversionHelper(mapInterface->getCoordinateConverterHelper()),
          mapCoordinateSystem(mapInterface->getMapConfig().mapCoordinateSystem), screenDensityPpi(screenDensityPpi),
          screenPixelAsRealMeterFactor(0.0254 / screenDensityPpi), centerPosition(mapCoordinateSystem.identifier, 0, 0, 0),
          bounds(mapCoordinateSystem.bounds) {
    auto mapConfig = mapInterface->getMapConfig();
    mapCoordinateSystem = mapConfig.mapCoordinateSystem;
    mapSystemRtl = mapCoordinateSystem.bounds.bottomRight.x > mapCoordinateSystem.bounds.topLeft.x;
    mapSystemTtb = mapCoordinateSystem.bounds.bottomRight.y > mapCoordinateSystem.bounds.topLeft.y;
    centerPosition.x = bounds.topLeft.x +
                       0.5 * (bounds.bottomRight.x - bounds.topLeft.x);
    centerPosition.y = bounds.topLeft.y +
                       0.5 * (bounds.bottomRight.y - bounds.topLeft.y);
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
        zoom = zoomMin;
    }

    notifyListeners();
}

void MapCamera2d::moveToCenterPositionZoom(const ::Coord &centerPosition, double zoom, bool animated) {
    Coord positionMapSystem = getBoundsCorrectedCoords(centerPosition);
    if (animated) {
        moveToCenterPosition(centerPosition, true);
        setZoom(zoom, true);
    } else {
        this->zoom = zoom;
        this->centerPosition.x = positionMapSystem.x;
        this->centerPosition.y = positionMapSystem.y;
        notifyListeners();
    }
}

void MapCamera2d::moveToCenterPosition(const ::Coord &centerPosition, bool animated) {
    Coord positionMapSystem = getBoundsCorrectedCoords(centerPosition);
    if (animated) {
        coordAnimation = std::make_shared<CoordAnimation>(DEFAULT_ANIM_LENGTH,
                                                      this->centerPosition,
                                                      centerPosition,
                                                      InterpolatorFunction::EaseIn,
           [=](Coord centerPosition){
            this->moveToCenterPosition(centerPosition, false);
        }, [=]{
            this->moveToCenterPosition(centerPosition, false);
            this->coordAnimation = nullptr;
        });
        coordAnimation->start();
        mapInterface->invalidate();
    } else {
        this->centerPosition.x = positionMapSystem.x;
        this->centerPosition.y = positionMapSystem.y;
        notifyListeners();
    }
}

::Coord MapCamera2d::getCenterPosition() { return centerPosition; }

void MapCamera2d::setZoom(double zoom, bool animated) {
    if (animated) {
        animation = std::make_shared<DoubleAnimation>(DEFAULT_ANIM_LENGTH,
                                                      this->zoom,
                                                      zoom,
                                                      InterpolatorFunction::EaseIn,
           [=](double zoom){
            this->setZoom(zoom, false);
        }, [=]{
            this->setZoom(zoom, false);
            this->animation = nullptr;
        });
        animation->start();
        mapInterface->invalidate();
    } else {
        this->zoom = zoom;
        notifyListeners();
        mapInterface->invalidate();
    }
}

double MapCamera2d::getZoom() { return zoom; }


void MapCamera2d::setRotation(float angle, bool animated) {
    double newAngle = fmod(angle + 360.0, 360.0);
    if (animated) {
        animation = std::make_shared<DoubleAnimation>(DEFAULT_ANIM_LENGTH,
                                                      this->angle,
                                                      newAngle,
                                                      InterpolatorFunction::EaseIn,
           [=](double angle){
            this->setRotation(angle, false);
        }, [=]{
            this->setRotation(newAngle, false);
            this->animation = nullptr;
        });
        animation->start();
        mapInterface->invalidate();
    } else {
        this->angle = newAngle;
        notifyListeners();
        mapInterface->invalidate();
    }
}

float MapCamera2d::getRotation() {
    return angle;
}

void MapCamera2d::setPaddingLeft(float padding, bool animated) {
    if (animated) {
        animation = std::make_shared<DoubleAnimation>(DEFAULT_ANIM_LENGTH,
                                                      this->paddingLeft,
                                                      padding,
                                                      InterpolatorFunction::EaseIn,
           [=](double padding){
            this->setPaddingLeft(padding, false);
        }, [=]{
            this->setPaddingLeft(padding, false);
            this->animation = nullptr;
        });
        animation->start();
        mapInterface->invalidate();
    } else {
        paddingLeft = padding;
        mapInterface->invalidate();
    }
}

void MapCamera2d::setPaddingRight(float padding, bool animated) {
    if (animated) {
        animation = std::make_shared<DoubleAnimation>(DEFAULT_ANIM_LENGTH,
                                                      this->paddingRight,
                                                      padding,
                                                      InterpolatorFunction::EaseIn,
           [=](double padding){
            this->setPaddingRight(padding, false);
        }, [=]{
            this->setPaddingRight(padding, false);
            this->animation = nullptr;
        });
        animation->start();
        mapInterface->invalidate();
    } else {
        paddingRight = padding;
        mapInterface->invalidate();
    }
}

void MapCamera2d::setPaddingTop(float padding, bool animated) {
    if (animated) {
        animation = std::make_shared<DoubleAnimation>(DEFAULT_ANIM_LENGTH,
                                                      this->paddingTop,
                                                      padding,
                                                      InterpolatorFunction::EaseIn,
           [=](double padding){
            this->setPaddingTop(padding, false);
        }, [=]{
            this->setPaddingTop(padding, false);
            this->animation = nullptr;
        });
        animation->start();
        mapInterface->invalidate();
    } else {
        paddingTop = padding;
        mapInterface->invalidate();
    }
}

void MapCamera2d::setPaddingBottom(float padding, bool animated) {
    if (animated) {
        animation = std::make_shared<DoubleAnimation>(DEFAULT_ANIM_LENGTH,
                                                      this->paddingBottom,
                                                      padding,
                                                      InterpolatorFunction::EaseIn,
           [=](double padding){
            this->setPaddingBottom(padding, false);
        }, [=]{
            this->setPaddingBottom(padding, false);
            this->animation = nullptr;
        });
        animation->start();
        mapInterface->invalidate();
    } else {
        paddingBottom = padding;
        mapInterface->invalidate();
    }
}

void MapCamera2d::addListener(const std::shared_ptr<MapCamera2dListenerInterface> &listener) { listeners.insert(listener); }

void MapCamera2d::removeListener(const std::shared_ptr<MapCamera2dListenerInterface> &listener) { listeners.erase(listener); }

std::shared_ptr<::CameraInterface> MapCamera2d::asCameraInterface() { return shared_from_this(); }

std::vector<float> MapCamera2d::getVpMatrix() {
    if (animation) animation->update();
    if (coordAnimation) coordAnimation->update();

    std::vector<float> newVpMatrix(16, 0);

    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
    double zoomFactor = screenPixelAsRealMeterFactor * zoom;

    Coord renderCoordCenter = conversionHelper->convertToRenderSystem(centerPosition);

    Matrix::setIdentityM(newVpMatrix, 0);

    Matrix::orthoM(newVpMatrix, 0, renderCoordCenter.x - 0.5 * sizeViewport.x, renderCoordCenter.x + 0.5 * sizeViewport.x,
                   renderCoordCenter.y + 0.5 * sizeViewport.y, renderCoordCenter.y - 0.5 * sizeViewport.y, -1, 1);

    Matrix::translateM(newVpMatrix, 0, renderCoordCenter.x, renderCoordCenter.y, 0);

    Matrix::scaleM(newVpMatrix, 0, 1 / zoomFactor, 1 / zoomFactor, 1);

    Matrix::rotateM(newVpMatrix, 0.0, angle, 0.0, 0.0, 1.0);

    Matrix::translateM(newVpMatrix, 0, -renderCoordCenter.x, -renderCoordCenter.y, 0);

    Matrix::translateM(newVpMatrix, 0, (paddingLeft - paddingRight) * zoomFactor, (paddingTop - paddingBottom) * zoomFactor, 0);

    return newVpMatrix;
}

std::vector<float>
MapCamera2d::getInvariantModelMatrix(const ::Coord &coordinate, bool scaleInvariant,
                                     bool rotationInvariant) {
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
    double zoomFactor = screenPixelAsRealMeterFactor * zoom;

    double halfWidth = sizeViewport.x * 0.5 * zoomFactor;
    double halfHeight = sizeViewport.y * 0.5 * zoomFactor;

    double sinAngle = sin(angle * M_PI / 180.0);
    double cosAngle = cos(angle * M_PI / 180.0);

    double deltaX = std::abs(halfWidth * cosAngle) + std::abs(halfHeight * sinAngle);
    double deltaY = std::abs(halfWidth * sinAngle) + std::abs(halfHeight * cosAngle);

    double topLeftX = centerPosition.x - deltaX;
    double topLeftY = centerPosition.y + deltaY;
    double bottomRightX = centerPosition.x + deltaX;
    double bottomRightY = centerPosition.y - deltaY;

    Coord topLeft = Coord(mapCoordinateSystem.identifier, topLeftX, topLeftY, centerPosition.z);
    Coord bottomRight = Coord(mapCoordinateSystem.identifier, bottomRightX, bottomRightY, centerPosition.z);
    return RectCoord(topLeft, bottomRight);
}

void MapCamera2d::notifyListeners() {
    auto visibleRect = getVisibleRect();
    for (auto listener : listeners) {
        listener->onVisibleBoundsChanged(visibleRect, zoom);
    }
}

bool MapCamera2d::onMove(const Vec2F &deltaScreen, bool confirmed, bool doubleClick) {
    if (!config.moveEnabled)
        return false;

    if (doubleClick) {
        double newZoom = zoom * (1.0 - (deltaScreen.y * 0.003));

        zoom = std::max(std::min(newZoom, zoomMin), zoomMax);

        notifyListeners();
        mapInterface->invalidate();
        return true;
    }

    float dx = deltaScreen.x;
    float dy = deltaScreen.y;

    float sinAngle = sin(angle * M_PI / 180.0);
    float cosAngle = cos(angle * M_PI / 180.0);

    float leftDiff = (cosAngle * dx + sinAngle * dy);
    float topDiff = (-sinAngle * dx + cosAngle * dy);

    centerPosition.x -= leftDiff * zoom * screenPixelAsRealMeterFactor;
    centerPosition.y += topDiff * zoom * screenPixelAsRealMeterFactor;

    auto config = mapInterface->getMapConfig();
    auto bottomRight = bounds.bottomRight;
    auto topLeft = bounds.topLeft;

    centerPosition.x = std::min(centerPosition.x, bottomRight.x);
    centerPosition.x = std::max(centerPosition.x, topLeft.x);

    centerPosition.y = std::max(centerPosition.y, bottomRight.y);
    centerPosition.y = std::min(centerPosition.y, topLeft.y);

    notifyListeners();
    mapInterface->invalidate();
    return true;
}

bool MapCamera2d::onDoubleClick(const ::Vec2F &posScreen) {
    if (!config.doubleClickZoomEnabled)
        return false;

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

    setZoom(targetZoom, true);
    return true;
}

void MapCamera2d::clearTouch() {
    isRotationThreasholdReached = false;
    tempAngle = angle;
}

bool MapCamera2d::onTwoFingerMove(const std::vector<::Vec2F> &posScreenOld, const std::vector<::Vec2F> &posScreenNew) {
    if (!config.twoFingerZoomEnabled)
        return false;

    if (posScreenOld.size() >= 2) {
        double scaleFactor =
                Vec2FHelper::distance(posScreenNew[0], posScreenNew[1]) / Vec2FHelper::distance(posScreenOld[0], posScreenOld[1]);
        zoom /= scaleFactor;

        zoom = std::max(std::min(zoom, zoomMin), zoomMax);

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

        centerPosition.x += diffCenterX;
        centerPosition.y += diffCenterY;

        if (config.rotationEnabled) {
            float olda = atan2(posScreenOld[0].x - posScreenOld[1].x, posScreenOld[0].y - posScreenOld[1].y);
            float newa = atan2(posScreenNew[0].x - posScreenNew[1].x, posScreenNew[0].y - posScreenNew[1].y);
            if (isRotationThreasholdReached) {
                angle = fmod((angle + (olda - newa) / M_PI * 180.0) + 360.0, 360.0);

                //Update centerPosition such that the midpoint is the rotation center
                double centerXDiff =
                        (centerScreen.x - midpoint.x) * cos(newa - olda) - (centerScreen.y - midpoint.y) * sin(newa - olda) +
                        midpoint.x - centerScreen.x;
                double centerYDiff =
                        (centerScreen.y - midpoint.y) * cos(newa - olda) - (centerScreen.x - midpoint.x) * sin(newa - olda) +
                        midpoint.y - centerScreen.y;
                double rotDiffX = (cosAngle * centerXDiff - sinAngle * centerYDiff);
                double rotDiffY = (cosAngle * centerYDiff + sinAngle * centerXDiff);
                centerPosition.x += rotDiffX * zoom * screenPixelAsRealMeterFactor;
                centerPosition.y += rotDiffY * zoom * screenPixelAsRealMeterFactor;

                for (auto listener : listeners) {
                    listener->onRotationChanged(angle);
                }
            } else {
                tempAngle = fmod((tempAngle + (olda - newa) / M_PI * 180.0) + 360.0, 360.0);
                if (std::abs(tempAngle - angle) >= ROTATION_THRESHOLD) {
                    isRotationThreasholdReached = true;
                }
            }
        }

        auto mapConfig = mapInterface->getMapConfig();
        auto bottomRight = bounds.bottomRight;
        auto topLeft = bounds.topLeft;

        centerPosition.x = std::min(centerPosition.x, bottomRight.x);
        centerPosition.x = std::max(centerPosition.x, topLeft.x);

        centerPosition.y = std::max(centerPosition.y, bottomRight.y);
        centerPosition.y = std::min(centerPosition.y, topLeft.y);

        notifyListeners();
        mapInterface->invalidate();
    }
    return true;
}

Coord MapCamera2d::coordFromScreenPosition(const ::Vec2F &posScreen) {
    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
    double zoomFactor = screenPixelAsRealMeterFactor * zoom;

    double xDiffToCenter = zoomFactor * (posScreen.x - ((double) sizeViewport.x / 2.0));
    double yDiffToCenter = zoomFactor * (posScreen.y - ((double) sizeViewport.y / 2.0));

    double angRad = -angle * M_PI / 180.0;
    double sinAng = std::sin(angRad);
    double cosAng = std::cos(angRad);

    double adjXDiff = xDiffToCenter * cosAng - yDiffToCenter * sinAng;
    double adjYDiff = xDiffToCenter * sinAng + yDiffToCenter * cosAng;

    return Coord(centerPosition.systemIdentifier, centerPosition.x + adjXDiff, centerPosition.y - adjYDiff, centerPosition.z);
}

double MapCamera2d::mapUnitsFromPixels(double distancePx) {
    return distancePx * screenPixelAsRealMeterFactor * zoom;
}

void MapCamera2d::setMinZoom(double zoomMin) {
    this->zoomMin = zoomMin;
    if (zoom > zoomMin) zoom = zoomMin;
    mapInterface->invalidate();
}

void MapCamera2d::setMaxZoom(double zoomMax) {
    this->zoomMax = zoomMax;
    if (zoom < zoomMax) zoom = zoomMax;
    mapInterface->invalidate();
}

void MapCamera2d::setBounds(const RectCoord &bounds) {
    RectCoord boundsMapSpace = mapInterface->getCoordinateConverterHelper()->convertRect(mapCoordinateSystem.identifier, bounds);
    this->bounds = boundsMapSpace;

    centerPosition = getBoundsCorrectedCoords(centerPosition);

    mapInterface->invalidate();
}

Coord MapCamera2d::getBoundsCorrectedCoords(const Coord &coords) {
    Coord mapCoords = mapInterface->getCoordinateConverterHelper()->convert(mapCoordinateSystem.identifier, coords);

    double minHor = std::min(bounds.topLeft.x, bounds.bottomRight.x);
    double maxHor = std::max(bounds.topLeft.x, bounds.bottomRight.x);
    double minVert = std::min(bounds.topLeft.y, bounds.bottomRight.y);
    double maxVert = std::max(bounds.topLeft.y, bounds.bottomRight.y);

    mapCoords.x = std::clamp(mapCoords.x, minHor, maxHor);
    mapCoords.y = std::clamp(mapCoords.y, minVert, maxVert);

    return mapCoords;
}
