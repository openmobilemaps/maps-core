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
#include "LineCapType.h"
#include "FeatureValueEvaluator.h"
#include "LineJoinType.h"
#include "VectorLayerDescription.h"

class LineVectorStyle {
  public:
    LineVectorStyle(std::shared_ptr<Value> lineColor = nullptr, std::shared_ptr<Value> lineOpacity = nullptr,
                    std::shared_ptr<Value> lineWidth = nullptr, std::shared_ptr<Value> lineDashArray = nullptr,
                    std::shared_ptr<Value> lineBlur = nullptr, std::shared_ptr<Value> lineCap = nullptr,
                    std::shared_ptr<Value> lineJoin = nullptr, std::shared_ptr<Value> lineOffset = nullptr,
                    std::shared_ptr<Value> blendMode = nullptr, std::shared_ptr<Value> lineDotted = nullptr,
                    std::shared_ptr<Value> lineDottedSkew = nullptr)
        : lineColorEvaluator(lineColor)
        , lineOpacityEvaluator(lineOpacity)
        , lineWidthEvaluator(lineWidth)
        , lineDashArrayEvaluator(lineDashArray)
        , lineBlurEvaluator(lineBlur)
        , lineCapEvaluator(lineCap)
        , lineJoinEvaluator(lineJoin)
        , lineOffsetEvaluator(lineOffset)
        , blendModeEvaluator(blendMode)
        , lineDottedEvaluator(lineDotted)
        , lineDottedSkewEvaluator(lineDottedSkew) {}

    LineVectorStyle(LineVectorStyle &style)
        : lineColorEvaluator(style.lineColorEvaluator)
        , lineOpacityEvaluator(style.lineOpacityEvaluator)
        , lineWidthEvaluator(style.lineWidthEvaluator)
        , lineDashArrayEvaluator(style.lineDashArrayEvaluator)
        , lineBlurEvaluator(style.lineBlurEvaluator)
        , lineCapEvaluator(style.lineCapEvaluator)
        , lineJoinEvaluator(style.lineJoinEvaluator)
        , lineOffsetEvaluator(style.lineOffsetEvaluator)
        , blendModeEvaluator(style.blendModeEvaluator)
        , lineDottedEvaluator(style.lineDottedEvaluator)
        , lineDottedSkewEvaluator(style.lineDottedSkewEvaluator) {}

    UsedKeysCollection getUsedKeys() const {
        UsedKeysCollection usedKeys;
        std::shared_ptr<Value> values[] = {
            lineColorEvaluator.getValue(), lineOpacityEvaluator.getValue(),   lineWidthEvaluator.getValue(),
            lineBlurEvaluator.getValue(),  lineDashArrayEvaluator.getValue(), lineCapEvaluator.getValue(), lineJoinEvaluator.getValue(),
            blendModeEvaluator.getValue(), lineDottedEvaluator.getValue(),    lineDottedSkewEvaluator.getValue()};

        for (auto const &value : values) {
            if (!value)
                continue;
            auto const setKeys = value->getUsedKeys();
            usedKeys.includeOther(setKeys);
        }

        return usedKeys;
    };

    BlendMode getBlendMode(const EvaluationContext &context) {
        static const BlendMode defaultValue = BlendMode::NORMAL;
        return blendModeEvaluator.getResult(context, defaultValue).value;
    }

    Color getLineColor(const EvaluationContext &context) {
        static const Color defaultValue = ColorUtil::c(0, 0, 0, 1.0);
        return lineColorEvaluator.getResult(context, defaultValue).value;
    }

    double getLineOpacity(const EvaluationContext &context) {
        static const double defaultValue = 1.0;
        return lineOpacityEvaluator.getResult(context, defaultValue).value;
    }

    double getLineBlur(const EvaluationContext &context) {
        static const double defaultValue = 0.0;
        double value = lineBlurEvaluator.getResult(context, defaultValue).value;
        return value * context.dpFactor;
    }

