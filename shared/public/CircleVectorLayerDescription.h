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

#include "Color.h"
#include "ColorUtil.h"
#include "FeatureValueEvaluator.h"
#include "VectorLayerDescription.h"

class CircleVectorStyle {
public:
    CircleVectorStyle(std::shared_ptr<Value> circleColor = nullptr,
                      std::shared_ptr<Value> circleOpacity = nullptr,
                      std::shared_ptr<Value> circleRadius = nullptr,
                      std::shared_ptr<Value> circleStrokeColor = nullptr,
                      std::shared_ptr<Value> circleStrokeOpacity = nullptr,
                      std::shared_ptr<Value> circleStrokeWidth = nullptr,
                      std::shared_ptr<Value> blendMode = nullptr)
        : circleColorEvaluator(circleColor)
        , circleOpacityEvaluator(circleOpacity)
        , circleRadiusEvaluator(circleRadius)
        , circleStrokeColorEvaluator(circleStrokeColor)
        , circleStrokeOpacityEvaluator(circleStrokeOpacity)
        , circleStrokeWidthEvaluator(circleStrokeWidth)
        , blendModeEvaluator(blendMode) {}

    CircleVectorStyle(CircleVectorStyle &style)
        : circleColorEvaluator(style.circleColorEvaluator)
        , circleOpacityEvaluator(style.circleOpacityEvaluator)
        , circleRadiusEvaluator(style.circleRadiusEvaluator)
        , circleStrokeColorEvaluator(style.circleStrokeColorEvaluator)
        , circleStrokeOpacityEvaluator(style.circleStrokeOpacityEvaluator)
        , circleStrokeWidthEvaluator(style.circleStrokeWidthEvaluator)
        , blendModeEvaluator(style.blendModeEvaluator) {}

    UsedKeysCollection getUsedKeys() const {
        UsedKeysCollection usedKeys;
        std::shared_ptr<Value> values[] = {
            circleColorEvaluator.getValue(),
            circleOpacityEvaluator.getValue(),
            circleRadiusEvaluator.getValue(),
            circleStrokeColorEvaluator.getValue(),
            circleStrokeOpacityEvaluator.getValue(),
            circleStrokeWidthEvaluator.getValue(),
            blendModeEvaluator.getValue(),
        };

        for (auto const &value : values) {
            if (!value) {
                continue;
            }
            auto const setKeys = value->getUsedKeys();
            usedKeys.includeOther(setKeys);
        }

        return usedKeys;
    }

    BlendMode getBlendMode(const EvaluationContext &context) {
        static const BlendMode defaultValue = BlendMode::NORMAL;
        return blendModeEvaluator.getResult(context, defaultValue).value;
    }

    Color getCircleColor(const EvaluationContext &context) {
        static const Color defaultValue = ColorUtil::c(0, 0, 0, 1.0);
        return circleColorEvaluator.getResult(context, defaultValue).value;
    }

    double getCircleOpacity(const EvaluationContext &context) {
        static const double defaultValue = 1.0;
        return circleOpacityEvaluator.getResult(context, defaultValue).value;
    }

    double getCircleRadius(const EvaluationContext &context) {
        static const double defaultValue = 5.0;
        return circleRadiusEvaluator.getResult(context, defaultValue).value * context.dpFactor;
    }

    Color getCircleStrokeColor(const EvaluationContext &context) {
        static const Color defaultValue = ColorUtil::c(0, 0, 0, 1.0);
        return circleStrokeColorEvaluator.getResult(context, defaultValue).value;
    }

    double getCircleStrokeOpacity(const EvaluationContext &context) {
        static const double defaultValue = 1.0;
        return circleStrokeOpacityEvaluator.getResult(context, defaultValue).value;
    }

    double getCircleStrokeWidth(const EvaluationContext &context) {
        static const double defaultValue = 0.0;
        return circleStrokeWidthEvaluator.getResult(context, defaultValue).value * context.dpFactor;
    }

    FeatureValueEvaluator<Color> circleColorEvaluator;
    FeatureValueEvaluator<double> circleOpacityEvaluator;
    FeatureValueEvaluator<double> circleRadiusEvaluator;
    FeatureValueEvaluator<Color> circleStrokeColorEvaluator;
    FeatureValueEvaluator<double> circleStrokeOpacityEvaluator;
    FeatureValueEvaluator<double> circleStrokeWidthEvaluator;
    FeatureValueEvaluator<BlendMode> blendModeEvaluator;
};

class CircleVectorLayerDescription : public VectorLayerDescription {
public:
    VectorLayerType getType() override { return VectorLayerType::circle; };
    CircleVectorStyle style;
    float selectionSizeFactor;

    CircleVectorLayerDescription(std::string identifier,
                                 std::string source,
                                 std::string sourceId,
                                 int minZoom,
                                 int maxZoom,
                                 int sourceMinZoom,
                                 int sourceMaxZoom,
                                 std::shared_ptr<Value> filter,
                                 CircleVectorStyle style,
                                 std::optional<int32_t> renderPassIndex,
                                 std::shared_ptr<Value> interactable,
                                 bool multiselect,
                                 bool selfMasked,
                                 float selectionSizeFactor)
        : VectorLayerDescription(identifier, source, sourceId, minZoom, maxZoom, sourceMinZoom, sourceMaxZoom, filter,
                                 renderPassIndex, interactable, multiselect, selfMasked)
        , style(style)
        , selectionSizeFactor(selectionSizeFactor) {}

    std::unique_ptr<VectorLayerDescription> clone() override {
        return std::make_unique<CircleVectorLayerDescription>(
            identifier,
            source,
            sourceLayer,
            minZoom,
            maxZoom,
            sourceMinZoom,
            sourceMaxZoom,
            filter ? filter->clone() : nullptr,
            style,
            renderPassIndex,
            interactable ? interactable->clone() : nullptr,
            multiselect,
            selfMasked,
            selectionSizeFactor);
    }

    UsedKeysCollection getUsedKeys() const override {
        UsedKeysCollection usedKeys;

        auto parentKeys = VectorLayerDescription::getUsedKeys();
        usedKeys.includeOther(parentKeys);

        auto styleKeys = style.getUsedKeys();
        usedKeys.includeOther(styleKeys);

        return usedKeys;
    }
};
