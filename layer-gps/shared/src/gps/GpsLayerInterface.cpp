/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "GpsLayerInterface.h"
#include "GpsLayer.h"

std::shared_ptr<GpsLayerInterface> GpsLayerInterface::create(const GpsStyleInfo & styleInfo) {
    return std::make_shared<GpsLayer>(styleInfo);
}
