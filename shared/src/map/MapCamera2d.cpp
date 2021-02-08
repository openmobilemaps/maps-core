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
        screenPixelAsRealMeterFactor(1.0 / (254 * screenDensityPpi)),
        centerPosition(mapCoordinateSystem.identifier, 0, 0, 0){
    auto mapConfig = mapInterface->getMapConfig();
    mapCoordinateSystem = mapConfig.mapCoordinateSystem;
    centerPosition.x = mapCoordinateSystem.boundsLeft + 0.5 * (mapCoordinateSystem.boundsRight - mapCoordinateSystem.boundsLeft);
    centerPosition.y = mapCoordinateSystem.boundsBottom + 0.5 * (mapCoordinateSystem.boundsTop - mapCoordinateSystem.boundsBottom);
    zoom = mapConfig.zoomMin;
}

void MapCamera2d::moveToCenterPositionZoom(const ::Coord &centerPosition, double zoom, bool animated) {
    if (animated) {
      beginAnimation(zoom, centerPosition);
    } else {
        this->zoom = zoom;
        this->centerPosition.x = centerPosition.x;
        this->centerPosition.y = centerPosition.y;
    }
}

void MapCamera2d::moveToCenterPosition(const ::Coord &centerPosition, bool animated) {
    if (animated) {
      beginAnimation(zoom, centerPosition);
    } else {
      this->centerPosition.x = centerPosition.x;
      this->centerPosition.y = centerPosition.y;
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

    return newMvpMatrix;
}

bool MapCamera2d::onMove(const Vec2F &deltaScreen, bool confirmed, bool doubleClick) {
    centerPosition.x -= deltaScreen.x * zoom * screenPixelAsRealMeterFactor;
    centerPosition.y += deltaScreen.y * zoom * screenPixelAsRealMeterFactor;

    mapInterface->invalidate();
    return false;
}

bool MapCamera2d::onDoubleClick(const ::Vec2F &posScreen) {
    auto targetZoom = std::max(zoom / 2, mapInterface->getMapConfig().zoomMax);

    beginAnimation(targetZoom, coordFromScreenPosition(posScreen));
}

bool MapCamera2d::onTwoFingerMove(const std::vector<::Vec2F> &posScreenOld, const std::vector<::Vec2F> &posScreenNew) {
    if (posScreenOld.size() >= 2) {
        zoom /= Vec2FHelper::distance(posScreenNew[0], posScreenNew[1]) / Vec2FHelper::distance(posScreenOld[0], posScreenOld[1]);


        zoom = std::max(std::min(zoom, mapInterface->getMapConfig().zoomMin), mapInterface->getMapConfig().zoomMax);

        auto midpoint = Vec2FHelper::midpoint(posScreenNew[0], posScreenNew[1]);
        auto oldMidpoint = Vec2FHelper::midpoint(posScreenOld[0], posScreenOld[1]);

        centerPosition.x -= (midpoint.x - oldMidpoint.x) * zoom * screenPixelAsRealMeterFactor;
        centerPosition.y += (midpoint.y - oldMidpoint.y) * zoom * screenPixelAsRealMeterFactor;

        /*
        float olda = atan2(posScreenOld[0].x - posScreenOld[1].x, posScreenOld[0].y - posScreenOld[1].x);
        float newa = atan2(posScreenNew[0].x - posScreenNew[1].x, posScreenNew[0].y - posScreenNew[1].y);
        angle += (olda - newa) / M_PI * 180.0;*/


        mapInterface->invalidate();
    }
}


Coord MapCamera2d::coordFromScreenPosition(const ::Vec2F &posScreen) {
  Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
  double zoomFactor = screenPixelAsRealMeterFactor * zoom;

  double xDiffToCenter = posScreen.x - ((double)sizeViewport.x / 2.0);
  double yDiffToCenter = posScreen.y - ((double)sizeViewport.y / 2.0);

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
    double progress = (double)(currentTime - cameraAnimation->startTime) / cameraAnimation->duration;

    if (progress >= 1) {
      zoom = cameraAnimation->targetZoom;
      centerPosition.x = cameraAnimation->targetCenterPosition.x;
      centerPosition.y = cameraAnimation->targetCenterPosition.y;
      this->cameraAnimation = std::nullopt;
    } else {
        zoom = cameraAnimation->startZoom + (cameraAnimation->targetZoom - cameraAnimation->startZoom) * std::pow(progress, 2);
        centerPosition.x = cameraAnimation->startCenterPosition.x + (cameraAnimation->targetCenterPosition.x - cameraAnimation->startCenterPosition.x) * std::pow(progress, 2);
        centerPosition.y = cameraAnimation->startCenterPosition.y + (cameraAnimation->targetCenterPosition.y - cameraAnimation->startCenterPosition.y) * std::pow(progress, 2);
    }
    mapInterface->invalidate();
  }
}
