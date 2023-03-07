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
                    double dpFactor = 1.0):
    lineColor(lineColor),
    lineOpacity(lineOpacity),
    lineWidth(lineWidth),
    lineDashArray(lineDashArray),
    lineBlur(lineBlur),
    lineCap(lineCap),
    dpFactor(dpFactor) {}

    std::unordered_set<std::string> getUsedKeys() const {
        std::unordered_set<std::string> usedKeys;
        std::vector<std::shared_ptr<Value>> values = { lineColor, lineOpacity, lineWidth, lineBlur, lineDashArray, lineCap };

        for (auto const &value: values) {
            if (!value) continue;
            auto const setKeys = value->getUsedKeys();
            usedKeys.insert(setKeys.begin(), setKeys.end());
        }

        return usedKeys;
    };

    Color getLineColor(const EvaluationContext &context){
        static const Color defaultValue = ColorUtil::c(0, 0, 0, 1.0);
        return lineColor ? lineColor->evaluateOr(context, defaultValue) : defaultValue;
    }

    double getLineOpacity(const EvaluationContext &context){
        static const double defaultValue = 1.0;
        return lineOpacity ? lineOpacity->evaluateOr(context, defaultValue) : defaultValue;
    }

    double getLineBlur(const EvaluationContext &context){
        static const double defaultValue = 0.0;
        return lineBlur ? lineBlur->evaluateOr(context, defaultValue) : defaultValue;
    }

    double getLineWidth(const EvaluationContext &context){
        static const double defaultValue = 1.0;
        double value = lineWidth ? lineWidth->evaluateOr(context, defaultValue) : defaultValue;
        return value * dpFactor;
    }

    std::vector<float> getLineDashArray(const EvaluationContext &context){
        static const std::vector<float> defaultValue = {};
        return lineDashArray ? lineDashArray->evaluateOr(context, defaultValue) : defaultValue;
    }

    LineCapType getLineCap(const EvaluationContext &context){
        static const LineCapType defaultValue = LineCapType::BUTT;
        return lineCap ? lineCap->evaluateOr(context, defaultValue) : defaultValue;
    }

private:
    std::shared_ptr<Value> lineColor;
    std::shared_ptr<Value> lineOpacity;
    std::shared_ptr<Value> lineBlur;
    std::shared_ptr<Value> lineWidth;
    std::shared_ptr<Value> lineDashArray;
    std::shared_ptr<Value> lineCap;
    double dpFactor;
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
                               std::optional<int32_t> renderPassIndex):
    VectorLayerDescription(identifier, source, sourceId, minZoom, maxZoom, filter, renderPassIndex),
    style(style) {};

    std::unique_ptr<VectorLayerDescription> clone() override {
        return std::make_unique<LineVectorLayerDescription>(identifier, source, sourceId, minZoom, maxZoom,
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

