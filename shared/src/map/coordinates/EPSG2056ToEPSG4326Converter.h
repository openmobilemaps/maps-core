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

/// Convert LV03+ to WGS 84 / Pseudo-Mercator
///  https://epsg.io/2056 to https://epsg.io/4326
class EPSG2056ToEPSG4326Converter : public CoordinateConverterInterface {
  public:
    EPSG2056ToEPSG4326Converter() {}

    virtual Coord convert(const Coord &coordinate) override {

        auto y = CHtoWGSlat(coordinate);
        auto x = CHtoWGSlng(coordinate);

        return Coord(getTo(), x, y, 0);
    }

    virtual std::string getFrom() override { return CoordinateSystemIdentifiers::EPSG2056(); }

    virtual std::string getTo() override { return CoordinateSystemIdentifiers::EPSG4326(); }

  private:
    // Convert CH y/x to WGS lat
    double CHtoWGSlat(const Coord &coordinate) {
        // Converts militar to civil and to unit = 1000km
        // Axiliary values (% Bern)
        double y_aux = (coordinate.x - 2600000) / 1000000;
        double x_aux = (coordinate.y - 1200000) / 1000000;

        // Process lat
        double lat = (16.9023892 + (3.238272 * x_aux)) - (0.270978 * pow(y_aux, 2)) - (0.002528 * pow(x_aux, 2)) -
                     (0.0447 * pow(y_aux, 2) * x_aux) - (0.0140 * pow(x_aux, 3));

        // Unit 10000" to 1 " and converts seconds to degrees (dec)
        lat = (lat * 100) / 36;

        return lat;
    }

    // Convert CH y/x to WGS long
    double CHtoWGSlng(const Coord &coordinate) {
        // Converts militar to civil and to unit = 1000km
        // Axiliary values (% Bern)
        double y_aux = (coordinate.x - 2600000) / 1000000;
        double x_aux = (coordinate.y - 1200000) / 1000000;

        // Process long
        double lng = (2.6779094 + (4.728982 * y_aux) + (0.791484 * y_aux * x_aux) + (0.1306 * y_aux * pow(x_aux, 2))) -
                     (0.0436 * pow(y_aux, 3));

        // Unit 10000" to 1 " and converts seconds to degrees (dec)
        lng = (lng * 100) / 36;

        return lng;
    }
};
