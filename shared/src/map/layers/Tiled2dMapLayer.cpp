#include "Tiled2dMapLayer.h"


Tiled2dMapLayer::Tiled2dMapLayer(const std::shared_ptr<MapInterface> &mapInterface):
mapInterface(mapInterface) {

}

std::shared_ptr<LayerInterface> Tiled2dMapLayer::asLayerInterface() {
  return nullptr;
}

std::vector<std::shared_ptr<::RenderPassInterface>> Tiled2dMapLayer::buildRenderPasses() {
  return {};
}

std::string Tiled2dMapLayer::getIdentifier(){
  return "identifier";
}

void Tiled2dMapLayer::pause() {

}

void Tiled2dMapLayer::resume() {

}

void Tiled2dMapLayer::hide() {

}

void Tiled2dMapLayer::show() {

}
