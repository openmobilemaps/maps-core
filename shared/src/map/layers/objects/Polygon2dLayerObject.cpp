#include "Polygon2dLayerObject.h"

Polygon2dLayerObject::Polygon2dLayerObject(std::shared_ptr<Polygon2dInterface> polygon, std::shared_ptr<ColorShaderInterface> shader): polygon(polygon), shader(shader) {
    renderConfig = std::make_shared<RenderConfig>(polygon->asGraphicsObject(), 0);
}

std::vector<std::shared_ptr<RenderConfigInterface>> Polygon2dLayerObject::getRenderConfig() {
    return {renderConfig};
}

void Polygon2dLayerObject::setPositions(std::vector<Vec2D> positions) {
    polygon->setPolygonPositions(positions, {}, true);
}

std::shared_ptr<GraphicsObjectInterface> Polygon2dLayerObject::getPolygonObject() {
    return polygon->asGraphicsObject();
}

std::shared_ptr<ShaderProgramInterface> Polygon2dLayerObject::getShaderProgram() {
    return shader->asShaderProgramInterface();
}
