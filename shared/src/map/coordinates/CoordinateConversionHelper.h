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
#include <vector>

class CoordinateConversionHelper : public CoordinateConversionHelperInterface {
public:

    CoordinateConversionHelper(MapCoordinateSystem mapCoordinateSystem);

    virtual void registerConverter(const std::shared_ptr<CoordinateConverterInterface> & converter) override;

    virtual Coord convert(const std::string & to, const Coord & coordinate) override;

    virtual RectCoord convertRect(const std::string & to, const RectCoord & rect) override;

    virtual RectCoord convertRectToRenderSystem(const RectCoord & rect) override;

    virtual Coord convertToRenderSystem(const Coord & coordinate) override;

private:
    std::unordered_map<std::tuple<std::string, std::string>, std::shared_ptr<CoordinateConverterInterface>> fromToConverterMap;

    std::unordered_map<std::tuple<std::string, std::string>, std::vector<std::shared_ptr<CoordinateConverterInterface>>> converterHelper;

    std::string mapCoordinateSystemIdentier;

    std::recursive_mutex converterMutex;

    void precomputeConverterHelper();
};
