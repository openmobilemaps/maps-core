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

#include "VectorLayerDescription.h"

class VectorMapDescription {
public:
    std::string identifier;
    std::string vectorUrl;
    int minZoom;
    int maxZoom;
    std::vector<std::shared_ptr<VectorLayerDescription>> layers;

    VectorMapDescription(std::string identifier,
                         std::string vectorUrl,
                         int minZoom,
                         int maxZoom,
                         std::vector<std::shared_ptr<VectorLayerDescription>> layers):
    identifier(identifier), vectorUrl(vectorUrl), layers(layers), minZoom(minZoom), maxZoom(maxZoom) {}
};
