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

std::string CoordinateSystemIdentifiers::RENDERSYSTEM() { return "render_system"; };

// WGS 84 / Pseudo-Mercator
// https://epsg.io/3857
std::string CoordinateSystemIdentifiers::EPSG3857() { return "EPSG:3857"; };

// WGS 84
// https://epsg.io/4326
std::string CoordinateSystemIdentifiers::EPSG4326() { return "EPSG:4326"; };

// LV03+
// https://epsg.io/2056
std::string CoordinateSystemIdentifiers::EPSG2056() { return "EPSG:2056"; };

// CH1903 / LV03
// https://epsg.io/21781
std::string CoordinateSystemIdentifiers::EPSG21781() { return "EPSG:21781"; };

std::string CoordinateSystemIdentifiers::fromCrsIdentifier(const std::string & identifier) {
    if (identifier == "urn:ogc:def:crs:EPSG:3857" ||
        identifier == "urn:ogc:def:crs:EPSG::3857" ||
        identifier == "EPSG:3857")
            return EPSG3857();
    if (identifier == "urn:ogc:def:crs:EPSG:4326" ||
        identifier == "urn:ogc:def:crs:EPSG::4326" ||
        identifier == "EPSG:4326")
            return EPSG4326();
    if (identifier == "urn:ogc:def:crs:EPSG:2056" ||
        identifier == "urn:ogc:def:crs:EPSG::2056" ||
        identifier == "EPSG:2056")
            return EPSG2056();
    if (identifier == "urn:ogc:def:crs:EPSG:21781" ||
        identifier == "urn:ogc:def:crs:EPSG::21781" ||
        identifier == "EPSG:21781")
            return EPSG21781();

     return "";
};
