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
    std::shared_ptr<Value<float>> opacity;
    std::shared_ptr<Value<float>> brightnessMin;
    std::shared_ptr<Value<float>> saturation;
    std::shared_ptr<Value<float>> contrast;

    RasterVectorStyle(std::shared_ptr<Value<float>> opacity = nullptr,
                      std::shared_ptr<Value<float>> brightnessMin = nullptr,
                      std::shared_ptr<Value<float>> saturation = nullptr,
                      std::shared_ptr<Value<float>> contrast = nullptr):
    opacity(opacity),
    brightnessMin(brightnessMin),
    saturation(saturation),
    contrast(contrast){}
};

class RasterVectorLayerDescription: public VectorLayerDescription  {
public:
    VectorLayerType getType() override { return VectorLayerType::raster; };

    std::string url;
    RasterVectorStyle style;
    bool adaptScaleToScreen = false;

    RasterVectorLayerDescription(std::string identifier,
                            int minZoom,
                            int maxZoom,
                            std::string url,
                           RasterVectorStyle style):
    VectorLayerDescription(identifier, "", minZoom, maxZoom, nullptr),
    style(style), url(url) {};

    RasterVectorLayerDescription(std::string identifier,
                            int minZoom,
                            int maxZoom,
                            std::string url,
                            bool adaptScaleToScreen = false):
    VectorLayerDescription(identifier, "", minZoom, maxZoom, nullptr),
    style(), url(url), adaptScaleToScreen(adaptScaleToScreen) {};
};
