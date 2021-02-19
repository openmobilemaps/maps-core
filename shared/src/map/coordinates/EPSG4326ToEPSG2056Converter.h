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

/// Convert WGS 84 / Pseudo-Mercator to LV03
///  https://epsg.io/4326 to https://epsg.io/2056
class EPSG4326ToEPSG2056Converter : public CoordinateConverterInterface {
  public:
    EPSG4326ToEPSG2056Converter() {}

    virtual Coord convert(const Coord &coordinate) override {

        // Converts degrees dec to sex
        double lat = DECtoSEX(coordinate.y);
        double lng = DECtoSEX(coordinate.x);

        // Converts degrees to seconds (sex)
        lat = DEGtoSEC(lat);
        lng = DEGtoSEC(lng);

        // Axiliary values (% Bern)
        double lat_aux = (lat - 169028.66) / 10000.;
        double lng_aux = (lng - 26782.5) / 10000.;

        double x = 600072.37 + 211455.93 * lng_aux - 10938.51 * lng_aux * lat_aux - 0.36 * lng_aux * (lat_aux * lat_aux) -
                   44.54 * (lng_aux * lng_aux * lng_aux);

        double y = 200147.07 + 308807.95 * lat_aux + 3745.25 * lng_aux * lng_aux + 76.63 * lat_aux * lat_aux -
                   194.56 * lng_aux * lng_aux * lat_aux + 119.79 * lat_aux * lat_aux * lat_aux;

        y += 1000000;
        x += 2000000;

        return Coord(getTo(), x, y, 0);
    }

    virtual std::string getFrom() override { return CoordinateSystemIdentifiers::EPSG4326(); }

    virtual std::string getTo() override { return CoordinateSystemIdentifiers::EPSG2056(); }

  private:
    double DECtoSEX(double angle) {
        // Extract DMS
        double deg = angle;
        double min = (angle - deg) * 60;
        double sec = (((angle - deg) * 60) - min) * 60;

        // Result in degrees sex (dd.mmss)
        return deg + min / 100 + sec / 10000;
    }

    // Convert Degrees angle to seconds
    double DEGtoSEC(double angle) {
        // Extract DMS
        double deg = angle;
        double min = (angle - deg) * 100;
        double sec = (((angle - deg) * 100) - min) * 100;

        // Result in degrees sex (dd.mmss)
        return sec + min * 60 + deg * 3600;
    }
};
