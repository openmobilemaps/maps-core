/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "Coord.h"
#include "CoordinateConversionHelper.h"
#include "CoordinateConverterInterface.h"
#include "CoordinateSystemIdentifiers.h"
#include "MapCoordinateSystem.h"
#include <cmath>

/// Convert LV03+ to WGS 84 / Pseudo-Mercator
///  https://epsg.io/2056 to https://epsg.io/4326
class EPSG2056ToEPSG4326Converter : public CoordinateConverterInterface {
  public:
    EPSG2056ToEPSG4326Converter() {}

    virtual Coord convert(const Coord &coordinate) override {

        const auto y = CHtoWGSlat(coordinate);
        const auto x = CHtoWGSlng(coordinate);
        const auto z = CHtoWGSalt(coordinate);

        return Coord(getTo(), x, y, z);
    }

    virtual int32_t getFrom() override { return CoordinateSystemIdentifiers::EPSG2056(); }

    virtual int32_t getTo() override { return CoordinateSystemIdentifiers::EPSG4326(); }

  private:
    // Convert CH y/x to WGS lat
    inline double CHtoWGSlat(const Coord &coordinate) {
        // Converts militar to civil and to unit = 1000km
        // Axiliary values (% Bern)
        const double y_aux = (coordinate.x - 2600000) / 1000000;
        const double x_aux = (coordinate.y - 1200000) / 1000000;

        // Process lat
        // Unit 10000" to 1 " and converts seconds to degrees (dec)
        const double lat = (((16.9023892 + (3.238272 * x_aux)) - (0.270978 * pow(y_aux, 2)) - (0.002528 * pow(x_aux, 2)) -
                     (0.0447 * pow(y_aux, 2) * x_aux) - (0.0140 * pow(x_aux, 3))) * 100) / 36;

        return lat;
    }

    // Convert CH y/x to WGS long
    inline double CHtoWGSlng(const Coord &coordinate) {
        // Converts militar to civil and to unit = 1000km
        // Axiliary values (% Bern)
        const double y_aux = (coordinate.x - 2600000) / 1000000;
        const double x_aux = (coordinate.y - 1200000) / 1000000;

        // Process long
        // Unit 10000" to 1 " and converts seconds to degrees (dec)
        const double lng = (((2.6779094 + (4.728982 * y_aux) + (0.791484 * y_aux * x_aux) + (0.1306 * y_aux * pow(x_aux, 2))) -
                     (0.0436 * pow(y_aux, 3))) * 100) / 36;

        return lng;
    }

    // Convert CH y/x to WGS long
    inline double CHtoWGSalt(const Coord &coordinate) {
        // Converts militar to civil and to unit = 1000km
        // Axiliary values (% Bern)
        const double y_aux = (coordinate.x - 2600000) / 1000000;
        const double x_aux = (coordinate.y - 1200000) / 1000000;

        // Process long
        return coordinate.z + 49.55 - (12.6 * y_aux) - (22.64 * x_aux);
    }
};
