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

#include "RenderPassStencilOptions.h"
#include "Value.h"
#include <memory>
#include <optional>
#include <string>

enum VectorLayerType {
 background, raster, line, polygon, circle, symbol, custom
};

class VectorLayerDescription {
public:
    std::string identifier;
    std::string source;
    std::string sourceLayer;
    int minZoom;
    int maxZoom;
    int sourceMinZoom;
    int sourceMaxZoom;
    std::shared_ptr<Value> filter;
    std::optional<int32_t> renderPassIndex;
    RenderPassStencilOptions renderPassStencilOptions;
    std::optional<std::string> maskSource;
    std::optional<std::string> maskSourceLayer;
    std::optional<std::string> maskLayerIdentifier;
    bool maskInverseRead = false;
    std::optional<std::string> maskTargetLayerIdentifier;
    bool multiselect;
    bool selfMasked;

    virtual VectorLayerType getType() = 0;

    virtual UsedKeysCollection getUsedKeys() const {
        UsedKeysCollection usedKeys;
        std::shared_ptr<Value> values[] = { filter, interactable };

        for (auto const &value: values) {
            if (!value) continue;
            auto const setKeys = value->getUsedKeys();
            usedKeys.includeOther(setKeys);
        }

        return usedKeys;
    }

    bool isInteractable(const EvaluationContext &context) {
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
                           int sourceMinZoom,
                           int sourceMaxZoom,
                           std::shared_ptr<Value> filter,
                           std::optional<int32_t> renderPassIndex,
                           std::shared_ptr<Value> interactable,
                           bool multiselect,
                           bool selfMasked):
            identifier(identifier),
            source(source),
            sourceLayer(sourceId),
            minZoom(minZoom),
            maxZoom(maxZoom),
            sourceMinZoom(sourceMinZoom),
            sourceMaxZoom(sourceMaxZoom),
            filter(filter),
            renderPassIndex(renderPassIndex),
            interactable(interactable),
            multiselect(multiselect),
            selfMasked(selfMasked) {}

    virtual ~VectorLayerDescription() = default;

    virtual std::unique_ptr<VectorLayerDescription> clone() = 0;

    void copyRenderPassOptionsTo(VectorLayerDescription &layerDescription) const {
        layerDescription.renderPassStencilOptions = renderPassStencilOptions;
        layerDescription.maskSource = maskSource;
        layerDescription.maskSourceLayer = maskSourceLayer;
        layerDescription.maskLayerIdentifier = maskLayerIdentifier;
        layerDescription.maskInverseRead = maskInverseRead;
        layerDescription.maskTargetLayerIdentifier = maskTargetLayerIdentifier;
    }
};
