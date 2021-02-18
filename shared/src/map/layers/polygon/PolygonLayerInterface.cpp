#include "PolygonLayer.h"

std::shared_ptr<PolygonLayerInterface> PolygonLayerInterface::create() { return std::make_shared<PolygonLayer>(); }
