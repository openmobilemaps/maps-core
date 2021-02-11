#include "MapCamera2d.h"
#include "Tiled2dMapLayer.h"


Tiled2dMapLayer::Tiled2dMapLayer(const std::shared_ptr<MapInterface> &mapInterface,
                                 const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig)
        : mapInterface(mapInterface), layerConfig(layerConfig) {
}

void Tiled2dMapLayer::setSourceInterface(const std::shared_ptr<Tiled2dMapSourceInterface> &sourceInterface) {
    this->sourceInterface = sourceInterface;
}

void Tiled2dMapLayer::onAdded() {
    auto camera = std::dynamic_pointer_cast<MapCamera2d>(mapInterface->getCamera());
    if (camera) {
        camera->addListener(shared_from_this());
        onVisibleBoundsChanged(camera->getVisibileRect(), camera->getZoom());
    }
}

void Tiled2dMapLayer::onRemoved() {
    auto camera = std::dynamic_pointer_cast<MapCamera2d>(mapInterface->getCamera());
    if (camera) {
        camera->removeListener(shared_from_this());
    }
}

void Tiled2dMapLayer::hide() {
    isHidden = true;
}

void Tiled2dMapLayer::show() {
    isHidden = false;
}

void Tiled2dMapLayer::onVisibleBoundsChanged(const ::RectCoord &visibleBounds, double zoom) {
    sourceInterface->onVisibleBoundsChanged(visibleBounds, zoom);
}
