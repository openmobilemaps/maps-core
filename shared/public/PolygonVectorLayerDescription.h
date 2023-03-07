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
#include "ColorUtil.h"

class PolygonVectorStyle {
public:

    PolygonVectorStyle(std::shared_ptr<Value> fillColor = nullptr,
                       std::shared_ptr<Value> fillOpacity = nullptr):
    fillColor(fillColor),
    fillOpacity(fillOpacity) {}

    std::unordered_set<std::string> getUsedKeys() const {

        std::unordered_set<std::string> usedKeys;
        std::vector<std::shared_ptr<Value>> values = {
            fillColor, fillOpacity
        };

        for (auto const &value: values) {
            if (!value) continue;
            auto const setKeys = value->getUsedKeys();
            usedKeys.insert(setKeys.begin(), setKeys.end());
        }

        return usedKeys;
    };

    Color getFillColor(const EvaluationContext &context){
        static const Color defaultValue = ColorUtil::c(0, 0, 0, 1.0);
        return fillColor ? fillColor->evaluateOr(context, defaultValue) : defaultValue;
    }

    double getFillOpacity(const EvaluationContext &context){
        static const double defaultValue = 1.0;
        return fillOpacity ? fillOpacity->evaluateOr(context, defaultValue) : defaultValue;
    }

private:
    std::shared_ptr<Value> fillColor;
    std::shared_ptr<Value> fillOpacity;
};

class PolygonVectorLayerDescription: public VectorLayerDescription {
public:
    VectorLayerType getType() override { return VectorLayerType::polygon; };
    PolygonVectorStyle style;

    PolygonVectorLayerDescription(std::string identifier,
                                  std::string source,
                                  std::string sourceId,
                                  int minZoom,
                                  int maxZoom,
                                  std::shared_ptr<Value> filter,
                                  PolygonVectorStyle style,
                                  std::optional<int32_t> renderPassIndex):
    VectorLayerDescription(identifier, source, sourceId, minZoom, maxZoom, filter, renderPassIndex),
    style(style) {};

    std::unique_ptr<VectorLayerDescription> clone() override {
        return std::make_unique<PolygonVectorLayerDescription>(identifier, source, sourceId, minZoom, maxZoom,
                                                               filter ? filter->clone() : nullptr, style, renderPassIndex);
    }

    virtual std::unordered_set<std::string> getUsedKeys() const override {
        std::unordered_set<std::string> usedKeys;

        auto parentKeys = VectorLayerDescription::getUsedKeys();
        usedKeys.insert(parentKeys.begin(), parentKeys.end());

        auto styleKeys = style.getUsedKeys();
        usedKeys.insert(styleKeys.begin(), styleKeys.end());

        return usedKeys;
    };
};
