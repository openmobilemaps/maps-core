//
// Created by Christoph Maurhofer on 05.02.2021.
//

#include "CoordinateConversionHelper.h"
#include "DefaultSystemToRenderConverter.h"


CoordinateConversionHelper::CoordinateConversionHelper(MapCoordinateSystem mapCoordinateSystem): mapCoordinateSystemIdentier(mapCoordinateSystem.identifier) {
    registerConverter(mapCoordinateSystem.identifier, RENDER_SYSTEM_ID,
                      std::make_shared<DefaultSystemToRenderConverter>(mapCoordinateSystem));
}

void CoordinateConversionHelper::registerConverter(const std::string &from, const std::string &to,
                                                   const std::shared_ptr<CoordinateConverterInterface> &converter) {
    fromToConverterMap[{from, to}] = converter;
}

Coord CoordinateConversionHelper::convert(const std::string &to, const Coord &coordinate) {
    if (coordinate.systemIdentifier == to) {
        return coordinate;
    }

    // first try if we can directly convert
    if ( auto converter = fromToConverterMap[{coordinate.systemIdentifier, to}]) {
        return converter->convert(coordinate);
    }

    // if we can't convert directly we try if we can first convert to mapCoordinateSystemIdentier and from there to the wished system
    if ( auto converterMapCoordinateSystem = fromToConverterMap[{coordinate.systemIdentifier, mapCoordinateSystemIdentier}]) {
        if ( auto converterTo = fromToConverterMap[{mapCoordinateSystemIdentier, to}]) {
            return converterTo->convert(converterMapCoordinateSystem->convert(coordinate));
        }
    }

    throw std::invalid_argument("Could not find an eligible converter from: \'" + coordinate.systemIdentifier + "\' to \'" + to + "\'");
}

RectCoord CoordinateConversionHelper::convertRect(const std::string & to, const RectCoord & rect) {
    return RectCoord(convert(to, rect.topLeft), convert(to, rect.bottomRight));
}

RectCoord CoordinateConversionHelper::convertRectToRenderSystem(const RectCoord & rect) {
  return convertRect(RENDER_SYSTEM_ID, rect);
}

Coord CoordinateConversionHelper::convertToRenderSystem(const Coord &coordinate) {
    return convert(RENDER_SYSTEM_ID, coordinate);
}
