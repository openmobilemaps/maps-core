/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include <metal_stdlib>
#include "DataStructures.metal"
using namespace metal;

struct RasterStyle {
    float opacity;
    float brightnessMin;
    float brightnessMax;
    float contrast;
    float saturation;
    float gamma;
    float brightnessShift;
};

fragment half4
rasterFragmentShader(VertexOut in [[stage_in]],
                   constant RasterStyle *styling [[buffer(1)]],
                   texture2d<half> texture0 [[ texture(0)]],
                   sampler textureSampler [[sampler(0)]])
{
    half4 color = texture0.sample(textureSampler, in.uv);

    if (color.a == 0.0 || styling[0].opacity == 0.0) {
        discard_fragment();
    }
    
    half3 rgb = color.rgb;

    rgb = clamp(rgb + styling[0].brightnessShift, half3(0.0), half3(1.0));

    float average = (color.r + color.g + color.b) / 3.0;
    
    rgb += (average - rgb) * styling[0].saturation;
    
    rgb = clamp((rgb - 0.5) * styling[0].contrast + 0.5, half3(0.0), half3(1.0));

    half3 brightnessMin = half3(styling[0].brightnessMin, styling[0].brightnessMin, styling[0].brightnessMin);
    half3 brightnessMax = half3(styling[0].brightnessMax, styling[0].brightnessMax, styling[0].brightnessMax);

    float gamma = styling[0].gamma;
    rgb = pow(rgb, (1.0 / (gamma)));
    rgb = mix(brightnessMin, brightnessMax, min(rgb / color.a, half3(1.0)));
    return half4(rgb * color.a, color.a) * styling[0].opacity;
}
