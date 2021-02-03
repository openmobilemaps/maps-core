#include "Polygon2dLayerObject.h"

Polygon2dLayerObject::Polygon2dLayerObject(std::shared_ptr<Polygon2dInterface> polygon, std::shared_ptr<ColorShaderInterface> shader): polygon(polygon), shader(shader) {
    renderConfig = std::make_shared<RenderConfig>(polygon->asGraphicsObject(), 0);
}

std::vector<std::shared_ptr<RenderConfigInterface>> Polygon2dLayerObject::getRenderConfig() {
    return {renderConfig};
}

void Polygon2dLayerObject::setPositions(std::vector<Vec2F> positions) {
    polygon->setPolygonPositions(positions, {}, true);
}
