#include "MapCamera2d.h"

std::shared_ptr<MapCamera2dInterface> MapCamera2dInterface::create(const std::shared_ptr<MapInterface> &mapInterface,
                                                                   float screenDensityPpi) {
    return std::make_shared<MapCamera2d>(mapInterface, screenDensityPpi);
}
