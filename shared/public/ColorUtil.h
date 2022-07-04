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

#include "Color.h"

class ColorUtil {
public:
    static Color c(int r, int g, int b, float a = 0.0) {
        return Color(((float)r) / 255.0,((float)g) / 255.0,((float)b) / 255.0, a);
    }
};
