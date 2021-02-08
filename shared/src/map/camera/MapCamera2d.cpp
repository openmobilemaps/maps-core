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
    scale = mapConfig.scaleMin;
}

void MapCamera2d::moveToCenterPositionScale(const ::Coord &centerPosition, double scale, bool animated) {
    if (animated) {
      beginAnimation(scale, centerPosition);
    } else {
      this->scale = scale;
      this->centerPosition.x = centerPosition.x;
      this->centerPosition.y = centerPosition.y;
      notifyListeners();
    }
}

void MapCamera2d::moveToCenterPosition(const ::Coord &centerPosition, bool animated) {
    if (animated) {
      beginAnimation(scale, centerPosition);
    } else {
      this->centerPosition.x = centerPosition.x;
      this->centerPosition.y = centerPosition.y;
      notifyListeners();
    }
}

::Coord MapCamera2d::getCenterPosition() {
    return centerPosition;
}

void MapCamera2d::setScale(double scale, bool animated) {
    if (animated) {
      beginAnimation(scale, centerPosition);
    } else {
      this->scale = scale;
      notifyListeners();
    }
}

double MapCamera2d::getScale() {
    return scale;
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
    double zoomFactor = screenPixelAsRealMeterFactor * scale;

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


void MapCamera2d::notifyListeners() {
  Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
  double zoomFactor = screenPixelAsRealMeterFactor * scale;
  Coord topLeft = Coord(mapCoordinateSystem.identifier,
                        centerPosition.x - ((double)sizeViewport.x / 2.0) * zoomFactor,
                        centerPosition.y + ((double)sizeViewport.y / 2.0) * zoomFactor,
                        centerPosition.z);
  Coord bottomRight = Coord(mapCoordinateSystem.identifier,
                            centerPosition.x + ((double)sizeViewport.x / 2.0) * zoomFactor,
                            centerPosition.y - ((double)sizeViewport.y / 2.0) * zoomFactor,
                            centerPosition.z);
  for (auto listener: listeners) {
    listener->onCenterPositionChanged(centerPosition, scale);
    listener->onVisibleBoundsChanged(topLeft, bottomRight, scale);
  }
}

bool MapCamera2d::onMove(const Vec2F &deltaScreen, bool confirmed, bool doubleClick) {
    centerPosition.x -= deltaScreen.x * scale * screenPixelAsRealMeterFactor;
    centerPosition.y += deltaScreen.y * scale * screenPixelAsRealMeterFactor;

    notifyListeners();
    mapInterface->invalidate();
    return true;
}

bool MapCamera2d::onDoubleClick(const ::Vec2F &posScreen) {
    auto targetScale = std::max(scale / 2, mapInterface->getMapConfig().scaleMax);

    beginAnimation(targetScale, coordFromScreenPosition(posScreen));
    return true;
}

bool MapCamera2d::onTwoFingerMove(const std::vector<::Vec2F> &posScreenOld, const std::vector<::Vec2F> &posScreenNew) {
    if (posScreenOld.size() >= 2) {
        scale /= Vec2FHelper::distance(posScreenNew[0], posScreenNew[1]) / Vec2FHelper::distance(posScreenOld[0], posScreenOld[1]);

        scale = std::max(std::min(scale, mapInterface->getMapConfig().scaleMin), mapInterface->getMapConfig().scaleMax);

        auto midpoint = Vec2FHelper::midpoint(posScreenNew[0], posScreenNew[1]);
        auto oldMidpoint = Vec2FHelper::midpoint(posScreenOld[0], posScreenOld[1]);

        centerPosition.x -= (midpoint.x - oldMidpoint.x) * scale * screenPixelAsRealMeterFactor;
        centerPosition.y += (midpoint.y - oldMidpoint.y) * scale * screenPixelAsRealMeterFactor;

        /*
        float olda = atan2(posScreenOld[0].x - posScreenOld[1].x, posScreenOld[0].y - posScreenOld[1].x);
        float newa = atan2(posScreenNew[0].x - posScreenNew[1].x, posScreenNew[0].y - posScreenNew[1].y);
        angle += (olda - newa) / M_PI * 180.0;*/


        notifyListeners();
        mapInterface->invalidate();
    }
    return true;
}


Coord MapCamera2d::coordFromScreenPosition(const ::Vec2F &posScreen) {
  Vec2I sizeViewport = mapInterface->getRenderingContext()->getViewportSize();
  double zoomFactor = screenPixelAsRealMeterFactor * scale;

  double xDiffToCenter = posScreen.x - ((double)sizeViewport.x / 2.0);
  double yDiffToCenter = posScreen.y - ((double)sizeViewport.y / 2.0);

  return Coord(centerPosition.systemIdentifier,
               centerPosition.x + (xDiffToCenter * zoomFactor),
               centerPosition.y - (yDiffToCenter * zoomFactor),
               centerPosition.z);
}


void MapCamera2d::beginAnimation(double scale, Coord centerPosition) {
  cameraAnimation = std::make_optional<CameraAnimation>({
    this->centerPosition,
    this->scale,
    centerPosition,
    scale,
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
        scale = cameraAnimation->targetScale;
      centerPosition.x = cameraAnimation->targetCenterPosition.x;
      centerPosition.y = cameraAnimation->targetCenterPosition.y;
      this->cameraAnimation = std::nullopt;
    } else {
        scale = cameraAnimation->startScale + (cameraAnimation->targetScale - cameraAnimation->startScale) * std::pow(progress, 2);
      centerPosition.x = cameraAnimation->startCenterPosition.x + (cameraAnimation->targetCenterPosition.x - cameraAnimation->startCenterPosition.x) * std::pow(progress, 2);
      centerPosition.y = cameraAnimation->startCenterPosition.y + (cameraAnimation->targetCenterPosition.y - cameraAnimation->startCenterPosition.y) * std::pow(progress, 2);
    }
    notifyListeners();
    mapInterface->invalidate();
  }
}
