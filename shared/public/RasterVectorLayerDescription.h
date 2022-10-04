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

class RasterVectorStyle {
public:

    RasterVectorStyle(std::shared_ptr<Value> rasterOpacity = nullptr):
    rasterOpacity(rasterOpacity) {}

    std::unordered_set<std::string> getUsedKeys() {
        std::unordered_set<std::string> usedKeys;
        std::vector<std::shared_ptr<Value>> values = { rasterOpacity };

        for (auto const &value: values) {
            if (!value) continue;
            auto const setKeys = value->getUsedKeys();
            usedKeys.insert(setKeys.begin(), setKeys.end());
        }

        return usedKeys;
    }

    double getRasterOpacity(const EvaluationContext &context){
        double defaultValue = 1.0;
        return rasterOpacity ? rasterOpacity->evaluateOr(context, defaultValue) : defaultValue;
    }
    
private:
    std::shared_ptr<Value> rasterOpacity;
};

class RasterVectorLayerDescription: public VectorLayerDescription  {
public:
    VectorLayerType getType() override { return VectorLayerType::raster; };

    std::string url;
    RasterVectorStyle style;
    bool adaptScaleToScreen = false;
    int32_t numDrawPreviousLayers = 2;
    bool maskTiles = false;
    double zoomLevelScaleFactor = 0.65;

    RasterVectorLayerDescription(std::string identifier,
                                 int minZoom,
                                 int maxZoom,
                                 std::string url,
                                 RasterVectorStyle style,
                                 std::optional<int32_t> renderPassIndex):
    VectorLayerDescription(identifier, "", "", minZoom, maxZoom, nullptr, renderPassIndex),
    style(style), url(url) {};

    RasterVectorLayerDescription(std::string identifier,
                                 int minZoom,
                                 int maxZoom,
                                 std::string url,
                                 bool adaptScaleToScreen = false,
                                 int32_t numDrawPreviousLayers = 2,
                                 bool maskTiles = false,
                                 double zoomLevelScaleFactor = 0.65,
                                 std::optional<int32_t> renderPassIndex = std::nullopt):
    VectorLayerDescription(identifier, "", "", minZoom, maxZoom, nullptr, renderPassIndex),
    style(), url(url), adaptScaleToScreen(adaptScaleToScreen), numDrawPreviousLayers(numDrawPreviousLayers), maskTiles(maskTiles), zoomLevelScaleFactor(zoomLevelScaleFactor)  {};

    virtual std::unordered_set<std::string> getUsedKeys() override {
        std::unordered_set<std::string> usedKeys;

        auto parentKeys = VectorLayerDescription::getUsedKeys();
        usedKeys.insert(parentKeys.begin(), parentKeys.end());

        auto styleKeys = style.getUsedKeys();
        usedKeys.insert(styleKeys.begin(), styleKeys.end());

        return usedKeys;
    };
};
