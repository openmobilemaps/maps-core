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
                          uint instanceId [[instance_id]])
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

//struct TextInstanceStyle {
//    float4 color;
//    float4 haloColor;
//    float haloWidth;
//};

fragment float4
textInstancedFragmentShader(TextInstancedVertexOut in [[stage_in]],
                       constant float *styles [[buffer(1)]],
                       texture2d<float> texture0 [[ texture(0)]],
                       sampler textureSampler [[sampler(0)]])
{
    const float2 uv = in.texureCoordinates.xy + in.texureCoordinates.zw * float2(in.uv.x, 1 - in.uv.y);
    const int styleOffset = in.styleIndex * 9;
    const float4 color = float4(styles[styleOffset + 0], styles[styleOffset + 1], styles[styleOffset + 2], styles[styleOffset + 3]);
    const float4 haloColor = float4(styles[styleOffset + 4], styles[styleOffset + 5], styles[styleOffset + 6], styles[styleOffset + 7]);
    const float haloWidth = styles[styleOffset + 8];

    if (color.a == 0 && haloColor.a == 0.0) {
        discard_fragment();
    }

    float4 dist = texture0.sample(textureSampler, uv);

    float median = max(min(dist.r, dist.g), min(max(dist.r, dist.g), dist.b)) / dist.a;
    float w = fwidth(median);
    float alpha = smoothstep(0.5 - w, 0.5 + w, median);

    float4 mixed = mix(haloColor, color, alpha);

    if(haloWidth > 0) {
      float start = (0.0 + 0.5 * (1.0 - haloWidth)) - w;
      float end = start + w;
      float a2 = smoothstep(start, end, median) * color.a;
      return float4(mixed.r * a2, mixed.g * a2, mixed.b * a2, a2);
    } else {
      float a2 = alpha * color.a;
      return float4(mixed.r * a2, mixed.g * a2, mixed.b * a2, a2);
    }

    return mixed;
}
