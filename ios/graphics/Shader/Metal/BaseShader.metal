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
baseVertexShader(const Vertex3DIn vertexIn [[stage_in]],
                constant float4x4 &mvpMatrix [[buffer(1)]],
                 constant float4x4 &mMatrix [[buffer(2)]])
{
    VertexOut out {
        .position = mvpMatrix * (mMatrix * float4(vertexIn.position.xy, 0.0, 1.0)),
        .uv = vertexIn.uv
    };
    
    return out;
}

vertex VertexOut
unitSphereBaseVertexShader(const Vertex3DIn vertexIn [[stage_in]],
                           constant float4x4 &viewMatrix [[buffer(1)]],
                           constant float4x4 &projectionMatrix [[buffer(2)]],
                           constant float4x4 &mMatrix [[buffer(3)]],
                           constant float4 &originOffset [[buffer(4)]]
                           )
{

    VertexOut out {
        .position = projectionMatrix * (viewMatrix * (mMatrix * (vertexIn.position + originOffset))),
        .uv = vertexIn.uv
    };

    return out;
}



fragment float4
baseFragmentShader(VertexOut in [[stage_in]],
                      constant float &alpha [[buffer(1)]],
                      texture2d<float> texture0 [[ texture(0)]],
                      sampler textureSampler [[sampler(0)]])
{
    float4 color = texture0.sample(textureSampler, in.uv);
    
    float a = color.a * alpha;

    if (a == 0) {
       discard_fragment();
    }

    return float4(color.r * a, color.g * a, color.b * a, a);
}


vertex VertexOut
colorVertexShader(const VertexIn vertexIn [[stage_in]],
                  constant float4x4 &mvpMatrix [[buffer(1)]],
                   constant float4x4 &mMatrix [[buffer(2)]])
{
    VertexOut out {
        .position = mvpMatrix * float4(vertexIn.position.xy, 0.0, 1.0),
        .uv = vertexIn.uv
    };

    return out;
}

fragment float4
colorFragmentShader(VertexOut in [[stage_in]],
                    constant float4 &color [[buffer(1)]])
{
    float a = color.a;
    return float4(color.r * a, color.g * a, color.b * a, a);
}

fragment float4
roundColorFragmentShader(VertexOut in [[stage_in]],
                    constant float4 &color [[buffer(1)]])
{
    if (length(in.uv - float2(0.5)) > 0.5)
    {
        discard_fragment();
    }

    float a = color.a;

    if (a == 0) {
       discard_fragment();
    }

    return float4(color.r * a, color.g * a, color.b * a, a);
}
