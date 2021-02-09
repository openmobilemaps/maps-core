#include "Tiled2dMapLayer.h"


Tiled2dMapLayer::Tiled2dMapLayer(const std::shared_ptr<MapInterface> &mapInterface,
                                 const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                 const std::shared_ptr<Tiled2dMapSourceInterface> &source) :
        mapInterface(mapInterface),
        layerConfig(layerConfig),
        source(source) {

}

void Tiled2dMapLayer::pause() {
    // TODO: Adjust source
}

void Tiled2dMapLayer::resume() {
    // TODO: Adjust source
}

void Tiled2dMapLayer::hide() {
    isHidden = true;
}

void Tiled2dMapLayer::show() {
    isHidden = false;
}

void Tiled2dMapLayer::onVisibleBoundsChanged(const ::RectCoord &visibleBounds, double zoom) {
    source->onVisibleBoundsChanged(visibleBounds, zoom);
}
