#include <logger/Logger.h>
#include "Matrix.h"
#include "MapCamera2d.h"
#include "Vec2D.h"
#include "Coord.h"
#include "MapInterface.h"
#include "MapConfig.h"
#include "Vec2FHelper.h"
#include "DateHelper.h"

MapCamera2d::MapCamera2d(const std::shared_ptr<MapInterface> &mapInterface, float screenDensityPpi) :
        mapInterface(mapInterface),
        conversionHelper(mapInterface->getCoordinateConverterHelper()),
        mapCoordinateSystem(mapInterface->getMapConfig().mapCoordinateSystem),
        screenDensityPpi(screenDensityPpi),
        screenPixelAsRealMeterFactor(0.0254 / screenDensityPpi),
        centerPosition(mapCoordinateSystem.identifier, 0, 0, 0) {
    auto mapConfig = mapInterface->getMapConfig();
    mapCoordinateSystem = mapConfig.mapCoordinateSystem;
    centerPosition.x = mapCoordinateSystem.bounds.topLeft.x +
                       0.5 * (mapCoordinateSystem.bounds.bottomRight.x - mapCoordinateSystem.bounds.topLeft.x);
    centerPosition.y = mapCoordinateSystem.bounds.topLeft.y +
                       0.5 * (mapCoordinateSystem.bounds.bottomRight.y - mapCoordinateSystem.bounds.topLeft.y);
    zoom = mapConfig.zoomMin;
}

void MapCamera2d::viewportSizeChanged() {
    notifyListeners();
}

void MapCamera2d::moveToCenterPositionZoom(const ::Coord &centerPosition, double zoom, bool animated) {
    if (animated) {
        beginAnimation(zoom, centerPosition);
    } else {
        this->zoom = zoom;
        this->centerPosition.x = centerPosition.x;
        this->centerPosition.y = centerPosition.y;
        notifyListeners();
    }
}

void MapCamera2d::moveToCenterPosition(const ::Coord &centerPosition, bool animated) {
    if (animated) {
        beginAnimation(zoom, centerPosition);
    } else {
        this->centerPosition.x = centerPosition.x;
        this->centerPosition.y = centerPosition.y;
        notifyListeners();
    }
}

::Coord MapCamera2d::getCenterPosition() {
    return centerPosition;
}

void MapCamera2d::setZoom(double zoom, bool animated) {
    if (animated) {
        beginAnimation(zoom, centerPosition);
    } else {
        this->zoom = zoom;
        notifyListeners();
    }
}

double MapCamera2d::getZoom() {
    return zoom;
}

void MapCamera2d::setPaddingLeft(float padding, bool animated) {
    paddingLeft = padding;
    mapInterface->invalidate();
}

void MapCamera2d::setPaddingRight(float padding, bool animated) {
    paddingRight = padding;
    mapInterface->invalidate();
}

void MapCamera2d::setPaddingTop(float padding, bool animated) {
    paddingTop = padding;
    mapInterface->invalidate();
}

void MapCamera2d::setPaddingBottom(float padding, bool animated) {
    paddingBottom = padding;
    mapInterface->invalidate();
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
    applyAnimationState();

    std::vector<float> newMvpMatrix(16, 0);

    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
    double zoomFactor = screenPixelAsRealMeterFactor * zoom;

    Coord renderCoordCenter = conversionHelper->convertToRenderSystem(centerPosition);

    Matrix::setIdentityM(newMvpMatrix, 0);


    Matrix::orthoM(newMvpMatrix, 0,
                   renderCoordCenter.x - 0.5 * sizeViewport.x,
                   renderCoordCenter.x + 0.5 * sizeViewport.x,
                   renderCoordCenter.y + 0.5 * sizeViewport.y,
                   renderCoordCenter.y - 0.5 * sizeViewport.y, -1, 1);


    Matrix::translateM(newMvpMatrix, 0, renderCoordCenter.x, renderCoordCenter.y, 0);

    Matrix::scaleM(newMvpMatrix, 0, 1 / zoomFactor, 1 / zoomFactor, 1);

    Matrix::rotateM(newMvpMatrix, 0.0, angle, 0.0, 0.0, 1.0);

    Matrix::translateM(newMvpMatrix, 0, -renderCoordCenter.x, -renderCoordCenter.y, 0);

    Matrix::translateM(newMvpMatrix, 0, (paddingLeft - paddingRight) * zoomFactor, (paddingTop - paddingBottom) * zoomFactor, 0);

    return newMvpMatrix;
}


