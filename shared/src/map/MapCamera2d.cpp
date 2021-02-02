#include "MapCamera2d.h"
#include "Vec2F.h"

std::shared_ptr<MapCamera2dInterface> MapCamera2dInterface::create() {
    return std::make_shared<MapCamera2d>();
}

MapCamera2d::MapCamera2d() {

}

void MapCamera2d::moveToCenterPosition(const ::Vec2F & position, float zoom, bool animated) {

}

void MapCamera2d::moveToCenterPositon(const ::Vec2F & position, bool animated) {

}

::Vec2F MapCamera2d::getCenterPosition() {
    Vec2F(0.0f ,0.0f);
}

void MapCamera2d::setZoom(float zoom, bool animated) {

}

float MapCamera2d::getZoom() {
    return 1.0f;
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
