#include "Textured2dLayerObject.h"

Textured2dLayerObject::Textured2dLayerObject(std::shared_ptr<Rectangle2dInterface> rectangle,
                                              std::shared_ptr<AlphaShaderInterface> shader,
                                             const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper):
rectangle(rectangle),
shader(shader),
conversionHelper(conversionHelper),
renderConfig(std::make_shared<RenderConfig>(rectangle->asGraphicsObject(), 0)) {
}


void Textured2dLayerObject::setFrame(const ::RectD & frame) {
  rectangle->setFrame(frame, RectD(0, 0, 1, 1));
}

void Textured2dLayerObject::setPosition(const ::Coord & coord, double width, double height) {
  auto renderCoord = conversionHelper->convertToRenderSystem(coord);
  setFrame(RectD(renderCoord.x, renderCoord.y, width, height));
}

std::vector<std::shared_ptr<RenderConfigInterface>> Textured2dLayerObject::getRenderConfig() {
  return { renderConfig };
}

void Textured2dLayerObject::setAlpha(float alpha) {
  shader->updateAlpha(alpha);
}