RectCoord MapCamera2d::getVisibleRect() {
    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
    double zoomFactor = screenPixelAsRealMeterFactor * zoom;

    double halfWidth = sizeViewport.x * 0.5 * zoomFactor;
    double halfHeight = sizeViewport.y * 0.5 * zoomFactor;

    double sinAngle = std::abs(sin(angle * M_PI / 180.0));
    double cosAngle = std::abs(cos(angle * M_PI / 180.0));

    double deltaX = halfWidth * cosAngle + halfHeight * sinAngle;
    double deltaY = halfWidth * sinAngle + halfHeight * cosAngle;

    double topLeftX = centerPosition.x - deltaX;
    double topLeftY = centerPosition.y + deltaY;
    double bottomRightX = centerPosition.x + deltaX;
    double bottomRightY = centerPosition.y - deltaY;

    Coord topLeft = Coord(mapCoordinateSystem.identifier,
                          topLeftX,
                          topLeftY,
                          centerPosition.z);
    Coord bottomRight = Coord(mapCoordinateSystem.identifier,
                              bottomRightX,
                              bottomRightY,
                              centerPosition.z);
    return RectCoord(topLeft, bottomRight);
}

void MapCamera2d::notifyListeners() {
    auto visibleRect = getVisibleRect();
    for (auto listener: listeners) {
        listener->onVisibleBoundsChanged(visibleRect, zoom);
    }
}

bool MapCamera2d::onMove(const Vec2F &deltaScreen, bool confirmed, bool doubleClick) {
    if (!config.moveEnabled) return false;

    float dx = deltaScreen.x;
    float dy = deltaScreen.y;

    float sinAngle = sin(angle * M_PI / 180.0);
    float cosAngle = cos(angle * M_PI / 180.0);

    float leftDiff = (cosAngle * dx + sinAngle * dy);
    float topDiff = (-sinAngle * dx + cosAngle * dy);

    centerPosition.x -= leftDiff * zoom * screenPixelAsRealMeterFactor;
    centerPosition.y += topDiff * zoom * screenPixelAsRealMeterFactor;

    auto config = mapInterface->getMapConfig();
    auto bottomRight = config.mapCoordinateSystem.bounds.bottomRight;
    auto topLeft = config.mapCoordinateSystem.bounds.topLeft;

    centerPosition.x = std::min(centerPosition.x, bottomRight.x);
    centerPosition.x = std::max(centerPosition.x, topLeft.x);

    centerPosition.y = std::max(centerPosition.y, bottomRight.y);
    centerPosition.y = std::min(centerPosition.y, topLeft.y);

    notifyListeners();
    mapInterface->invalidate();
    return true;
}

bool MapCamera2d::onDoubleClick(const ::Vec2F &posScreen) {
    if (!config.doubleClickZoomEnabled) return false;

    auto targetZoom = zoom / 2;

    targetZoom = std::max(std::min(targetZoom, mapInterface->getMapConfig().zoomMin), mapInterface->getMapConfig().zoomMax);

    auto position = coordFromScreenPosition(posScreen);

    auto config = mapInterface->getMapConfig();
    auto bottomRight = config.mapCoordinateSystem.bounds.bottomRight;
    auto topLeft = config.mapCoordinateSystem.bounds.topLeft;

    position.x = std::min(position.x, bottomRight.x);
    position.x = std::max(position.x, topLeft.x);

    position.y = std::max(position.y, bottomRight.y);
    position.y = std::min(position.y, topLeft.y);


    beginAnimation(targetZoom, position);
    return true;
}

