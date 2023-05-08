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

struct TextInstanceStyle {
    float4 color;
    float4 haloColor;
};

struct TextInstancedVertexOut {
  float4 position [[ position ]];
  float2 uv;
  float4 texureCoordinates;
  uint16_t styleIndex;
};

vertex TextInstancedVertexOut
textInstancedVertexShader(const VertexIn vertexIn [[stage_in]],
                 constant float4x4 &mvpMatrix [[buffer(1)]],
                          constant float2 *positions [[buffer(2)]],
                          constant float2 *scales [[buffer(3)]],
                          constant float *rotations [[buffer(4)]],
                          constant float4 *texureCoordinates [[buffer(5)]],
                          constant uint16_t *styleIndices [[buffer(6)]],
                          ushort instanceId [[instance_id]])
{
    const float2 position = positions[instanceId];
    const float2 scale = scales[instanceId];
    const float rotation = rotations[instanceId];

    const float angle = rotation * M_PI_F / 180.0;

    const float4x4 model_matrix = float4x4(
                                              float4(cos(angle) * scale.x, -sin(angle) * scale.x, 0, 0),
                                              float4(sin(angle) * scale.y, cos(angle) * scale.y, 0, 0),
                                              float4(0, 0, 0, 0),
                                              float4(position.x, position.y, 0.0, 1)
                                              );


    const float4x4 matrix = mvpMatrix * model_matrix;

    TextInstancedVertexOut out {
      .position = matrix * float4(vertexIn.position.xy, 0.0, 1.0),
      .uv = vertexIn.uv,
      .texureCoordinates = texureCoordinates[instanceId],
      .styleIndex = styleIndices[instanceId]
    };

    return out;
}

fragment float4
textInstancedFragmentShader(TextInstancedVertexOut in [[stage_in]],
                       constant TextInstanceStyle *styles [[buffer(1)]],
                       texture2d<float> texture0 [[ texture(0)]],
                       sampler textureSampler [[sampler(0)]])
{
    const float2 uv = in.texureCoordinates.xy + in.texureCoordinates.zw * float2(in.uv.x, 1 - in.uv.y);

    const TextInstanceStyle style = styles[in.styleIndex];

    float4 dist = texture0.sample(textureSampler, uv);

    if (style.color.a == 0 || (style.haloColor.a == 0.0 && dist.x <= 0.5)) {
        discard_fragment();
    }

    float delta = 0.1;
    float alpha = smoothstep(0.5 - delta, 0.5 + delta, dist.x);

    float4 mixed = mix(style.haloColor, style.color, alpha);

    float a2 = smoothstep(0.40, 0.5, sqrt(dist.x));
    return float4(mixed.r * a2, mixed.g * a2, mixed.b * a2, a2);
}
