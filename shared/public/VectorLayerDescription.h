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
 background, raster, line, polygon, symbol
};

class VectorLayerDescription {
public:
    std::string identifier;
    std::string source;
    std::string sourceId;
    int minZoom;
    int maxZoom;
    std::shared_ptr<Value> filter;
    std::optional<int32_t> renderPassIndex;

    virtual VectorLayerType getType() = 0;

    virtual std::unordered_set<std::string> getUsedKeys() {
        return filter ? filter->getUsedKeys() : std::unordered_set<std::string>();
    };

protected:
public:
    VectorLayerDescription(std::string identifier,
                           std::string source,
                           std::string sourceId,
                           int minZoom,
                           int maxZoom,
                           std::shared_ptr<Value> filter,
                           std::optional<int32_t> renderPassIndex):
    identifier(identifier),
    source(source),
    sourceId(sourceId),
    minZoom(minZoom),
    maxZoom(maxZoom),
    filter(filter),
    renderPassIndex(renderPassIndex) {}
};
