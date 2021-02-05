//
// Created by Christoph Maurhofer on 05.02.2021.
//

#pragma once

#include "Coord.h"
#include "CoordinateConversionHelperInterface.h"
#include "MapCoordinateSystem.h"
#include <unordered_map>
#include "HashedTouple.h"
#include "string"

class CoordinateConversionHelper : public CoordinateConversionHelperInterface {
public:
    static std::string RENDER_SYSTEM_ID;

    CoordinateConversionHelper(MapCoordinateSystem mapCoordinateSystem);

    void registerConverter(const std::string & from, const std::string & to, const std::shared_ptr<CoordinateConverterInterface> & converter);

    Coord convert(const std::string & to, const Coord & coordinate);

    Coord convertToRenderSystem(const Coord & coordinate);

private:
    std::unordered_map<std::tuple<std::string, std::string>, std::shared_ptr<CoordinateConverterInterface>> fromToConverterMap;

    std::string mapCoordinateSystemIdentier;

};
