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


float4
color(float v [[stage_in]], texture2d<float> inputColorMap [[texture(0)]], sampler textureSampler [[sampler(0)]]) {
//    if (v == 0) {
//        return {1.0, 1.0, 1.0, 0.0};
//    }
    const float numberOfColors = 10.0;
    const float s = floor(255.0 / numberOfColors);
    const float t = floor(64.0 / numberOfColors);

    // this gives the color number
    int c = int(v * 255.0 / s + 0.5);

    v = (float(c) * t + 0.5 * t) / 64.0;

    return inputColorMap.sample(textureSampler, float2(0.5, v));
}

float4
gaussianBlur(texture2d<float> texture [[texture(0)]], float2 textureCoordinates [[stage_in]], sampler textureSampler [[sampler(0)]]) {

    float width = texture.get_width() * 0.5;
    float height = texture.get_height() * 0.5;

//    float kern[25] = {0.003765, 0.015019, 0.023792, 0.015019, 0.003765,
//                      0.015019, 0.059912, 0.094907, 0.059912, 0.015019,
//                      0.023792, 0.094907, 0.150342, 0.094907, 0.023792,
//                      0.015019, 0.059912, 0.094907, 0.059912, 0.015019,
//                      0.003765, 0.015019, 0.023792, 0.015019, 0.003765};
//
    // We take a blur kernel, but ignore some values
    float kern[25] = {
        0.00000000000, 0.00000000000, 0.02751200875, 0.00000000000, 0.00000000000,
        0.00000000000, 0.06927956743, 0.10974622623, 0.06927956743, 0.00000000000,
        0.02751200875, 0.10974622623, 0.17384879033, 0.10974622623, 0.02751200875,
        0.00000000000, 0.06927956743, 0.10974622623, 0.06927956743, 0.00000000000,
        0.00000000000, 0.00000000000, 0.02751200875, 0.00000000000, 0.00000000000
    };
    
    float4 sum = float4(0.0);
    for (int i = -2; i <= 2; i++) {
        for (int j = -2; j <= 2; j++) {
            float kernelValue = kern[(i + 2) * 5 + (j + 2)];
            if (kernelValue == 0) {
                continue;
            }
            float2 offset = float2(float(i) / width, float(j) / height);
            float4 sample = texture.sample(textureSampler, textureCoordinates + offset);
            sum += sample * kernelValue;
        }
    }

    return sum;
}


vertex VertexOut
textureInterpolationVertexShader(const VertexIn vertexIn [[stage_in]],
                 constant float4x4 &mvpMatrix [[buffer(1)]])
{
    VertexOut out {
        .position = float4((mvpMatrix * float4(vertexIn.position.xy, 0.0, 1.0)).xy, 0.0, 1.0),
        .uv = vertexIn.uv
    };

    return out;
}

fragment float4
textureInterpolationFragmentShader(VertexOut in [[stage_in]],
                       constant float &interpolation [[buffer(3)]],
                       texture2d<float> colorLegend [[ texture(2)]],
                       texture2d<float> texture1 [[ texture(1)]],
                       texture2d<float> texture0 [[ texture(0)]],
                       sampler textureSampler [[sampler(0)]])
{
    // 1. Spatial interpolation with Gaussian blur
    float4 color1 = gaussianBlur(texture0, in.uv, textureSampler);
    float4 color2 = gaussianBlur(texture1, in.uv, textureSampler);
    
    // 2. Temporal interpolation
    float4 interpolatedColor = mix(color1, color2, interpolation);
    
    // 3. Mapping the green-channel value to a color
    float4 mappedValue = color(interpolatedColor.g, colorLegend, textureSampler);
    
//    if (mappedValue.a == 0) {
//        discard_fragment();
//    }

    return mappedValue;
}
