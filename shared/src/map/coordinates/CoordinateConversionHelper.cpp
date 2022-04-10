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
#include "RenderToDefaultSystemConverter.h"
#include "EPSG2056ToEPSG4326Converter.h"
#include "EPSG3857ToEPSG4326Converter.h"
#include "EPSG4326ToEPSG2056Converter.h"
#include "EPSG4326ToEPSG3857Converter.h"
#include "EPSG2056ToEPGS21781Converter.h"
#include "EPSG21781ToEPGS2056Converter.h"


/**
 * This instance is independent of the map and does not know about the rendering system.
 * It can not be used to convert coordinates into rendering space.
 */
std::shared_ptr<CoordinateConversionHelperInterface> CoordinateConversionHelperInterface::independentInstance() {
    static std::shared_ptr<CoordinateConversionHelperInterface> singleton;
    if (singleton) return singleton;
    singleton = std::make_shared<CoordinateConversionHelper>();
    return singleton;
}

CoordinateConversionHelper::CoordinateConversionHelper(MapCoordinateSystem mapCoordinateSystem)
        : mapCoordinateSystemIdentier(mapCoordinateSystem.identifier),
          renderSystemConverter(std::make_shared<DefaultSystemToRenderConverter>(mapCoordinateSystem)) {
    registerConverter(renderSystemConverter);
    registerConverter(std::make_shared<RenderToDefaultSystemConverter>(mapCoordinateSystem));
    addDefaultConverters();
}

/**
 * This instance is independent of the map and does not know about the rendering system.
 * It can not be used to convert coordinates into rendering space.
 */
CoordinateConversionHelper::CoordinateConversionHelper() {
    addDefaultConverters();
}


void CoordinateConversionHelper::addDefaultConverters() {
    registerConverter(std::make_shared<EPSG4326ToEPSG3857Converter>());
    registerConverter(std::make_shared<EPSG3857ToEPSG4326Converter>());
    registerConverter(std::make_shared<EPSG2056ToEPSG4326Converter>());
    registerConverter(std::make_shared<EPSG4326ToEPSG2056Converter>());
    registerConverter(std::make_shared<EPSG2056ToEPGS21781Converter>());
    registerConverter(std::make_shared<EPSG21781ToEPGS2056Converter>());
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

    const auto tuple = std::tuple<std::string, std::string>(coordinate.systemIdentifier, to);

    // first try if we can directly convert
    auto c = fromToConverterMap.find(tuple);
    if (c != fromToConverterMap.end()) {
        return c->second->convert(coordinate);
    }

    auto ch = converterHelper.find(tuple);
    if (ch != converterHelper.end()) {
        auto const &converterChain = ch->second;
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
    auto topLeft = convertToRenderSystem(rect.topLeft);
    auto bottomRight = convertToRenderSystem(rect.bottomRight);
    return RectCoord(topLeft, bottomRight);
}

Coord CoordinateConversionHelper::convertToRenderSystem(const Coord &coordinate) {
    if (coordinate.systemIdentifier == mapCoordinateSystemIdentier) {
        return renderSystemConverter->convert(coordinate);
    } else {
        return convert(CoordinateSystemIdentifiers::RENDERSYSTEM(), coordinate);
    }
}

QuadCoord CoordinateConversionHelper::convertQuad(const std::string &to, const QuadCoord &quad) {
    Coord topLeft = convert(to, quad.topLeft);
    Coord topRight = convert(to, quad.topRight);
    Coord bottomRight = convert(to, quad.bottomRight);
    Coord bottomLeft = convert(to, quad.bottomLeft);
    bool ltr = topRight.x > topLeft.x;
    bool ttb = bottomLeft.y > topLeft.y;
    if (ltr) {
        if (ttb) {
            return QuadCoord(topLeft, topRight, bottomRight, bottomLeft);
        } else {
            return QuadCoord(bottomLeft, bottomRight, topRight, topLeft);
        }
    } else {
        if (ttb) {
            return QuadCoord(topRight, topLeft, bottomLeft, bottomRight);
        } else {
            return QuadCoord(bottomRight, bottomLeft, topLeft, topRight);
        }
    }
}

QuadCoord CoordinateConversionHelper::convertQuadToRenderSystem(const QuadCoord &quad) {
    return convertQuad(CoordinateSystemIdentifiers::RENDERSYSTEM(), quad);
}

void CoordinateConversionHelper::precomputeConverterHelper() {
    converterHelper.clear();

    // two steps
    for (auto const &converterFirst : fromToConverterMap) {
        for (auto const &converterSecond : fromToConverterMap) {
            // if output of first converter is input of last
            auto from = converterFirst.second->getFrom();
            auto to = converterSecond.second->getTo();

            if (from != to &&
                fromToConverterMap.count({from, to}) == 0 && converterFirst.second->getTo() == converterSecond.second->getFrom()) {
                converterHelper[{from, to}] = {converterFirst.second, converterSecond.second};
            }
        }
    }

    // three steps
    for (auto const &converterFirst : fromToConverterMap) {
        for (auto const &converterSecond : fromToConverterMap) {
            for (auto const &converterThird : fromToConverterMap) {
                auto from = converterFirst.second->getFrom();
                auto to = converterThird.second->getTo();

                if (from != to && fromToConverterMap.count({from, to}) == 0 && converterHelper.count({from, to}) == 0 &&
                    converterFirst.second->getTo() == converterSecond.second->getFrom() &&
                    converterSecond.second->getTo() == converterThird.second->getFrom()) {
                    converterHelper[{from, to}] = {converterFirst.second, converterSecond.second, converterThird.second};
                }
            }
        }
    }

    // four steps
    for (auto const &converterFirst : fromToConverterMap) {
        for (auto const &converterSecond : fromToConverterMap) {
            for (auto const &converterThird : fromToConverterMap) {
                for (auto const &converterFourth : fromToConverterMap) {
                    auto from = converterFirst.second->getFrom();
                    auto to = converterFourth.second->getTo();

                    if (from != to &&
                        fromToConverterMap.count({from, to}) == 0 && converterHelper.count({from, to}) == 0 &&
                        converterFirst.second->getTo() == converterSecond.second->getFrom() &&
                        converterSecond.second->getTo() == converterThird.second->getFrom() &&
                        converterThird.second->getTo() == converterFourth.second->getFrom()) {
                        converterHelper[{from, to}] = {converterFirst.second, converterSecond.second, converterThird.second,
                                                       converterFourth.second};
                    }
                }
            }
        }
    }
}
