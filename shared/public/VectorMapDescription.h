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
#include "Color.h"

class VectorMapSourceDescription {
public:
    std::string identifier;
    std::string vectorUrl;
    int minZoom;
    int maxZoom;

    VectorMapSourceDescription(std::string identifier,
                         std::string vectorUrl,
                         int minZoom,
                         int maxZoom):
    identifier(identifier), vectorUrl(vectorUrl), minZoom(minZoom), maxZoom(maxZoom) {}
};

class VectorMapDescription {
public:
    std::string identifier;
    std::vector<std::shared_ptr<VectorMapSourceDescription>> vectorSources;
    std::vector<std::shared_ptr<VectorLayerDescription>> layers;
    std::string spriteBaseUrl;

    VectorMapDescription(std::string identifier,
                         std::vector<std::shared_ptr<VectorMapSourceDescription>> vectorSources,
                         std::vector<std::shared_ptr<VectorLayerDescription>> layers,
                         std::string spriteBaseUrl):
    identifier(identifier), vectorSources(vectorSources), layers(layers), spriteBaseUrl(spriteBaseUrl) {}
};
