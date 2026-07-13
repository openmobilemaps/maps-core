/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "GeoDistanceHelper.h"
#include "CoordinateConversionHelperInterface.h"
#include "CoordinateSystemIdentifiers.h"
#include <cmath>

namespace {

double haversineDistanceMeters(const Coord &fromWgs84, const Coord &toWgs84) {
    constexpr double earthRadiusMeters = 6371000.0;

    const double latDistance = (toWgs84.y - fromWgs84.y) * M_PI / 180.0;
    const double lonDistance = (toWgs84.x - fromWgs84.x) * M_PI / 180.0;
    const double a = std::sin(latDistance / 2) * std::sin(latDistance / 2) +
                     std::cos(fromWgs84.y * M_PI / 180.0) * std::cos(toWgs84.y * M_PI / 180.0) *
                     std::sin(lonDistance / 2) * std::sin(lonDistance / 2);
    const double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    return earthRadiusMeters * c;
}

} // namespace

double GeoDistanceHelper::distanceMeters(const Coord &from, const Coord &to) {
    const auto coordinateConverter = CoordinateConversionHelperInterface::independentInstance();
    const auto fromWgs84 = coordinateConverter->convert(CoordinateSystemIdentifiers::EPSG4326(), from);
    const auto toWgs84 = coordinateConverter->convert(CoordinateSystemIdentifiers::EPSG4326(), to);
    return haversineDistanceMeters(fromWgs84, toWgs84);
}
