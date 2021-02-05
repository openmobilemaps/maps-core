#include "Polygon2dLayerObject.h"

Polygon2dLayerObject::Polygon2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                           const std::shared_ptr<Polygon2dInterface> &polygon,
                                           const std::shared_ptr<ColorShaderInterface> &shader) : conversionHelper(
        conversionHelper), polygon(polygon), shader(shader) {
    renderConfig = std::make_shared<RenderConfig>(polygon->asGraphicsObject(), 0);
}

std::vector<std::shared_ptr<RenderConfigInterface>> Polygon2dLayerObject::getRenderConfig() {
    return {renderConfig};
}

void Polygon2dLayerObject::setPositions(std::vector<Coord> positions) {
    std::vector<Vec2D> renderCoords;
    for (Coord mapCoord : positions) {
        Coord renderCoord = conversionHelper->convertToRenderSystem(mapCoord);
        renderCoords.push_back(Vec2D(renderCoord.x, renderCoord.y));
    }
    polygon->setPolygonPositions(renderCoords, {}, false);
}

std::shared_ptr<GraphicsObjectInterface> Polygon2dLayerObject::getPolygonObject() {
    return polygon->asGraphicsObject();
}

std::shared_ptr<ShaderProgramInterface> Polygon2dLayerObject::getShaderProgram() {
    return shader->asShaderProgramInterface();
}
