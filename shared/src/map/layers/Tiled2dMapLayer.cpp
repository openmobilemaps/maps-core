#include "Tiled2dMapLayer.h"


Tiled2dMapLayer::Tiled2dMapLayer(const std::shared_ptr<MapInterface> &mapInterface):
mapInterface(mapInterface) {

}

virtual std::shared_ptr<LayerInterface> Tiled2dMapLayer::asLayerInterface() {

}

virtual std::vector<std::shared_ptr<::RenderPassInterface>> Tiled2dMapLayer::buildRenderPasses() {

}

virtual std::string Tiled2dMapLayer::getIdentifier(){

}

virtual void Tiled2dMapLayer::pause() {

}

virtual void Tiled2dMapLayer::resume() {

}

virtual void Tiled2dMapLayer::hide() {

}

virtual void Tiled2dMapLayer::show() {

}
