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
 background, raster, line, polygon, symbol, custom
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

    virtual std::unordered_set<std::string> getUsedKeys() const {
        std::unordered_set<std::string> usedKeys;
        std::vector<std::shared_ptr<Value>> values = { filter, interactable };

        for (auto const &value: values) {
            if (!value) continue;
            auto const setKeys = value->getUsedKeys();
            usedKeys.insert(setKeys.begin(), setKeys.end());
        }

        return usedKeys;
    }

    bool isInteractable(const EvaluationContext &context){
        static const bool defaultValue = false;
        return interactable && interactable->evaluateOr(context, defaultValue);
    }

protected:
    std::shared_ptr<Value> interactable;

public:
    VectorLayerDescription(std::string identifier,
                           std::string source,
                           std::string sourceId,
                           int minZoom,
                           int maxZoom,
                           std::shared_ptr<Value> filter,
                           std::optional<int32_t> renderPassIndex,
                           std::shared_ptr<Value> interactable):
    identifier(identifier),
    source(source),
    sourceId(sourceId),
    minZoom(minZoom),
    maxZoom(maxZoom),
    filter(filter),
    renderPassIndex(renderPassIndex),
    interactable(interactable) {}

    virtual std::unique_ptr<VectorLayerDescription> clone() = 0;
};
