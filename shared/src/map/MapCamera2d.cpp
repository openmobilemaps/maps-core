#include "MapCamera2d.h"
#include "Vec2D.h"

MapCamera2d::MapCamera2d(float screenDensityPpi) {

}

void MapCamera2d::moveToCenterPosition(const ::Vec2D & position, double zoom, bool animated) {

}

void MapCamera2d::moveToCenterPositon(const ::Vec2D & position, bool animated) {

}

::Vec2D MapCamera2d::getCenterPosition() {
    Vec2D(0.0 ,0.0);
}

void MapCamera2d::setZoom(double zoom, bool animated) {

}

double MapCamera2d::getZoom() {
    return 1.0;
}

void MapCamera2d::setPaddingLeft(float padding) {

}

void MapCamera2d::setPaddingRight(float padding) {

}

void MapCamera2d::setPaddingTop(float padding) {

}

void MapCamera2d::setPaddingBottom(float padding) {

}

void MapCamera2d::addListener(const std::shared_ptr<MapCamera2dListenerInterface> & listener) {

}

void MapCamera2d::removeListener(const std::shared_ptr<MapCamera2dListenerInterface> & listener) {

}

std::shared_ptr<::CameraInterface> MapCamera2d::asCameraIntercace() {
    return nullptr;
}
