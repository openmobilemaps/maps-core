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
#include "VectorMapSourceDescription.h"
#include "RasterShaderStyle.h"
#include "FeatureValueEvaluator.h"

class RasterVectorStyle {
public:
    RasterVectorStyle(std::shared_ptr<Value> rasterOpacity,
                      std::shared_ptr<Value> rasterBrightnessMin,
                      std::shared_ptr<Value> rasterBrightnessMax,
                      std::shared_ptr<Value> rasterContrast,
                      std::shared_ptr<Value> rasterSaturation,
                      std::shared_ptr<Value> rasterGamma,
                      std::shared_ptr<Value> rasterBrightnessShift,
                      std::shared_ptr<Value> blendMode) :
            rasterOpacityEvaluator(rasterOpacity),
            rasterBrightnessMinEvaluator(rasterBrightnessMin),
            rasterBrightnessMaxEvaluator(rasterBrightnessMax),
            rasterContrastEvaluator(rasterContrast), rasterSaturationEvaluator(rasterSaturation),
            rasterGammaEvaluator(rasterGamma),
            rasterBrightnessShiftEvaluator(rasterBrightnessShift), blendModeEvaluator(blendMode) {}

    RasterVectorStyle(RasterVectorStyle &style) :
            rasterOpacityEvaluator(style.rasterOpacityEvaluator),
            rasterBrightnessMinEvaluator(style.rasterBrightnessMinEvaluator),
            rasterBrightnessMaxEvaluator(style.rasterBrightnessMaxEvaluator),
            rasterContrastEvaluator(style.rasterContrastEvaluator),
            rasterSaturationEvaluator(style.rasterSaturationEvaluator),
            rasterGammaEvaluator(style.rasterGammaEvaluator),
            rasterBrightnessShiftEvaluator(style.rasterBrightnessShiftEvaluator),
            blendModeEvaluator(style.blendModeEvaluator) {}

    UsedKeysCollection getUsedKeys() const {
        UsedKeysCollection usedKeys;
        std::shared_ptr<Value> values[] = { 
            rasterOpacityEvaluator.getValue(),
            rasterBrightnessMinEvaluator.getValue(),
            rasterBrightnessMaxEvaluator.getValue(),
            rasterContrastEvaluator.getValue(),
            rasterSaturationEvaluator.getValue(),
            rasterGammaEvaluator.getValue(),
            rasterBrightnessShiftEvaluator.getValue(),
            blendModeEvaluator.getValue()
        };

        for (auto const &value: values) {
            if (!value) continue;
            auto const setKeys = value->getUsedKeys();
            usedKeys.includeOther(setKeys);
        }

        return usedKeys;
    }

    BlendMode getBlendMode(const EvaluationContext &context) {
        static const BlendMode defaultValue = BlendMode::NORMAL;
        return blendModeEvaluator.getResult(context, defaultValue).value;
    }
    
    RasterShaderStyle getRasterStyle(const EvaluationContext &context) {
        return {
            (float) getRasterOpacity(context),
            (float) getRasterBrightnessMin(context),
            (float) getRasterBrightnessMax(context),
            (float) getRasterContrast(context),
            (float) getRasterSaturation(context),
            (float) getRasterGamma(context),
            (float) getRasterBrightnessShift(context)
        };
    }

    double getRasterOpacity(const EvaluationContext &context) {
        double defaultValue = 1.0;
        return rasterOpacityEvaluator.getResult(context, defaultValue).value;
    }
    
    double getRasterBrightnessMin(const EvaluationContext &context) {
        double defaultValue = 0.0;
        return rasterBrightnessMinEvaluator.getResult(context, defaultValue).value;
    }
    
    double getRasterBrightnessMax(const EvaluationContext &context) {
        double defaultValue = 1.0;
        return rasterBrightnessMaxEvaluator.getResult(context, defaultValue).value;
    }
    
    double getRasterContrast(const EvaluationContext &context) {
        double defaultValue = 0.0;
        return rasterContrastEvaluator.getResult(context, defaultValue).value;
    }

    double getRasterSaturation(const EvaluationContext &context) {
        double defaultValue = 0.0;
        return rasterSaturationEvaluator.getResult(context, defaultValue).value;
    }

    double getRasterGamma(const EvaluationContext &context) {
        double defaultValue = 1.0;
        return rasterGammaEvaluator.getResult(context, defaultValue).value;
    }


    double getRasterBrightnessShift(const EvaluationContext &context) {
        double defaultValue = 0.0;
        return rasterBrightnessShiftEvaluator.getResult(context, defaultValue).value;
    }

    FeatureValueEvaluator<double> rasterOpacityEvaluator;
    FeatureValueEvaluator<double> rasterBrightnessMinEvaluator;
    FeatureValueEvaluator<double> rasterBrightnessMaxEvaluator;
    FeatureValueEvaluator<double> rasterContrastEvaluator;
    FeatureValueEvaluator<double> rasterSaturationEvaluator;
    FeatureValueEvaluator<double> rasterGammaEvaluator;
    FeatureValueEvaluator<double> rasterBrightnessShiftEvaluator;
    FeatureValueEvaluator<BlendMode> blendModeEvaluator;
};

struct RasterVectorMapSourceDescription : public VectorMapSourceDescription {
    bool maskTiles;
    
    RasterVectorMapSourceDescription(std::string identifier,
                               std::string url,
                               int minZoom,
                               int maxZoom,
                               std::optional<::RectCoord> bounds,
                               std::optional<float> zoomLevelScaleFactor,
                               std::optional<bool> adaptScaleToScreen,
                               std::optional<int> numDrawPreviousLayers,
                               std::optional<bool> underzoom,
                               std::optional<bool> overzoom,
                               std::optional<std::vector<int>> levels,
                               bool maskTiles) :
            VectorMapSourceDescription(identifier, url, minZoom, maxZoom, bounds, zoomLevelScaleFactor, adaptScaleToScreen, numDrawPreviousLayers, underzoom, overzoom, levels),
            maskTiles(maskTiles) {}
};

class RasterVectorLayerDescription: public VectorLayerDescription  {
public:
    VectorLayerType getType() override { return VectorLayerType::raster; };

    std::shared_ptr<RasterVectorMapSourceDescription> source;
    RasterVectorStyle style;

    RasterVectorLayerDescription(std::string identifier,
                                 std::shared_ptr<RasterVectorMapSourceDescription> source,
                                 int minZoom,
                                 int maxZoom,
                                 std::shared_ptr<Value> filter,
                                 std::optional<int32_t> renderPassIndex,
                                 std::shared_ptr<Value> interactable,
                                 RasterVectorStyle style)
    : VectorLayerDescription(identifier, source->identifier, "", minZoom, maxZoom, source->minZoom, source->maxZoom, filter, renderPassIndex, interactable, false, false)
    , source(source)
    , style(style) {}

    std::unique_ptr<VectorLayerDescription> clone() override {
        return std::make_unique<RasterVectorLayerDescription>(identifier,
                                            source,
                                            minZoom,
                                            maxZoom,
                                            filter,
                                            renderPassIndex,
                                            interactable ? interactable->clone() : nullptr,
                                            style);
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
