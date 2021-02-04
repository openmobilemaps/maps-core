#include "Matrix.h"
#include "MapCamera2d.h"
#include "Vec2D.h"
#include "MapInterface.h"
#include "MapConfig.h"

MapCamera2d::MapCamera2d(const std::shared_ptr<MapInterface> &mapInterface, float screenDensityPpi) :
        mapInterface(mapInterface),
        mapCoordinateSystem(mapInterface->getMapConfig().mapCoordinateSystem),
        screenDensityPpi(screenDensityPpi),
        screenPixelAsRealMeterFactor(1.0 / (2.54 * screenDensityPpi)) {
    auto mapConfig = mapInterface->getMapConfig();
    mapCoordinateSystem = mapConfig.mapCoordinateSystem;
    double centerX = mapCoordinateSystem.boundsLeft + 0.5 * (mapCoordinateSystem.boundsRight - mapCoordinateSystem.boundsLeft);
    double centerY = mapCoordinateSystem.boundsBottom + 0.5 * (mapCoordinateSystem.boundsTop - mapCoordinateSystem.boundsBottom);
    centerPosition = Vec2D(centerX, centerY);
    zoom = mapConfig.zoomMin;
}

void MapCamera2d::moveToCenterPositionZoom(const ::Vec2D &centerPosition, double zoom, bool animated) {
    if (animated) {
        // TODO
    } else {
        this->zoom = zoom;
        this->centerPosition = centerPosition;
    }
}

void MapCamera2d::moveToCenterPosition(const ::Vec2D &centerPosition, bool animated) {
    if (animated) {
        // TODO
    } else {
        this->centerPosition = centerPosition;
    }
}

::Vec2D MapCamera2d::getCenterPosition() {
    return centerPosition;
}

void MapCamera2d::setZoom(double zoom, bool animated) {
    if (animated) {
        // TODO
    } else {
        this->zoom = zoom;
    }
}

double MapCamera2d::getZoom() {
    return zoom;
}

void MapCamera2d::setPaddingLeft(float padding) {
    paddingLeft = padding;
}

void MapCamera2d::setPaddingRight(float padding) {
    paddingRight = padding;
}

void MapCamera2d::setPaddingTop(float padding) {
    paddingTop = padding;
}

void MapCamera2d::setPaddingBottom(float padding) {
    paddingBottom = padding;
}

void MapCamera2d::addListener(const std::shared_ptr<MapCamera2dListenerInterface> &listener) {
    listeners.insert(listener);
}

void MapCamera2d::removeListener(const std::shared_ptr<MapCamera2dListenerInterface> &listener) {
    listeners.erase(listener);
}

std::shared_ptr<::CameraInterface> MapCamera2d::asCameraInterface() {
    return shared_from_this();
}

std::vector<float> MapCamera2d::getMvpMatrix() {
    std::vector<float> newMvpMatrix(16, 0);

    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
    double zoomFactor =  screenPixelAsRealMeterFactor * zoom;
    Vec2D coordBottomLeft = Vec2D(centerPosition.x - ((sizeViewport.x) * 0.5 * zoomFactor) + paddingLeft,
                               centerPosition.y - ((sizeViewport.y) * 0.5 * zoomFactor) + paddingTop);

    Matrix::setIdentityM(newMvpMatrix, 0);
    Matrix::orthoM(newMvpMatrix, 0, 0, sizeViewport.x, 0, sizeViewport.y, -1, 1);

    Matrix::scaleM(newMvpMatrix, 0, 1 / zoomFactor, 1 / zoomFactor, 1);
    Matrix::translateM(newMvpMatrix, 0, -coordBottomLeft.x, -coordBottomLeft.y, 0);

    Matrix::translateM(newMvpMatrix, 0, centerPosition.x, centerPosition.y, 0);
    Matrix::rotateM(newMvpMatrix, 0.0, angle, 0.0, 0.0, 1.0);
    Matrix::translateM(newMvpMatrix, 0, -centerPosition.x, -centerPosition.y, 0);

    return newMvpMatrix;
}

bool MapCamera2d::onMove(const Vec2F &deltaScreen, bool confirmed, bool doubleClick) {
    centerPosition.x -= deltaScreen.x * zoom * screenPixelAsRealMeterFactor;
    centerPosition.y += deltaScreen.y * zoom * screenPixelAsRealMeterFactor;
    return true;
}


bool MapCamera2d::onDoubleClick(const ::Vec2F &posScreen) {
    zoom = std::max(zoom / 2, mapInterface->getMapConfig().zoomMax);
}
