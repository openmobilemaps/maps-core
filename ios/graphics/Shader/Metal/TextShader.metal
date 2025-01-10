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
                 ushort amp_id [[amplification_id]],
                 constant float4x4x2 &mvpMatrix [[buffer(1)]])
{
    VertexOut out {
        .position = float4((mvpMatrix.matrices[amp_id] * float4(vertexIn.position.xy, 0.0, 1.0)).xy, 0.0, 1.0),
        .uv = vertexIn.uv
    };

    return out;
}

fragment half4
textFragmentShader(VertexOut in [[stage_in]],
                       constant half4 &color [[buffer(1)]],
                       constant half4 &haloColor [[buffer(2)]],
                       constant float &opacity [[buffer(3)]],
                       constant float &haloWidth [[buffer(4)]],
                       texture2d<half> texture0 [[ texture(0)]],
                       sampler textureSampler [[sampler(0)]])
{
    half4 dist = texture0.sample(textureSampler, in.uv);

    if (opacity == 0) {
      discard_fragment();
    }

    float median = max(min(dist.r, dist.g), min(max(dist.r, dist.g), dist.b)) / dist.a;
    float w = fwidth(median);
    float alpha = smoothstep(0.5 - w, 0.5 + w, median);

    half4 mixed = mix(haloColor, color, alpha);

    if(haloWidth > 0) {
      float start = (0.0 + 0.5 * (1.0 - haloWidth)) - w;
      float end = start + w;
      float a2 = smoothstep(start, end, median) * opacity;
      return half4(mixed.r * a2, mixed.g * a2, mixed.b * a2, a2);
    } else {
      return mixed;
    }

    return mixed;
}
