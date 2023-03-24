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

    std::unordered_set<std::string> getUsedKeys() const {
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
    bool underzoom = true;
    bool overzoom = true;

    RasterVectorLayerDescription(std::string identifier,
                                 int minZoom,
                                 int maxZoom,
                                 std::string url,
                                 RasterVectorStyle style,
                                 std::optional<int32_t> renderPassIndex,
                                 std::shared_ptr<Value> interactable):
    VectorLayerDescription(identifier, identifier, "", minZoom, maxZoom, nullptr, renderPassIndex, interactable),
    style(style), url(url) {};

    RasterVectorLayerDescription(std::string identifier,
                                 int minZoom,
                                 int maxZoom,
                                 std::string url,
                                 bool adaptScaleToScreen = false,
                                 int32_t numDrawPreviousLayers = 2,
                                 bool maskTiles = false,
                                 double zoomLevelScaleFactor = 0.65,
                                 std::shared_ptr<Value> interactable = nullptr,
                                 std::optional<int32_t> renderPassIndex = std::nullopt,
                                 bool underzoom = true,
                                 bool overzoom = true) :
            VectorLayerDescription(identifier, identifier, "", minZoom, maxZoom, nullptr, renderPassIndex, interactable),
            style(), url(url), adaptScaleToScreen(adaptScaleToScreen), numDrawPreviousLayers(numDrawPreviousLayers),
            maskTiles(maskTiles), zoomLevelScaleFactor(zoomLevelScaleFactor), underzoom(underzoom), overzoom(overzoom) {};

    std::unique_ptr<VectorLayerDescription> clone() override {
        return std::make_unique<RasterVectorLayerDescription>(identifier, minZoom, maxZoom, url, adaptScaleToScreen,
                                                              numDrawPreviousLayers, maskTiles, zoomLevelScaleFactor,
                                                              interactable ? interactable->clone() : nullptr,
                                                              renderPassIndex, underzoom, overzoom);
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
