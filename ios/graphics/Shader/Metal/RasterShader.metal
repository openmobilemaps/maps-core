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
};

vertex VertexOut
unitSphereRasterVertexShader(const Vertex3DIn vertexIn [[stage_in]],
                 constant float4x4 &mvpMatrix [[buffer(1)]])
{

    float4 adjPos = float4(1.0 / length(vertexIn.position.xyz) * vertexIn.position.xyz, 1.0);

    VertexOut out {
        .position = mvpMatrix * adjPos,
        .uv = vertexIn.uv
    };

    return out;
}

vertex VertexOut
rasterVertexShader(const Vertex3DIn vertexIn [[stage_in]],
                 constant float4x4 &mvpMatrix [[buffer(1)]])
{
    VertexOut out {
        .position = mvpMatrix * float4(vertexIn.position.xyz, 1.0),
        .uv = vertexIn.uv
    };
    
    return out;
}

fragment float4
rasterFragmentShader(VertexOut in [[stage_in]],
                   constant RasterStyle *styling [[buffer(1)]],
                   texture2d<float> texture0 [[ texture(0)]],
                   sampler textureSampler [[sampler(0)]])
{
    float4 color = texture0.sample(textureSampler, in.uv);
    
    color.a *= styling[0].opacity;
    
    if (color.a == 0) {
        discard_fragment();
    }
    
    float3 rgb = color.rgb;
    
    float average = (color.r + color.g + color.b) / 3.0;
    
    rgb += (average - rgb) * styling[0].saturation;
    
    rgb = (rgb - 0.5) * styling[0].contrast + 0.5;
    
    
    float3 brightnessMin = float3(styling[0].brightnessMin, styling[0].brightnessMin, styling[0].brightnessMin);
    float3 brightnessMax = float3(styling[0].brightnessMax, styling[0].brightnessMax, styling[0].brightnessMax);

    float gamma = styling[0].gamma;
    rgb = pow(rgb, (1.0 / (gamma)));
    rgb = mix(brightnessMin, brightnessMax, min(rgb / color.a, float3(1.0)));
    return float4(rgb * color.a, color.a) * styling[0].opacity;
}