    double getLineWidth(const EvaluationContext &context) {
        static const double defaultValue = 1.0;
        double value = lineWidthEvaluator.getResult(context, defaultValue).value;
        return value * context.dpFactor;
    }

    std::vector<float> getLineDashArray(const EvaluationContext &context) {
        static const std::vector<float> defaultValue = {};
        return lineDashArrayEvaluator.getResult(context, defaultValue).value;
    }

    LineCapType getLineCap(const EvaluationContext &context) {
        static const LineCapType defaultValue = LineCapType::BUTT;
        return lineCapEvaluator.getResult(context, defaultValue).value;
    }

    LineJoinType getLineJoin(const EvaluationContext &context) {
        static const LineJoinType defaultValue = LineJoinType::MITER;
        return lineJoinEvaluator.getResult(context, defaultValue).value;
    }

    double getLineOffset(const EvaluationContext &context, double width) {
        static const double defaultValue = 0.0;
        double offset = lineOffsetEvaluator.getResult(context, defaultValue).value;
        return std::min(offset * context.dpFactor, width * 0.5);
    }

    bool getLineDotted(const EvaluationContext &context) {
        static const bool defaultValue = false;
        return lineDottedEvaluator.getResult(context, defaultValue).value;
    }

    double getLineDottedSkew(const EvaluationContext &context) {
        static const bool defaultValue = 1.0;
        return lineDottedSkewEvaluator.getResult(context, defaultValue).value;
    }

    bool isSimpleLine() {
        return lineBlurEvaluator.getValue() == nullptr && lineDashArrayEvaluator.getValue() == nullptr &&
               lineOffsetEvaluator.getValue() == nullptr && lineDottedEvaluator.getValue() == nullptr &&
               lineDottedSkewEvaluator.getValue() == nullptr;
    }

    FeatureValueEvaluator<Color> lineColorEvaluator;
    FeatureValueEvaluator<double> lineOpacityEvaluator;
    FeatureValueEvaluator<double> lineBlurEvaluator;
    FeatureValueEvaluator<double> lineWidthEvaluator;
    FeatureValueEvaluator<std::vector<float>> lineDashArrayEvaluator;
    FeatureValueEvaluator<LineCapType> lineCapEvaluator;
    FeatureValueEvaluator<LineJoinType> lineJoinEvaluator;
    FeatureValueEvaluator<double> lineOffsetEvaluator;
    FeatureValueEvaluator<BlendMode> blendModeEvaluator;
    FeatureValueEvaluator<bool> lineDottedEvaluator;
    FeatureValueEvaluator<double> lineDottedSkewEvaluator;
};

class LineVectorLayerDescription : public VectorLayerDescription {
  public:
    VectorLayerType getType() override { return VectorLayerType::line; };
    LineVectorStyle style;
    float selectionSizeFactor;

    LineVectorLayerDescription(std::string identifier, std::string source, std::string sourceId, int minZoom, int maxZoom,
                               int sourceMinZoom, int sourceMaxZoom, std::shared_ptr<Value> filter, LineVectorStyle style,
                               std::optional<int32_t> renderPassIndex, std::shared_ptr<Value> interactable, bool multiselect,
                               bool selfMasked, float selectionSizeFactor)
        : VectorLayerDescription(identifier, source, sourceId, minZoom, maxZoom, sourceMinZoom, sourceMaxZoom, filter,
                                 renderPassIndex, interactable, multiselect, selfMasked)
        , style(style)
        , selectionSizeFactor(selectionSizeFactor) {};

    std::unique_ptr<VectorLayerDescription> clone() override {
        return std::make_unique<LineVectorLayerDescription>(
            identifier, source, sourceLayer, minZoom, maxZoom, sourceMinZoom, sourceMaxZoom, filter ? filter->clone() : nullptr,
            style, renderPassIndex, interactable ? interactable->clone() : nullptr, multiselect, selfMasked, selectionSizeFactor);
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
