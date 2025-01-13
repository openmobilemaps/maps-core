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
        return blendModeEvaluator.getResult(context, defaultValue);
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
        return rasterOpacityEvaluator.getResult(context, defaultValue);
    }
    
    double getRasterBrightnessMin(const EvaluationContext &context) {
        double defaultValue = 0.0;
        return rasterBrightnessMinEvaluator.getResult(context, defaultValue);
    }
    
    double getRasterBrightnessMax(const EvaluationContext &context) {
        double defaultValue = 1.0;
        return rasterBrightnessMaxEvaluator.getResult(context, defaultValue);
    }
    
    double getRasterContrast(const EvaluationContext &context) {
        double defaultValue = 0.0;
        return rasterContrastEvaluator.getResult(context, defaultValue);
    }

    double getRasterSaturation(const EvaluationContext &context) {
        double defaultValue = 0.0;
        return rasterSaturationEvaluator.getResult(context, defaultValue);
    }

    double getRasterGamma(const EvaluationContext &context) {
        double defaultValue = 1.0;
        return rasterGammaEvaluator.getResult(context, defaultValue);
    }


    double getRasterBrightnessShift(const EvaluationContext &context) {
        double defaultValue = 0.0;
        return rasterBrightnessShiftEvaluator.getResult(context, defaultValue);
    }

    ValueEvaluator<double> rasterOpacityEvaluator;
    ValueEvaluator<double> rasterBrightnessMinEvaluator;
    ValueEvaluator<double> rasterBrightnessMaxEvaluator;
    ValueEvaluator<double> rasterContrastEvaluator;
    ValueEvaluator<double> rasterSaturationEvaluator;
    ValueEvaluator<double> rasterGammaEvaluator;
    ValueEvaluator<double> rasterBrightnessShiftEvaluator;
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
    std::optional<std::string> coordinateReferenceSystem;
    std::optional<std::vector<int>> levels;

    RasterVectorLayerDescription(std::string identifier,
                                 std::string source,
                                 int minZoom,
                                 int maxZoom,
                                 int sourceMinZoom,
                                 int sourceMaxZoom,
                                 std::string url,
                                 std::shared_ptr<Value> filter,
                                 RasterVectorStyle style,
                                 bool adaptScaleToScreen,
                                 int32_t numDrawPreviousLayers,
                                 bool maskTiles,
                                 double zoomLevelScaleFactor,
                                 std::optional<int32_t> renderPassIndex,
                                 std::shared_ptr<Value> interactable,
                                 bool underzoom,
                                 bool overzoom,
                                 std::optional<::RectCoord> bounds,
                                 std::optional<std::string> coordinateReferenceSystem,
                                 std::optional<std::vector<int>> levels) :
    VectorLayerDescription(identifier, source, "", minZoom, maxZoom, sourceMinZoom, sourceMaxZoom, filter, renderPassIndex, interactable, false, false),
    style(style), url(url), underzoom(underzoom), overzoom(overzoom), adaptScaleToScreen(adaptScaleToScreen), numDrawPreviousLayers(numDrawPreviousLayers),
    maskTiles(maskTiles), zoomLevelScaleFactor(zoomLevelScaleFactor), bounds(bounds), coordinateReferenceSystem(coordinateReferenceSystem), levels(levels) {};


    std::unique_ptr<VectorLayerDescription> clone() override {
        return std::make_unique<RasterVectorLayerDescription>(identifier,
                                            source,
                                            minZoom,
                                            maxZoom,
                                            sourceMinZoom,
                                            sourceMaxZoom,
                                            url,
                                            filter,
                                            style,
                                            adaptScaleToScreen,
                                            numDrawPreviousLayers,
                                            maskTiles,
                                            zoomLevelScaleFactor,
                                            renderPassIndex,
                                            interactable ? interactable->clone() : nullptr,
                                            underzoom,
                                            overzoom,
                                            bounds,
                                            coordinateReferenceSystem,
                                            levels);
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
