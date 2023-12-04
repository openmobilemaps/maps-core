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

    PolygonVectorStyle(PolygonVectorStyle &style)
    : fillColor(style.fillColor),
      fillOpacity(style.fillOpacity),
      fillPattern(style.fillPattern),
      blendMode(style.blendMode) {}

    UsedKeysCollection getUsedKeys() const {

        UsedKeysCollection usedKeys;
        std::shared_ptr<Value> values[] = {
            fillColor, fillOpacity, fillPattern
        };

        for (auto const &value: values) {
            if (!value) continue;
            auto const setKeys = value->getUsedKeys();
            usedKeys.includeOther(setKeys);
        }

        return usedKeys;
    };

    BlendMode getBlendMode(const EvaluationContext &context) {
        static const BlendMode defaultValue = BlendMode::NORMAL;
        return blendModeEvaluator.getResult(blendMode, context, defaultValue);
    }

    Color getFillColor(const EvaluationContext &context) {
        static const Color defaultValue = ColorUtil::c(0, 0, 0, 1.0);
        return fillColorEvaluator.getResult(fillColor, context, defaultValue);
    }

    double getFillOpacity(const EvaluationContext &context) {
        static const double defaultValue = 1.0;
        return fillOpacityEvaluator.getResult(fillOpacity, context, defaultValue);
    }

    std::string getFillPattern(const EvaluationContext &context) {
        static const std::string defaultValue = "";
        return fillPatternEvaluator.getResult(fillPattern, context, defaultValue);
    }

    bool hasPatternPotentially() {
        return fillPattern ? true : false;
    }

public:
    std::shared_ptr<Value> fillColor;
    std::shared_ptr<Value> fillOpacity;
    std::shared_ptr<Value> fillPattern;
    std::shared_ptr<Value> blendMode;
    
private:
    ValueEvaluator<Color> fillColorEvaluator;
    ValueEvaluator<double> fillOpacityEvaluator;
    ValueEvaluator<std::string> fillPatternEvaluator;
    ValueEvaluator<BlendMode> blendModeEvaluator;
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
        return std::make_unique<PolygonVectorLayerDescription>(identifier, source, sourceLayer, minZoom, maxZoom,
                                                               filter ? filter->clone() : nullptr, style, renderPassIndex,
                                                               interactable ? interactable->clone() : nullptr);
    }

    virtual UsedKeysCollection getUsedKeys() const override {
        UsedKeysCollection usedKeys;

        auto parentKeys = VectorLayerDescription::getUsedKeys();
        usedKeys.includeOther(parentKeys);

        auto styleKeys = style.getUsedKeys();
        usedKeys.includeOther(styleKeys);

        return usedKeys;
    };
};
