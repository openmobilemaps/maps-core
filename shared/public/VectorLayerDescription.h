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

#include "Value.h"
#include <vector>

enum VectorLayerType {
 raster, line, polygon, symbol
};

class VectorLayerDescription {
public:
    std::string identifier;
    std::string sourceId;
    int minZoom;
    int maxZoom;
    std::shared_ptr<Value<bool>> filter;

    virtual VectorLayerType getType() = 0;

protected:
public:
    VectorLayerDescription(std::string identifier,
                           std::string sourceId,
                           int minZoom,
                           int maxZoom,
                           std::shared_ptr<Value<bool>> filter):
    identifier(identifier),
    sourceId(sourceId),
    minZoom(minZoom),
    maxZoom(maxZoom),
    filter(filter) {}
};
