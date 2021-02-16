//
// Created by Christoph Maurhofer on 05.02.2021.
//

#include "CoordinateConversionHelper.h"
#include "DefaultSystemToRenderConverter.h"
#include "CoordinateConversionCommon.h"
#include "EPSG4326ToEPSG3857Converter.h"
#include "EPSG3857ToEPSG4326Converter.h"
#include "EPSG2056ToEPSG4326Converter.h"
#include "EPSG4326ToEPSG2056Converter.h"


CoordinateConversionHelper::CoordinateConversionHelper(MapCoordinateSystem mapCoordinateSystem): mapCoordinateSystemIdentier(mapCoordinateSystem.identifier) {

    registerConverter(std::make_shared<DefaultSystemToRenderConverter>(mapCoordinateSystem));

    registerConverter(std::make_shared<EPSG4326ToEPSG3857Converter>());
    registerConverter(std::make_shared<EPSG3857ToEPSG4326Converter>());
    registerConverter(std::make_shared<EPSG2056ToEPSG4326Converter>());
    registerConverter(std::make_shared<EPSG4326ToEPSG2056Converter>());
}

void CoordinateConversionHelper::registerConverter(const std::shared_ptr<CoordinateConverterInterface> &converter) {
    std::lock_guard<std::recursive_mutex> lock(converterMutex);
    fromToConverterMap[{converter->getFrom(), converter->getTo()}] = converter;
    precomputeConverterHelper();
}

Coord CoordinateConversionHelper::convert(const std::string &to, const Coord &coordinate) {
    if (coordinate.systemIdentifier == to) {
        return coordinate;
    }

    // first try if we can directly convert
    if ( auto const &converter = fromToConverterMap[{coordinate.systemIdentifier, to}]) {
        return converter->convert(coordinate);
    }

    if ( converterHelper.count({coordinate.systemIdentifier, to}) != 0 ) {
        auto const &converterChain = converterHelper[{coordinate.systemIdentifier, to}];
        auto intermediateCoord = coordinate;
        for (auto const &converter: converterChain) {
            intermediateCoord = converter->convert(intermediateCoord);
        }
        if (intermediateCoord.systemIdentifier != to) {
            throw std::logic_error("converterHelper contains invalid entry");
        }
        return intermediateCoord;
    }

    throw std::invalid_argument("Could not find an eligible converter from: \'" + coordinate.systemIdentifier + "\' to \'" + to + "\'");
}

RectCoord CoordinateConversionHelper::convertRect(const std::string & to, const RectCoord & rect) {
    return RectCoord(convert(to, rect.topLeft), convert(to, rect.bottomRight));
}

RectCoord CoordinateConversionHelper::convertRectToRenderSystem(const RectCoord & rect) {
  return convertRect(CoordinateConversionCommon::RENDER_SYSTEM_ID, rect);
}

Coord CoordinateConversionHelper::convertToRenderSystem(const Coord &coordinate) {
    return convert(CoordinateConversionCommon::RENDER_SYSTEM_ID, coordinate);
}

void CoordinateConversionHelper::precomputeConverterHelper() {
    converterHelper.clear();

    for (auto const &converterFirst: fromToConverterMap) {
        for (auto const &converterSecond: fromToConverterMap) {
            // if output of first converter is inpute of second
            auto from = converterFirst.second->getFrom();
            auto to = converterSecond.second->getTo();
            if (converterFirst.second->getTo() == converterSecond.second->getFrom() &&
                from != to &&
                fromToConverterMap.count({from, to}) == 0) {
                converterHelper[{from, to}] = {converterFirst.second, converterSecond.second};
            }
        }
    }
}
