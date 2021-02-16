#pragma once
#include "LayerObjectInterface.h"
#include "RenderConfigInterface.h"
#include "Rectangle2dInterface.h"
#include "AlphaShaderInterface.h"
#include "Vec2D.h"
#include "Coord.h"
#include "RectCoord.h"
#include "CoordinateConversionHelperInterface.h"
#include "RenderConfig.h"

class Textured2dLayerObject: public LayerObjectInterface {
public:
    Textured2dLayerObject(std::shared_ptr<Rectangle2dInterface> rectangle,
                          std::shared_ptr<AlphaShaderInterface> shader,
                          const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper);

  virtual ~Textured2dLayerObject() override {}

  virtual void update() override;

  virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig() override;

  void setFrame(const ::RectD & frame);

  void setPosition(const ::Coord & coord, double width, double height);

  void setRectCoord(const ::RectCoord & rectCoord);

  void setAlpha(float alpha);

  std::shared_ptr<Rectangle2dInterface> getRectangleObject();

private:
    std::shared_ptr<Rectangle2dInterface> rectangle;
    std::shared_ptr<AlphaShaderInterface> shader;

    std::shared_ptr<RenderConfig> renderConfig;

    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
};
