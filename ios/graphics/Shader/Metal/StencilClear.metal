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
stencilClearVertexShader(const Vertex3DTextureIn vertexIn [[stage_in]],
                    constant float4x4 &mvpMatrix [[buffer(1)]])
{
    VertexOut out {
        .position = float4(vertexIn.position.xy, 0.0, 1.0),
    };

    return out;
}

fragment half4
stencilClearFragmentShader(VertexOut in [[stage_in]],
                      constant float &alpha [[buffer(1)]],
                      texture2d<float> texture0 [[ texture(0)]],
                      sampler textureSampler [[sampler(0)]])
{
    return half4(0,0,0,0);
}
