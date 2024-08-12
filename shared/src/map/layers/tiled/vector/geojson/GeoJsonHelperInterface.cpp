/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "GeoJsonHelperInterface.h"
#include "GeoJsonHelper.h"

std::shared_ptr<GeoJsonHelperInterface> GeoJsonHelperInterface::independentInstance() {
    static std::shared_ptr<GeoJsonHelperInterface> singleton;
    if (singleton)
        return singleton;
    singleton = std::make_shared<GeoJsonHelper>();
    return singleton;
}
