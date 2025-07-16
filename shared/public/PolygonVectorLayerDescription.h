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
#include "ValueEvaluator.h"

class PolygonVectorStyle {
public:

    PolygonVectorStyle(std::shared_ptr<Value> fillColor,
                       std::shared_ptr<Value> fillOpacity,
                       std::shared_ptr<Value> fillPattern,
                       std::shared_ptr<Value> blendMode,
                       bool fadeInPattern,
                       std::shared_ptr<Value> stripeWidth):
    fillColorEvaluator(fillColor),
    fillOpacityEvaluator(fillOpacity),
    fillPatternEvaluator(fillPattern),
    blendModeEvaluator(blendMode),
    fadeInPattern(fadeInPattern),
    stripeWidthEvaluator(stripeWidth) {}

    PolygonVectorStyle(PolygonVectorStyle &style)
    : fillColorEvaluator(style.fillColorEvaluator),
      fillOpacityEvaluator(style.fillOpacityEvaluator),
      fillPatternEvaluator(style.fillPatternEvaluator),
      blendModeEvaluator(style.blendModeEvaluator),
      fadeInPattern(style.fadeInPattern),
      stripeWidthEvaluator(style.stripeWidthEvaluator) {}

    UsedKeysCollection getUsedKeys() const {

        UsedKeysCollection usedKeys;
        std::shared_ptr<Value> values[] = {
            fillColorEvaluator.getValue(), fillOpacityEvaluator.getValue(), fillPatternEvaluator.getValue(), stripeWidthEvaluator.getValue()
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
        return blendModeEvaluator.getResult(context, defaultValue).value;
    }

    Color getFillColor(const EvaluationContext &context) {
        static const Color defaultValue = ColorUtil::c(0, 0, 0, 1.0);
        return fillColorEvaluator.getResult(context, defaultValue).value;
    }

    double getFillOpacity(const EvaluationContext &context) {
        static const double defaultValue = 1.0;
        return fillOpacityEvaluator.getResult(context, defaultValue).value;
    }

    std::string getFillPattern(const EvaluationContext &context) {
        static const std::string defaultValue = "";
        return fillPatternEvaluator.getResult(context, defaultValue).value;
    }

    bool hasPatternPotentially() {
        return fillPatternEvaluator.getValue() ? true : false;
    }

    std::vector<float> getStripeWidth(const EvaluationContext &context) {
        static const std::vector<float> defaultValue = {1.0, 1.0};
        auto stripeInfo = stripeWidthEvaluator.getResult(context, defaultValue).value;
        for (int i = 0; i < stripeInfo.size(); ++i) {
            stripeInfo[i] = stripeInfo[i] * context.dpFactor;
        }
        return stripeInfo;
    }

    bool isStripedPotentially() {
        return stripeWidthEvaluator.getValue() ? true : false;
    }

    bool hasFadeInPattern() {
        return fadeInPattern;
    }

    bool fadeInPattern;

    ValueEvaluator<Color> fillColorEvaluator;
    ValueEvaluator<double> fillOpacityEvaluator;
    ValueEvaluator<std::string> fillPatternEvaluator;
    ValueEvaluator<BlendMode> blendModeEvaluator;
    ValueEvaluator<std::vector<float>> stripeWidthEvaluator;
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
                                  int sourceMinZoom,
                                  int sourceMaxZoom,
                                  std::shared_ptr<Value> filter,
                                  PolygonVectorStyle style,
                                  std::optional<int32_t> renderPassIndex,
                                  std::shared_ptr<Value> interactable,
                                  bool multiselect,
                                  bool selfMasked):
    VectorLayerDescription(identifier, source, sourceId, minZoom, maxZoom, sourceMinZoom, sourceMaxZoom, filter, renderPassIndex, interactable, multiselect, selfMasked),
    style(style) {};

    std::unique_ptr<VectorLayerDescription> clone() override {
        return std::make_unique<PolygonVectorLayerDescription>(identifier, source, sourceLayer, minZoom, maxZoom, sourceMinZoom, sourceMaxZoom,
                                                               filter ? filter->clone() : nullptr, style, renderPassIndex,
                                                               interactable ? interactable->clone() : nullptr, multiselect, selfMasked);
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
