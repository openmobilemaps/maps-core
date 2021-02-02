#include "MapCamera2d.h"

std::shared_ptr<MapCamera2dInterface> MapCamera2dInterface::create() {
    return std::make_shared<MapCamera2d>();
}
