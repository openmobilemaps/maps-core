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

vertex VertexOut
textVertexShader(const VertexIn vertexIn [[stage_in]],
                 constant float4x4 &mvpMatrix [[buffer(1)]],
                 constant float &scale [[buffer(2)]],
                 constant float2 &reference [[buffer(3)]])
{
    float2 pos = (mvpMatrix * float4(vertexIn.position.xy, 0.0, 1.0)).xy;
    float2 ref = (mvpMatrix * float4(reference.xy, 0.0, 1.0)).xy;
    pos = ref.xy + (pos.xy - ref.xy) * scale;

    VertexOut out {
        .position = float4(pos.xy, 0.0, 1.0),
        .uv = vertexIn.uv
    };

    return out;
}

fragment float4
textFragmentShader(VertexOut in [[stage_in]],
                   constant float4 &color [[buffer(1)]],
                   texture2d<float> texture0 [[ texture(0)]],
                   sampler textureSampler [[sampler(0)]])
{
    float delta = 0.1;
    float4 dist = texture0.sample(textureSampler, in.uv);
    float alpha = smoothstep(0.5 - delta, 0.5 + delta, dist.x) * color.a;
    return float4(color.r * alpha, color.g * alpha, color.b * alpha, alpha);
}

fragment float4
textFragmentShaderHalo(VertexOut in [[stage_in]],
                   constant float4 &color [[buffer(1)]],
                   texture2d<float> texture0 [[ texture(0)]],
                   sampler textureSampler [[sampler(0)]])
{
    float delta = 0.1;
    float4 dist = texture0.sample(textureSampler, in.uv);
    float alpha = smoothstep(0.5 - delta, 0.5 + delta, dist.x);

    float4 glowColor = float4(1.0, 1.0, 1.0, 1.0);
    float4 glyphColor = float4(color.r, color.g, color.b, color.a * alpha);

    float4 mixed = mix(glowColor, glyphColor, alpha);

    float a2 = smoothstep(0.0, 0.5, sqrt(dist.x));
    return float4(mixed.r * a2, mixed.g * a2, mixed.b * a2, a2);
}
