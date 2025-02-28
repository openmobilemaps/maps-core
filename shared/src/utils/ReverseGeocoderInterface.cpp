/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */


#include "ReverseGeocoder.h"

/*not-null*/ std::shared_ptr<ReverseGeocoderInterface> ReverseGeocoderInterface::create(const /*not-null*/ std::shared_ptr<::LoaderInterface> & loader, const std::string & tileUrlTemplate, int32_t zoomLevel) {
    return std::make_shared<ReverseGeocoder>(loader, tileUrlTemplate, zoomLevel);
}
