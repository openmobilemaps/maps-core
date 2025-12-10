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

const constant float BlendScale = 1000;
const constant float BlendOffset = 0.01;

template <typename T>
T bilerp(T c00, T c01, T c10, T c11, float2 uv) {
    T c0 = mix(c00, c01, T(uv[0]));
    T c1 = mix(c10, c11, T(uv[0]));
    return mix(c0, c1, T(uv[1]));
}

template <typename T>
T baryinterp(T c0, T c1, T c2, float3 bary) {
    return c0 * bary[0] + c1 * bary[1] + c2 * bary[2];
}

float4 transform(float2 coordinate, float4 origin) {
    float x = 1.0 * sin(coordinate.y) * cos(coordinate.x) - origin.x;
    float y = 1.0 * cos(coordinate.y) - origin.y;
    float z = -1.0 * sin(coordinate.y) * sin(coordinate.x) - origin.z;
    return float4(x, y, z, 0);
}

[[patch(quad, 4)]] vertex VertexOut
quadTessellationVertexShader(const patch_control_point<Vertex3DTextureTessellatedIn> controlPoints [[stage_in]],
                             const float2 positionInPatch [[position_in_patch]],
                             constant float4x4 &vpMatrix [[buffer(1)]],
                             constant float4x4 &mMatrix [[buffer(2)]],
                             constant float4 &originOffset [[buffer(3)]],
                             constant float4 &origin [[buffer(4)]],
                             constant bool &is3d [[buffer(5)]])
{
    Vertex3DTextureTessellatedIn vA = controlPoints[0];
    Vertex3DTextureTessellatedIn vB = controlPoints[1];
    Vertex3DTextureTessellatedIn vC = controlPoints[2];
    Vertex3DTextureTessellatedIn vD = controlPoints[3];
    
    float4 vertexPosition = bilerp(vA.position, vB.position, vC.position, vD.position, positionInPatch);
    float2 vertexFrameCoord = bilerp(vA.frameCoord, vB.frameCoord, vC.frameCoord, vD.frameCoord, positionInPatch);
    float2 vertexUV = bilerp(vA.uv, vB.uv, vC.uv, vD.uv, positionInPatch);
    
    float4 position = vertexPosition;
    if (is3d) {
        float4 bent = transform(vertexFrameCoord, origin) - originOffset;
        float blend = saturate(length(originOffset) * BlendScale - BlendOffset);
        position = mix(position, bent, blend);
    }
     
    VertexOut out {
        .position = vpMatrix * ((mMatrix * float4(position.xyz, 1)) + originOffset),
        .uv = vertexUV
    };
  
    return out;
}

[[patch(triangle, 3)]] vertex VertexOut
polygonTessellationVertexShader(const patch_control_point<Vertex3DTessellatedIn> controlPoints [[stage_in]],
                                const float3 positionInPatch [[position_in_patch]],
                                constant float4x4 &vpMatrix [[buffer(1)]],
                                constant float4x4 &mMatrix [[buffer(2)]],
                                constant float4 &originOffset [[buffer(3)]],
                                constant float4 &origin [[buffer(4)]],
                                constant bool &is3d [[buffer(5)]])
{
    Vertex3DTessellatedIn vA = controlPoints[0];
    Vertex3DTessellatedIn vB = controlPoints[1];
    Vertex3DTessellatedIn vC = controlPoints[2];
    
    float4 vertexPosition = baryinterp(vA.position, vB.position, vC.position, positionInPatch);
    float2 vertexFrameCoord = baryinterp(vA.frameCoord, vB.frameCoord, vC.frameCoord, positionInPatch);
    
    float4 position = vertexPosition;
    if (is3d) {
        float4 bent = transform(vertexFrameCoord, origin) - originOffset;
        float blend = saturate(length(originOffset) * BlendScale - BlendOffset);
        position = mix(position, bent, blend);
    }
    
    VertexOut out {
        .position = vpMatrix * (float4(position.xyz, 1) + originOffset),
    };

    return out;
}
