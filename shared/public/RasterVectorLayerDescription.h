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
#include "RasterShaderStyle.h"


class RasterVectorStyle {
public:
    RasterVectorStyle(std::shared_ptr<Value> rasterOpacity,
                      std::shared_ptr<Value> rasterBrightnessMin,
                      std::shared_ptr<Value> rasterBrightnessMax,
                      std::shared_ptr<Value> rasterContrast,
                      std::shared_ptr<Value> rasterSaturation,
                      std::shared_ptr<Value> rasterGamma,
                      std::shared_ptr<Value> blendMode) :
            rasterOpacity(rasterOpacity), rasterBrightnessMin(rasterBrightnessMin), rasterBrightnessMax(rasterBrightnessMax),
            rasterContrast(rasterContrast), rasterSaturation(rasterSaturation), rasterGamma(rasterGamma), blendMode(blendMode) {}

    RasterVectorStyle(RasterVectorStyle &style) :
            rasterOpacity(style.rasterOpacity), rasterBrightnessMin(style.rasterBrightnessMin), rasterBrightnessMax(style.rasterBrightnessMax),
            rasterContrast(style.rasterContrast), rasterSaturation(style.rasterSaturation), rasterGamma(style.rasterGamma), blendMode(style.blendMode) {}

    UsedKeysCollection getUsedKeys() const {
        UsedKeysCollection usedKeys;
        std::shared_ptr<Value> values[] = { 
            rasterOpacity, 
            rasterBrightnessMin,
            rasterBrightnessMax,
            rasterContrast,
            rasterSaturation,
            rasterGamma,
            blendMode
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
        return blendMode ? blendMode->evaluateOr(context, defaultValue) : defaultValue;
    }
    
    RasterShaderStyle getRasterStyle(const EvaluationContext &context) {
        return {
            (float) getRasterOpacity(context),
            (float) getRasterBrightnessMin(context),
            (float) getRasterBrightnessMax(context),
            (float) getRasterContrast(context),
            (float) getRasterSaturation(context),
            (float) getRasterGamma(context)
        };
    }

    double getRasterOpacity(const EvaluationContext &context) {
        double defaultValue = 1.0;
        return rasterOpacityEvaluator.getResult(rasterOpacity, context, defaultValue);
    }
    
    double getRasterBrightnessMin(const EvaluationContext &context) {
        double defaultValue = 0.0;
        return rasterBrightnessMinEvaluator.getResult(rasterBrightnessMin, context, defaultValue);
    }
    
    double getRasterBrightnessMax(const EvaluationContext &context) {
        double defaultValue = 1.0;
        return rasterBrightnessMaxEvaluator.getResult(rasterBrightnessMax, context, defaultValue);
    }
    
    double getRasterContrast(const EvaluationContext &context) {
        double defaultValue = 0.0;
        return rasterContrastEvaluator.getResult(rasterContrast, context, defaultValue);
    }

    double getRasterSaturation(const EvaluationContext &context) {
        double defaultValue = 0.0;
        return rasterSaturationEvaluator.getResult(rasterSaturation, context, defaultValue);
    }

    double getRasterGamma(const EvaluationContext &context) {
        double defaultValue = 1.0;
        return rasterGammaEvaluator.getResult(rasterGamma, context, defaultValue);
    }

    std::shared_ptr<Value> rasterOpacity;
    std::shared_ptr<Value> rasterBrightnessMin;
    std::shared_ptr<Value> rasterBrightnessMax;
    std::shared_ptr<Value> rasterContrast;
    std::shared_ptr<Value> rasterSaturation;
    std::shared_ptr<Value> rasterGamma;
    std::shared_ptr<Value> blendMode;
private:
    ValueEvaluator<double> rasterOpacityEvaluator;
    ValueEvaluator<double> rasterBrightnessMinEvaluator;
    ValueEvaluator<double> rasterBrightnessMaxEvaluator;
    ValueEvaluator<double> rasterContrastEvaluator;
    ValueEvaluator<double> rasterSaturationEvaluator;
    ValueEvaluator<double> rasterGammaEvaluator;
    ValueEvaluator<BlendMode> blendModeEvaluator;
};

class RasterVectorLayerDescription: public VectorLayerDescription  {
public:
    VectorLayerType getType() override { return VectorLayerType::raster; };

    std::string url;
    RasterVectorStyle style;
    bool adaptScaleToScreen;
    int32_t numDrawPreviousLayers;
    bool maskTiles;
    double zoomLevelScaleFactor;
    bool overzoom;
    bool underzoom;
    std::optional<::RectCoord> bounds;

    RasterVectorLayerDescription(std::string identifier,
                                 std::string source,
                                 int minZoom,
                                 int maxZoom,
                                 std::string url,
                                 RasterVectorStyle style,
                                 bool adaptScaleToScreen,
                                 int32_t numDrawPreviousLayers,
                                 bool maskTiles,
                                 double zoomLevelScaleFactor,
                                 std::optional<int32_t> renderPassIndex,
                                 std::shared_ptr<Value> interactable,
                                 bool underzoom,
                                 bool overzoom,
                                 std::optional<::RectCoord> bounds):
    VectorLayerDescription(identifier, source, "", minZoom, maxZoom, nullptr, renderPassIndex, interactable, false),
    style(style), url(url), underzoom(underzoom), overzoom(overzoom), adaptScaleToScreen(adaptScaleToScreen), numDrawPreviousLayers(numDrawPreviousLayers),
    maskTiles(maskTiles), zoomLevelScaleFactor(zoomLevelScaleFactor), bounds(bounds) {};


    std::unique_ptr<VectorLayerDescription> clone() override {
        return std::make_unique<RasterVectorLayerDescription>(identifier,
                                            source,
                                            minZoom,
                                            maxZoom,
                                            url,
                                            style,
                                            adaptScaleToScreen,
                                            numDrawPreviousLayers,
                                            maskTiles,
                                            zoomLevelScaleFactor,
                                            renderPassIndex,
                                            interactable ? interactable->clone() : nullptr,
                                            underzoom,
                                            overzoom,
                                            bounds);
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
