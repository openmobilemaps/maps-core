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

float2 baryinterp(float2 a, float2 b, float2 c, float3 p) {
    return a * p.x + b * p.y + c * p.z;
}

[[patch(triangle, 3)]]
vertex VertexOut
baseVertexShader(const patch_control_point<VertexIn> patch [[stage_in]],
                 const float3 positionInPatch [[position_in_patch]],
                    constant float4x4 &mvpMatrix [[buffer(1)]],
                    constant float &time [[buffer(2)]])
{
    float2 p0 = patch[0].position;
    float2 p1 = patch[1].position;
    float2 p2 = patch[3].position;
    float2 pos = baryinterp(p0, p1, p2, positionInPatch);

    float2 uv0 = patch[0].uv;
    float2 uv1 = patch[1].uv;
    float2 uv2 = patch[3].uv;
    float2 uv = baryinterp(uv0, uv1, uv2, positionInPatch);

    float px = pos.x;
    float py = pos.y;

    float R = 6371000;
    float lambda = px / R;
    float phi = 2*atan(exp(py / R)) - 0 * 3.1415926 / 2;

    float radius = 1.0;
    float ratio = 2556.0/1179.0;

    VertexOut out {
        .position = float4(radius*sin(phi)*cos(lambda+time), radius*cos(phi) / ratio, radius*sin(phi)*sin(lambda+time), 1),
        .uv = uv
    };


    
    return out;
}

vertex VertexOut
flatBaseVertexShader(const VertexIn vertexIn [[stage_in]],
                 constant float4x4 &mvpMatrix [[buffer(1)]],
                 constant float &time [[buffer(2)]])
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

    if (a == 0) {
       discard_fragment();
    }

    return float4(color.r * a, color.g * a, color.b * a, a);
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

    if (a == 0) {
       discard_fragment();
    }

    return float4(color.r * a, color.g * a, color.b * a, a);
}


[[patch(triangle, 3)]]
vertex VertexOut
colorVertexShader(const patch_control_point<VertexIn> patch [[stage_in]],
                  const float3 positionInPatch [[position_in_patch]],
                    constant float4x4 &mvpMatrix [[buffer(1)]],
                  constant float &time [[buffer(2)]]
                  )
{

    float2 p0 = patch[0].position;
    float2 p1 = patch[1].position;
    float2 p2 = patch[3].position;
    float2 pos = baryinterp(p0, p1, p2, positionInPatch);

    float2 uv0 = patch[0].uv;
    float2 uv1 = patch[1].uv;
    float2 uv2 = patch[3].uv;
    float2 uv = baryinterp(uv0, uv1, uv2, positionInPatch);

//    float2 pos = patch.position;
//    float2 uv = patch.uv;

//    float2 pos = patch[0].position;
//    float2 uv = patch[0].uv;

    float px = pos.x;
    float py = pos.y;

    float R = 6371000;
    float lambda = px / R;
    float phi = 2*atan(exp(py / R)) - 0 * 3.1415926 / 2;

    float radius = 1.0;
    float ratio = 2556.0/1179.0;

     VertexOut out {
         .position = float4(radius*sin(phi)*cos(lambda+time), radius*cos(phi) / ratio, radius*sin(phi)*sin(lambda+time), 1),
         .uv = uv
     };



    /*
    VertexOut out {
        .position = mvpMatrix * float4(vertexIn.position.xy, 0.0, 1.0),
        .uv = vertexIn.uv
    };
     */


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


kernel void compute_tess_factors(
         device MTLTriangleTessellationFactorsHalf *factors [[buffer(0)]],
         uint pid [[thread_position_in_grid]]) {
     factors[pid].edgeTessellationFactor[0] = 5;
     factors[pid].edgeTessellationFactor[1] = 5;
     factors[pid].edgeTessellationFactor[2] = 5;
     factors[pid].insideTessellationFactor = 5;
 }
