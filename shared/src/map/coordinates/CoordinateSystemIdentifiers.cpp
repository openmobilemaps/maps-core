/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "CoordinateSystemIdentifiers.h"
#include <stdexcept>

int32_t CoordinateSystemIdentifiers::RENDERSYSTEM() { return 0; };

// WGS 84 / Pseudo-Mercator
// https://epsg.io/3857
int32_t CoordinateSystemIdentifiers::EPSG3857() { return 3857; };

// WGS 84
// https://epsg.io/4326
int32_t CoordinateSystemIdentifiers::EPSG4326() { return 4326; };

// LV03+
// https://epsg.io/2056
int32_t CoordinateSystemIdentifiers::EPSG2056() { return 2056; };

// CH1903 / LV03
// https://epsg.io/21781
int32_t CoordinateSystemIdentifiers::EPSG21781() { return 21781; };

// Unit Sphere Polar
// x, y, z with reference to earth as unit sphere
int32_t CoordinateSystemIdentifiers::UnitSphere() {
    return 0x80000000 | 1;
}

int32_t CoordinateSystemIdentifiers::fromCrsIdentifier(const std::string &identifier) {
    if (identifier == "urn:ogc:def:crs:EPSG:3857" || identifier == "urn:ogc:def:crs:EPSG::3857" || identifier == "EPSG:3857" ||
        identifier == "urn:ogc:def:crs:EPSG:6.3:3857" || identifier == "urn:ogc:def:crs:EPSG:6.18.3:3857" ||
        identifier == "urn:ogc:def:crs:EPSG:6.18:3:3857")
        return EPSG3857();
    if (identifier == "urn:ogc:def:crs:EPSG:4326" || identifier == "urn:ogc:def:crs:EPSG::4326" || identifier == "EPSG:4326" ||
        identifier == "urn:ogc:def:crs:OGC:1.3:CRS84")
        return EPSG4326();
    if (identifier == "urn:ogc:def:crs:EPSG:2056" || identifier == "urn:ogc:def:crs:EPSG::2056" ||
        identifier == "urn:ogc:def:crs:EPSG:6.3:2056" || identifier == "EPSG:2056")
        return EPSG2056();
    if (identifier == "urn:ogc:def:crs:EPSG:21781" || identifier == "urn:ogc:def:crs:EPSG::21781" || identifier == "EPSG:21781")
        return EPSG21781();

    throw std::invalid_argument("Unsupported CRS identifier: " + identifier);
}

double CoordinateSystemIdentifiers::unitToMeterFactor(int32_t coordinateSystemIdentifier) {
    if (coordinateSystemIdentifier == CoordinateSystemIdentifiers::EPSG3857() ||
        coordinateSystemIdentifier == CoordinateSystemIdentifiers::EPSG2056() ||
        coordinateSystemIdentifier == CoordinateSystemIdentifiers::EPSG21781()) {
        return 1.0;
    } else if (coordinateSystemIdentifier == CoordinateSystemIdentifiers::EPSG4326()) {
        return 360.0 / 40075017.0;
    } else {
        throw std::invalid_argument("Unsupported coordinate system identifier: " + std::to_string(coordinateSystemIdentifier));
    }
}
