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
                      std::shared_ptr<Value> rasterSaturation):
    rasterOpacity(rasterOpacity), rasterBrightnessMin(rasterBrightnessMin), rasterBrightnessMax(rasterBrightnessMax), rasterContrast(rasterContrast), rasterSaturation(rasterSaturation) {}

    std::unordered_set<std::string> getUsedKeys() {
        std::unordered_set<std::string> usedKeys;
        std::vector<std::shared_ptr<Value>> values = { 
            rasterOpacity, 
            rasterBrightnessMin,
            rasterBrightnessMax,
            rasterContrast,
            rasterSaturation 
        };

        for (auto const &value: values) {
            if (!value) continue;
            auto const setKeys = value->getUsedKeys();
            usedKeys.insert(setKeys.begin(), setKeys.end());
        }

        return usedKeys;
    }
    
    RasterShaderStyle getRasterStyle(const EvaluationContext &context) {
        return {
            (float) getRasterOpacity(context),
            (float) getRasterBrightnessMin(context),
            (float) getRasterBrightnessMax(context),
            (float) getRasterContrast(context),
            (float) getRasterSaturation(context)
        };
    }

    double getRasterOpacity(const EvaluationContext &context) {
        double defaultValue = 1.0;
        return rasterOpacity ? rasterOpacity->evaluateOr(context, defaultValue) : defaultValue;
    }
    
    double getRasterBrightnessMin(const EvaluationContext &context) {
        double defaultValue = 0.0;
        return rasterBrightnessMin ? rasterBrightnessMin->evaluateOr(context, defaultValue) : defaultValue;
    }
    
    double getRasterBrightnessMax(const EvaluationContext &context) {
        double defaultValue = 1.0;
        return rasterBrightnessMax ? rasterBrightnessMax->evaluateOr(context, defaultValue) : defaultValue;
    }
    
    double getRasterContrast(const EvaluationContext &context) {
        double defaultValue = 0.0;
        return rasterContrast ? rasterContrast->evaluateOr(context, defaultValue) : defaultValue;
    }
    
    double getRasterSaturation(const EvaluationContext &context) {
        double defaultValue = 0.0;
        return rasterSaturation ? rasterSaturation->evaluateOr(context, defaultValue) : defaultValue;
    }
    
private:
    std::shared_ptr<Value> rasterOpacity;
    std::shared_ptr<Value> rasterBrightnessMin;
    std::shared_ptr<Value> rasterBrightnessMax;
    std::shared_ptr<Value> rasterContrast;
    std::shared_ptr<Value> rasterSaturation;
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

    RasterVectorLayerDescription(std::string identifier,
                                 int minZoom,
                                 int maxZoom,
                                 std::string url,
                                 RasterVectorStyle style,
                                 bool adaptScaleToScreen,
                                 int32_t numDrawPreviousLayers,
                                 bool maskTiles,
                                 double zoomLevelScaleFactor,
                                 std::optional<int32_t> renderPassIndex,
                                 bool overzoom,
                                 bool underzoom):
    VectorLayerDescription(identifier, "", "", minZoom, maxZoom, nullptr, renderPassIndex),
    style(style), url(url), adaptScaleToScreen(adaptScaleToScreen), numDrawPreviousLayers(numDrawPreviousLayers), maskTiles(maskTiles), zoomLevelScaleFactor(zoomLevelScaleFactor), overzoom(overzoom), underzoom(underzoom)  {};

    virtual std::unordered_set<std::string> getUsedKeys() override {
        std::unordered_set<std::string> usedKeys;

        auto parentKeys = VectorLayerDescription::getUsedKeys();
        usedKeys.insert(parentKeys.begin(), parentKeys.end());

        auto styleKeys = style.getUsedKeys();
        usedKeys.insert(styleKeys.begin(), styleKeys.end());

        return usedKeys;
    };
};
