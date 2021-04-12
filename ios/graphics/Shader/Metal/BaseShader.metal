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
baseVertexShader(const VertexIn vertexIn [[stage_in]],
                    constant float4x4 &mvpMatrix [[buffer(1)]])
{
    VertexOut out {
        .position = mvpMatrix * float4(vertexIn.position.xy, 0.0, 1.0),
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
    return float4(color.r * a, color.g * a, color.b * a, a);
}

vertex VertexOut
lineVertexShader(const VertexIn vertexIn [[stage_in]],
                 constant float4x4 &mvpMatrix [[buffer(1)]],
                 constant float &miter [[buffer(2)]])
{
    VertexOut out {
        .position = mvpMatrix * float4(vertexIn.position.xy, 0.0, 1.0),
        .uv = (mvpMatrix * float4(vertexIn.position.xy, 0.0, 1.0)).xy,
        .lineStart = mvpMatrix * float4(vertexIn.lineStart.xy, 0.0, 1.0),
        .lineEnd = mvpMatrix * float4(vertexIn.lineEnd.xy, 0.0, 1.0),
    };

    return out;
}

fragment float4
lineFragmentShader(VertexOut in [[stage_in]],
                   constant float4 &color [[buffer(1)]],
                   float radius)
{
    float x0 = in.uv.x;
    float y0 = in.uv.y;

    float x1 = in.lineStart.x;
    float y1 = in.lineStart.y;

    float x2 = in.lineEnd.x;
    float y2 = in.lineEnd.y;

    float dis = abs((x2 - x1) * (y1 - y0) - (x1 - x0) * (y2 - y1))  / sqrt(pow(x2 - x1,2) + pow(y2 - y1,2));

    if (dis > radius) {
        discard_fragment();
    }

    return float4(dis, 0, 0, 1);


    /*float2 pa = in.uv - in.lineStart.xy;
    float2 ba = in.lineEnd.xy - in.lineStart.xy;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    float lenght = length( pa - ba*h ) - radius;
    if (lenght <= -10) {
        discard_fragment();
    }
    return color;*/
}

vertex VertexOut
pointVertexShader(const VertexIn vertexIn [[stage_in]],
                  constant float4x4 &mvpMatrix [[buffer(1)]],
                  constant float &pointSize [[buffer(2)]])
{
    VertexOut out {
        .position = mvpMatrix * float4(vertexIn.position.xy, 0.0, 1.0),
        .pointsize = pointSize
    };

    return out;
}

fragment float4
pointFragmentShader(VertexOut in [[stage_in]],
                    float2 pointCoord  [[point_coord]],
                    constant float4 &color [[buffer(1)]])
{
    // draw only where the point coord is in the radius
    // s.t. the point is round
    if (length(pointCoord - float2(0.5)) > 0.5)
    {
        discard_fragment();
    }

    float a = color.a;
    return float4(color.r * a, color.g * a, color.b * a, a);
}

vertex VertexOut
colorVertexShader(const VertexIn vertexIn [[stage_in]],
                    constant float4x4 &mvpMatrix [[buffer(1)]])
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
    return float4(color.r * a, color.g * a, color.b * a, a);
}
