//
// Created by Christoph Maurhofer on 05.02.2021.
//

#include "CoordinateConversionHelper.h"
#include "DefaultSystemToRenderConverter.h"

std::string CoordinateConversionHelper::RENDER_SYSTEM_ID = "render_system";

CoordinateConversionHelper::CoordinateConversionHelper(MapCoordinateSystem mapCoordinateSystem) {
    registerConverter(mapCoordinateSystem.identifier, RENDER_SYSTEM_ID,
                      std::make_shared<DefaultSystemToRenderConverter>(mapCoordinateSystem));
}

void CoordinateConversionHelper::registerConverter(const std::string &from, const std::string &to,
                                                   const std::shared_ptr<CoordinateConverterInterface> &converter) {
    //fromToConverterMap.insert(std::make_tuple(from, to), converter);
}

Coord CoordinateConversionHelper::convert(const std::string &to, const Coord &coordinate) {
    return Coord(to, 0, 0, 0);
}

Coord CoordinateConversionHelper::convertToRenderSystem(const Coord &coordinate) {
    return Coord(RENDER_SYSTEM_ID, 0, 0, 0);
}
