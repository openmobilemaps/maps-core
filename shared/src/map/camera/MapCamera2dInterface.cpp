/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "MapCamera3d.h"
#include "MapCamera2d.h"

std::shared_ptr<MapCamera2dInterface> MapCamera2dInterface::create(const std::shared_ptr<MapInterface> &mapInterface,
                                                                   float screenDensityPpi, bool is3D) {
    std::shared_ptr<MapCamera2dInterface> camera = nullptr;
    if (is3D) {
        camera = std::make_shared<MapCamera3d>(mapInterface, screenDensityPpi);
    } else {
        camera = std::make_shared<MapCamera2d>(mapInterface, screenDensityPpi);
    }
    return camera;
}
