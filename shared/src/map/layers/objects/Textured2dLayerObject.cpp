#include "Textured2dLayerObject.h"

Textured2dLayerObject::Textured2dLayerObject(std::shared_ptr<Rectangle2dInterface> rectangle,
                                              std::shared_ptr<AlphaShaderInterface> shader): rectangle(rectangle), shader(shader) {
    rectangle->setFrame(RectD(2660013.54, 1185171.98, 100, 100), RectD(0, 0, 1, 1));
}

std::vector<std::shared_ptr<RenderConfigInterface>> Textured2dLayerObject::getRenderConfig() {
    std::vector<std::shared_ptr<RenderConfigInterface>> empty;
    return empty;
}
