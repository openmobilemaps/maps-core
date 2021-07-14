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

struct LineVertexIn {
    float2 position [[attribute(0)]];
    float2 lineA [[attribute(1)]];
    float2 lineB [[attribute(2)]];
    float2 widthNormal [[attribute(3)]];
    float2 lenghtNormal [[attribute(4)]];
};

struct LineVertexOut {
    float4 position [[ position ]];
    float2 uv;
    float2 lineA;
    float2 lineB;
};


vertex LineVertexOut
lineVertexShader(const LineVertexIn vertexIn [[stage_in]],
                 constant float4x4 &mvpMatrix [[buffer(1)]],
                 constant float &width [[buffer(2)]])
{
    // extend position in width direction and in lenght direction by width / 2.0
    float4 extendedPosition = float4(vertexIn.position.xy + (vertexIn.widthNormal.xy * width / 2.0) + (vertexIn.lenghtNormal.xy * width / 2.0), 0.0, 1.0);
    
    LineVertexOut out {
        .position = mvpMatrix * extendedPosition,
        .uv = extendedPosition.xy,
        .lineA = vertexIn.lineA,
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