bool MapCamera2d::onTwoFingerMove(const std::vector<::Vec2F> &posScreenOld, const std::vector<::Vec2F> &posScreenNew) {
    if (!config.twoFingerZoomEnabled) return false;

    if (posScreenOld.size() >= 2) {
        double scaleFactor = Vec2FHelper::distance(posScreenNew[0], posScreenNew[1]) /
                             Vec2FHelper::distance(posScreenOld[0], posScreenOld[1]);
        zoom /= scaleFactor;

        zoom = std::max(std::min(zoom, mapInterface->getMapConfig().zoomMin), mapInterface->getMapConfig().zoomMax);

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

        auto mapConfig = mapInterface->getMapConfig();
        auto bottomRight = mapConfig.mapCoordinateSystem.bounds.bottomRight;
        auto topLeft = mapConfig.mapCoordinateSystem.bounds.topLeft;

        centerPosition.x = std::min(centerPosition.x, bottomRight.x);
        centerPosition.x = std::max(centerPosition.x, topLeft.x);

        centerPosition.y = std::max(centerPosition.y, bottomRight.y);
        centerPosition.y = std::min(centerPosition.y, topLeft.y);

        if (config.rotationEnabled) {
            float olda = atan2(posScreenOld[0].x - posScreenOld[1].x, posScreenOld[0].y - posScreenOld[1].y);
            float newa = atan2(posScreenNew[0].x - posScreenNew[1].x, posScreenNew[0].y - posScreenNew[1].y);
            angle = fmod((angle + (olda - newa) / M_PI * 180.0) + 360.0, 360.0);
        }

        notifyListeners();
        mapInterface->invalidate();
    }
    return true;
}


Coord MapCamera2d::coordFromScreenPosition(const ::Vec2F &posScreen) {
    Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
    double zoomFactor = screenPixelAsRealMeterFactor * zoom;

    double xDiffToCenter = posScreen.x - ((double) sizeViewport.x / 2.0);
    double yDiffToCenter = posScreen.y - ((double) sizeViewport.y / 2.0);

    return Coord(centerPosition.systemIdentifier,
                 centerPosition.x + (xDiffToCenter * zoomFactor),
                 centerPosition.y - (yDiffToCenter * zoomFactor),
                 centerPosition.z);
}


void MapCamera2d::beginAnimation(double zoom, Coord centerPosition) {
    cameraAnimation = std::make_optional<CameraAnimation>({
                                                                  this->centerPosition,
                                                                  this->zoom,
                                                                  centerPosition,
                                                                  zoom,
                                                                  DateHelper::currentTimeMillis(),
                                                                  300
                                                          });

    mapInterface->invalidate();
}

void MapCamera2d::applyAnimationState() {
    if (auto cameraAnimation = this->cameraAnimation) {
        long long currentTime = DateHelper::currentTimeMillis();
        double progress = (double) (currentTime - cameraAnimation->startTime) / cameraAnimation->duration;

        if (progress >= 1) {
            zoom = cameraAnimation->targetZoom;
            centerPosition.x = cameraAnimation->targetCenterPosition.x;
            centerPosition.y = cameraAnimation->targetCenterPosition.y;
            this->cameraAnimation = std::nullopt;
        } else {
            zoom = cameraAnimation->startZoom +
                   (cameraAnimation->targetZoom - cameraAnimation->startZoom) * std::pow(progress, 2);
            centerPosition.x = cameraAnimation->startCenterPosition.x +
                               (cameraAnimation->targetCenterPosition.x - cameraAnimation->startCenterPosition.x) *
                               std::pow(progress, 2);
            centerPosition.y = cameraAnimation->startCenterPosition.y +
                               (cameraAnimation->targetCenterPosition.y - cameraAnimation->startCenterPosition.y) *
                               std::pow(progress, 2);
        }
        notifyListeners();
        mapInterface->invalidate();
    }
}
