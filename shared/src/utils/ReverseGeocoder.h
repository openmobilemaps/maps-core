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
#include "ReverseGeocoderInterface.h"

class ReverseGeocoder: public ReverseGeocoderInterface {
public:
    ReverseGeocoder(const /*not-null*/ std::shared_ptr<::LoaderInterface> & loader, const std::string & tileUrlTemplate, int32_t zoomLevel);

    virtual std::vector<::VectorLayerFeatureCoordInfo> reverseGeocode(const ::Coord & coord, int64_t thresholdMeters) override;

    virtual std::optional<::VectorLayerFeatureCoordInfo> reverseGeocodeClosest(const ::Coord & coord, int64_t thresholdMeters) override;

private:
    double distance(const Coord &c1, const Coord &c2);
    
    const /*not-null*/ std::shared_ptr<::LoaderInterface> loader;
    const std::string tileUrlTemplate;
    int32_t zoomLevel;
};

