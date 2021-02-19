/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */


#include "CoordinateConversionHelper.h"
#include "CoordinateSystemIdentifiers.h"
#include "DefaultSystemToRenderConverter.h"
#include "EPSG2056ToEPSG4326Converter.h"
#include "EPSG3857ToEPSG4326Converter.h"
#include "EPSG4326ToEPSG2056Converter.h"
#include "EPSG4326ToEPSG3857Converter.h"

CoordinateConversionHelper::CoordinateConversionHelper(MapCoordinateSystem mapCoordinateSystem)
    : mapCoordinateSystemIdentier(mapCoordinateSystem.identifier) {

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
    if (auto const &converter = fromToConverterMap[{coordinate.systemIdentifier, to}]) {
        return converter->convert(coordinate);
    }

    if (converterHelper.count({coordinate.systemIdentifier, to}) != 0) {
        auto const &converterChain = converterHelper[{coordinate.systemIdentifier, to}];
        auto intermediateCoord = coordinate;
        for (auto const &converter : converterChain) {
            intermediateCoord = converter->convert(intermediateCoord);
        }
        if (intermediateCoord.systemIdentifier != to) {
            throw std::logic_error("converterHelper contains invalid entry");
        }
        return intermediateCoord;
    }

    throw std::invalid_argument("Could not find an eligible converter from: \'" + coordinate.systemIdentifier + "\' to \'" + to +
                                "\'");
}

RectCoord CoordinateConversionHelper::convertRect(const std::string &to, const RectCoord &rect) {
    return RectCoord(convert(to, rect.topLeft), convert(to, rect.bottomRight));
}

RectCoord CoordinateConversionHelper::convertRectToRenderSystem(const RectCoord &rect) {
    return convertRect(CoordinateSystemIdentifiers::RENDERSYSTEM(), rect);
}

Coord CoordinateConversionHelper::convertToRenderSystem(const Coord &coordinate) {
    return convert(CoordinateSystemIdentifiers::RENDERSYSTEM(), coordinate);
}

void CoordinateConversionHelper::precomputeConverterHelper() {
    converterHelper.clear();

    // two steps
    for (auto const &converterFirst : fromToConverterMap) {
        for (auto const &converterSecond : fromToConverterMap) {
            // if output of first converter is inpute of second
            auto from = converterFirst.second->getFrom();
            auto to = converterSecond.second->getTo();

            if (converterFirst.second->getTo() == converterSecond.second->getFrom() && from != to &&
                fromToConverterMap.count({from, to}) == 0) {
                converterHelper[{from, to}] = {converterFirst.second, converterSecond.second};
            }
        }
    }

    // three steps
    for (auto const &converterFirst : fromToConverterMap) {
        for (auto const &converterSecond : fromToConverterMap) {
            for (auto const &converterThird : fromToConverterMap) {
                // if output of first converter is inpute of second
                auto from = converterFirst.second->getFrom();
                auto to = converterThird.second->getTo();

                if (converterFirst.second->getTo() == converterSecond.second->getFrom() &&
                    converterSecond.second->getTo() == converterThird.second->getFrom() && from != to &&
                    fromToConverterMap.count({from, to}) == 0 && converterHelper.count({from, to}) == 0) {
                    converterHelper[{from, to}] = {converterFirst.second, converterSecond.second, converterThird.second};
                }
            }
        }
    }

    // three steps
}
