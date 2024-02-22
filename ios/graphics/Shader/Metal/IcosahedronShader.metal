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
using namespace metal;


struct IcosahedronVertexIn {
    float2 position [[attribute(0)]];
    float value [[attribute(1)]];
};

struct IcosahedronVertexOut {
    float4 position [[ position ]];
    float value [[attribute(1)]];
};

vertex IcosahedronVertexOut
icosahedronVertexShader(const IcosahedronVertexIn vertexIn [[stage_in]],
                 constant float4x4 &mvpMatrix [[buffer(1)]])
{

    const float phi = (vertexIn.position.x - 180.0) * M_PI_F / 180.0;
    const float th = (vertexIn.position.y - 90.0) * M_PI_F / 180.0;

    float4 adjPos = float4(1.0 * sin(th) * cos(phi),
                       1.0 * cos(th),
                       -1.0 * sin(th) * sin(phi),
                       1.0);
    
    IcosahedronVertexOut out {
        .position = mvpMatrix * adjPos,
        .value = vertexIn.value
    };

    return out;
}

fragment float4
icosahedronFragmentShader(IcosahedronVertexOut in [[stage_in]],
                          constant float4 &color [[buffer(1)]])
{
    float a = color.a * in.value;
    return float4(color.r * a, color.g * a, color.b * a, a);
}
