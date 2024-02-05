///*
// * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
// *
// *  This Source Code Form is subject to the terms of the Mozilla Public
// *  License, v. 2.0. If a copy of the MPL was not distributed with this
// *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
// *
// *  SPDX-License-Identifier: MPL-2.0
// */

#pragma once


#include "GeoJsonFeatureParserInterface.h"

class GeoJsonFeatureParser: public GeoJsonFeatureParserInterface {
public:
    GeoJsonFeatureParser();
    
    std::optional<std::vector<::VectorLayerFeatureInfo>> parse(const std::string & geoJson) override;

    std::optional<std::vector<GeoJsonPoint>> parseWithPointGeometry(const std::string & geoJson) override;
};
