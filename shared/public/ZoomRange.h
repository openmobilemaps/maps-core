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

#include <limits>

struct ZoomRange {
    double minZoom = std::numeric_limits<double>::infinity();
    double maxZoom = -std::numeric_limits<double>::infinity();

    bool isFull() const {
        return minZoom == 0.0 && maxZoom == std::numeric_limits<double>::infinity();
    }

    bool contains(double zoom) const {
        return zoom >= minZoom && zoom <= maxZoom;
    }

    void merge(double min, double max) {
        if (min < minZoom) { minZoom = min; }
        if (max > maxZoom) { maxZoom = max; }
    }

    void setFullRange() {
        minZoom = 0.0;
        maxZoom = std::numeric_limits<double>::infinity();
    }
};
