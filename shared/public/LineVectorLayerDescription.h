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
#include "LineCapType.h"

class LineVectorStyle {
public:
    LineVectorStyle(std::shared_ptr<Value> lineColor = nullptr,
                    std::shared_ptr<Value> lineOpacity = nullptr,
                    std::shared_ptr<Value> lineWidth = nullptr,
                    std::shared_ptr<Value> lineDashArray = nullptr,
                    std::shared_ptr<Value> lineBlur = nullptr,
                    std::shared_ptr<Value> lineCap = nullptr,
                    std::shared_ptr<Value> lineOffset = nullptr,
                    std::shared_ptr<Value> blendMode = nullptr,
                    std::shared_ptr<Value> lineDotted = nullptr,
                    std::shared_ptr<Value> lineDottedSkew = nullptr)
            : lineColor(lineColor),
              lineOpacity(lineOpacity),
              lineWidth(lineWidth),
              lineDashArray(lineDashArray),
              lineBlur(lineBlur),
              lineCap(lineCap),
              lineOffset(lineOffset),
              blendMode(blendMode),
              lineDotted(lineDotted),
              lineDottedSkew(lineDottedSkew) {}

    LineVectorStyle(LineVectorStyle &style)
            : lineColor(style.lineColor),
              lineOpacity(style.lineOpacity),
              lineWidth(style.lineWidth),
              lineDashArray(style.lineDashArray),
              lineBlur(style.lineBlur),
              lineCap(style.lineCap),
              lineOffset(style.lineOffset),
              blendMode(style.blendMode),
              lineDotted(style.lineDotted),
              lineDottedSkew(style.lineDottedSkew) {}

    UsedKeysCollection getUsedKeys() const {
        UsedKeysCollection usedKeys;
        std::shared_ptr<Value> values[] = { lineColor, lineOpacity, lineWidth, lineBlur, lineDashArray, lineCap, blendMode, lineDotted, lineDottedSkew };

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

    Color getLineColor(const EvaluationContext &context){
        static const Color defaultValue = ColorUtil::c(0, 0, 0, 1.0);
        return lineColorEvaluator.getResult(lineColor, context, defaultValue);
    }

    double getLineOpacity(const EvaluationContext &context){
        static const double defaultValue = 1.0;
        return lineOpacityEvaluator.getResult(lineOpacity, context, defaultValue);
    }

    double getLineBlur(const EvaluationContext &context){
        static const double defaultValue = 0.0;
        double value = lineBlurEvaluator.getResult(lineBlur, context, defaultValue);
        return value * context.dpFactor;
    }

    double getLineWidth(const EvaluationContext &context){
        static const double defaultValue = 1.0;
        double value = lineWidthEvaluator.getResult(lineWidth, context, defaultValue);
        return value * context.dpFactor;
    }

    std::vector<float> getLineDashArray(const EvaluationContext &context){
        static const std::vector<float> defaultValue = {};
        return lineDashArrayEvaluator.getResult(lineDashArray, context, defaultValue);
    }

    LineCapType getLineCap(const EvaluationContext &context){
        static const LineCapType defaultValue = LineCapType::BUTT;
        return lineCapEvaluator.getResult(lineCap, context, defaultValue);
    }

    double getLineOffset(const EvaluationContext &context) {
        static const double defaultValue = 0.0;
        double offset = lineOffsetEvaluator.getResult(lineOffset, context, defaultValue);
        return std::min(offset * context.dpFactor, getLineWidth(context) * 0.5);
    }
    
    bool getLineDotted(const EvaluationContext &context) {
        static const bool defaultValue = false;
        return lineDottedEvaluator.getResult(lineDotted, context, defaultValue);
    }
    
    double getLineDottedSkew(const EvaluationContext &context) {
        static const bool defaultValue = 1.0;
        return lineDottedSkewEvaluator.getResult(lineDottedSkew, context, defaultValue);
    }

    std::shared_ptr<Value> lineColor;
    std::shared_ptr<Value> lineOpacity;
    std::shared_ptr<Value> lineBlur;
    std::shared_ptr<Value> lineWidth;
    std::shared_ptr<Value> lineDashArray;
    std::shared_ptr<Value> lineCap;
    std::shared_ptr<Value> lineOffset;
    std::shared_ptr<Value> blendMode;
    std::shared_ptr<Value> lineDotted;
    std::shared_ptr<Value> lineDottedSkew;

private:
    ValueEvaluator<Color> lineColorEvaluator;
    ValueEvaluator<double> lineOpacityEvaluator;
    ValueEvaluator<double> lineBlurEvaluator;
    ValueEvaluator<double> lineWidthEvaluator;
    ValueEvaluator<std::vector<float>> lineDashArrayEvaluator;
    ValueEvaluator<LineCapType> lineCapEvaluator;
    ValueEvaluator<double> lineOffsetEvaluator;
    ValueEvaluator<BlendMode> blendModeEvaluator;
    ValueEvaluator<bool> lineDottedEvaluator;
    ValueEvaluator<double> lineDottedSkewEvaluator;
};

class LineVectorLayerDescription: public VectorLayerDescription {
public:
    VectorLayerType getType() override { return VectorLayerType::line; };
    LineVectorStyle style;

    LineVectorLayerDescription(std::string identifier,
                               std::string source,
                               std::string sourceId,
                               int minZoom,
                               int maxZoom,
                               std::shared_ptr<Value> filter,
                               LineVectorStyle style,
                               std::optional<int32_t> renderPassIndex,
                               std::shared_ptr<Value> interactable,
                               bool multiselect,
                               bool selfMasked):
    VectorLayerDescription(identifier, source, sourceId, minZoom, maxZoom, filter, renderPassIndex, interactable, multiselect, selfMasked),
    style(style) {};

    std::unique_ptr<VectorLayerDescription> clone() override {
        return std::make_unique<LineVectorLayerDescription>(identifier, source, sourceLayer, minZoom, maxZoom,
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

