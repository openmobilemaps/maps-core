//
// Created by Christoph Maurhofer on 05.02.2021.
//

#pragma once

#include "Coord.h"
#include "RectCoord.h"
#include "CoordinateConversionHelperInterface.h"
#include "MapCoordinateSystem.h"
#include <unordered_map>
#include "HashedTuple.h"
#include "string"
#include <mutex>

class CoordinateConversionHelper : public CoordinateConversionHelperInterface {
public:
    static inline const std::string RENDER_SYSTEM_ID = "render_system";

    CoordinateConversionHelper(MapCoordinateSystem mapCoordinateSystem);

    void registerConverter(const std::string & from, const std::string & to, const std::shared_ptr<CoordinateConverterInterface> & converter);

    Coord convert(const std::string & to, const Coord & coordinate);

    RectCoord convertRect(const std::string & to, const RectCoord & rect);

    RectCoord convertRectToRenderSystem(const RectCoord & rect);

    Coord convertToRenderSystem(const Coord & coordinate);

private:
    std::unordered_map<std::tuple<std::string, std::string>, std::shared_ptr<CoordinateConverterInterface>> fromToConverterMap;

    std::string mapCoordinateSystemIdentier;

    std::recursive_mutex converterMutex;
};
