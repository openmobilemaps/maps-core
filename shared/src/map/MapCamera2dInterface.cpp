#include "MapCamera2d.h"

std::shared_ptr<MapCamera2dInterface> MapCamera2dInterface::create(float screenDensityPpi) {
    return std::make_shared<MapCamera2d>(screenDensityPpi);
}
