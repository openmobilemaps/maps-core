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

    PolygonVectorStyle(std::shared_ptr<Value> fillColor,
                       std::shared_ptr<Value> fillOpacity,
                       std::shared_ptr<Value> fillPattern,
                       std::shared_ptr<Value> blendMode):
    fillColor(fillColor),
    fillOpacity(fillOpacity),
    fillPattern(fillPattern),
    blendMode(blendMode) {}

    std::unordered_set<std::string> getUsedKeys() const {

        std::unordered_set<std::string> usedKeys;
        std::vector<std::shared_ptr<Value>> values = {
            fillColor, fillOpacity, fillPattern
        };

        for (auto const &value: values) {
            if (!value) continue;
            auto const setKeys = value->getUsedKeys();
            usedKeys.insert(setKeys.begin(), setKeys.end());
        }

        return usedKeys;
    };

    BlendMode getBlendMode(const EvaluationContext &context) const {
        static const BlendMode defaultValue = BlendMode::NORMAL;
        return blendMode ? blendMode->evaluateOr(context, defaultValue) : defaultValue;
    }

    Color getFillColor(const EvaluationContext &context) const {
        static const Color defaultValue = ColorUtil::c(0, 0, 0, 1.0);
        return fillColor ? fillColor->evaluateOr(context, defaultValue) : defaultValue;
    }

    double getFillOpacity(const EvaluationContext &context) const {
        static const double defaultValue = 1.0;
        return fillOpacity ? fillOpacity->evaluateOr(context, defaultValue) : defaultValue;
    }

    std::string getFillPattern(const EvaluationContext &context) const {
        static const std::string defaultValue = "";
        return fillPattern ? fillPattern->evaluateOr(context, defaultValue) : defaultValue;
    }

    bool hasPatternPotentially() {
        return fillPattern ? true : false;
    }


private:
    std::shared_ptr<Value> fillColor;
    std::shared_ptr<Value> fillOpacity;
    std::shared_ptr<Value> fillPattern;
    std::shared_ptr<Value> blendMode;
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
                                  std::optional<int32_t> renderPassIndex,
                                  std::shared_ptr<Value> interactable):
    VectorLayerDescription(identifier, source, sourceId, minZoom, maxZoom, filter, renderPassIndex, interactable),
    style(style) {};

    std::unique_ptr<VectorLayerDescription> clone() override {
        return std::make_unique<PolygonVectorLayerDescription>(identifier, source, sourceId, minZoom, maxZoom,
                                                               filter ? filter->clone() : nullptr, style, renderPassIndex,
                                                               interactable ? interactable->clone() : nullptr);
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
