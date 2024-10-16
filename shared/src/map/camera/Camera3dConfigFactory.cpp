/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Camera3dConfigFactory.h"
#include "Camera3dConfig.h"

Camera3dConfig Camera3dConfigFactory::getRestorConfig() {
    static constexpr float minZoom = 200000000.0f;
    static constexpr float maxZoom = 4000000.0f;

    float pitchSwitch = (46000000.0f - minZoom) / (maxZoom - minZoom);

    CameraInterpolation pitchInterpolationValues({
        {0.0f, 0.0f},
        {pitchSwitch, 25.0f},
        {1.0f, 0.0f}
    });

    CameraInterpolation verticalDisplacementInterpolationValues({
        {0.0f, 0.0f},
        {pitchSwitch, 1.0f},
        {1.0f, 0.0f}
    });

    return Camera3dConfig("restor_config", true, std::nullopt, minZoom, maxZoom,
                          pitchInterpolationValues, verticalDisplacementInterpolationValues);
}
