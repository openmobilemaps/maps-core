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

#include "GeoJsonHelperInterface.h"
#include "GeoJsonPoint.h"
#include "json.h"
#include "CoordinateConversionHelperInterface.h"

class GeoJsonHelper : public GeoJsonHelperInterface {
public:
    GeoJsonHelper();

    std::string geoJsonStringFromFeatureInfo(const GeoJsonPoint & point) override;

    std::string geoJsonStringFromFeatureInfos(const std::vector<GeoJsonPoint> & points) override;

private:
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;

    nlohmann::json jsonFromFeatureInfo(const GeoJsonPoint & point);
};
