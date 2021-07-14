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

vertex LineVertexOut
lineVertexShader(const LineVertexIn vertexIn [[stage_in]],
                 constant float4x4 &mvpMatrix [[buffer(1)]],
                 constant float &width [[buffer(2)]])
{
    LineVertexOut out {
        .position = mvpMatrix * float4(vertexIn.position.xy /* + (vertexIn.n.xy * miter)*/, 0.0, 1.0),
        .uv = vertexIn.position.xy,
        .lineA= vertexIn.lineA,
        .lineB = vertexIn.lineB
    };

    return out;
}

fragment float4
lineFragmentShader(LineVertexOut in [[stage_in]],
                   constant float4 &color [[buffer(1)]],
                   constant float &radius [[buffer(2)]])
{



    float2 m = in.lineB - in.lineA;
    float t0 = dot(m, in.uv - in.lineA) / dot(m, m);
    float d;
    if (t0 <= 0) {
        d = length(in.uv - in.lineA) / radius;
    } else if (t0 > 0 && t0 < 1) {
        float2 intersectPt = in.lineA + t0 * m;
        d = length(in.uv - intersectPt) / radius;
    } else {
        d = length(in.uv - in.lineB) / radius;
    }

    if (d >= 1) {
        discard_fragment();
    }

    return color;
}
