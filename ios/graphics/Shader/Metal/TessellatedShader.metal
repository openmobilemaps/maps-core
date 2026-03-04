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
inline T bilerp(T c00, T c01, T c10, T c11, float2 uv) {
    T c0 = mix(c00, c01, T(uv[0]));
    T c1 = mix(c10, c11, T(uv[0]));
    return mix(c0, c1, T(uv[1]));
}

template<typename T>
inline T bilerp_fast(T c00, T c01, T c10, T c11, half2 uv)
{
    half u = uv.x;
    half v = uv.y;
    half w00 = (half)1 - u;
    half w01 = u;
    half w10 = (half)1 - u;
    half w11 = u;
    half oneMinusV = (half)1 - v;
    w00 *= oneMinusV;
    w01 *= oneMinusV;
    w10 *= v;
    w11 *= v;
    return c00 * float(w00) + c01 * float(w01) + c10 * float(w10) + c11 * float(w11);
}

template <typename T>
inline T baryinterp(T c0, T c1, T c2, float3 bary) {
    return c0 * bary[0] + c1 * bary[1] + c2 * bary[2];
}

inline float4 transform(float2 coordinate, float4 origin) {
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
    half2 p = half2(positionInPatch);
    
    float4 position = bilerp_fast(vA.position, vB.position, vC.position, vD.position, p);
    if (is3d) {
        float2 frameCoord = bilerp_fast(vA.frameCoord, vB.frameCoord, vC.frameCoord, vD.frameCoord, p);
        float4 bent = transform(frameCoord, origin) - originOffset;
        float blend = saturate(length(originOffset) * BlendScale - BlendOffset);
        position = mix(position, bent, blend);
    }
    float2 uv = bilerp_fast(vA.uv, vB.uv, vC.uv, vD.uv, p);
    
    VertexOut out {
        .position = vpMatrix * ((mMatrix * float4(position.xyz, 1)) + originOffset),
        .uv = uv
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
    
    float4 position = baryinterp(vA.position, vB.position, vC.position, positionInPatch);
    if (is3d) {
        float2 frameCoord = baryinterp(vA.frameCoord, vB.frameCoord, vC.frameCoord, positionInPatch);
        float4 bent = transform(frameCoord, origin) - originOffset;
        float blend = saturate(length(originOffset) * BlendScale - BlendOffset);
        position = mix(position, bent, blend);
    }
    
    VertexOut out {
        .position = vpMatrix * (float4(position.xyz, 1) + originOffset),
    };

    return out;
}

float decodeElevation(float3 rgbaAltitude) {
    return (rgbaAltitude.r * 256.0 * 256.0 * 255.0 +
            rgbaAltitude.g * 256.0 * 255.0 +
            rgbaAltitude.b * 255.0) / 10.0 - 10000.0;
}

inline float3 displacedLocalPosition(const Vertex3DTextureTessellatedIn vA,
                                     const Vertex3DTextureTessellatedIn vB,
                                     const Vertex3DTextureTessellatedIn vC,
                                     const Vertex3DTextureTessellatedIn vD,
                                     half2 p,
                                     float3 origin,
                                     float3 originOffset,
                                     bool is3d,
                                     bool hasElevationTexture,
                                     texture2d<float> elevationTexture0,
                                     sampler sampler0)
{
    float4 position = bilerp_fast(vA.position, vB.position, vC.position, vD.position, p);

    if (is3d) {
        float2 frameCoord = bilerp_fast(vA.frameCoord, vB.frameCoord, vC.frameCoord, vD.frameCoord, p);
        float4 bent = transform(frameCoord, float4(origin, 0.0)) - float4(originOffset, 0.0);
        float blend = saturate(length(originOffset) * BlendScale - BlendOffset);
        position = mix(position, bent, blend);

        if (hasElevationTexture) {
            float2 uv = bilerp_fast(vA.uv, vB.uv, vC.uv, vD.uv, p);
            float3 radialNormal = normalize(position.xyz + origin + originOffset);
            float elevation = decodeElevation(elevationTexture0.sample(sampler0, uv).rgb);
            const float ElevationScale = 1.0 / 4000000.0;
            position.xyz += radialNormal * elevation * ElevationScale;
        }
    }

    return position.xyz;
}

[[patch(quad, 4)]] vertex VertexOutDisplaced
quadTessellationDisplacementVertexShader(const patch_control_point<Vertex3DTextureTessellatedIn> controlPoints [[stage_in]],
                                         const float2 positionInPatch [[position_in_patch]],
                                         constant float4x4 &vpMatrix [[buffer(1)]],
                                         constant float4x4 &mMatrix [[buffer(2)]],
                                         constant float4 &originOffset [[buffer(3)]],
                                         constant float4 &origin [[buffer(4)]],
                                         constant bool &is3d [[buffer(5)]],
                                         constant bool &hasElevationTexture [[buffer(6)]],
                                         texture2d<float> elevationTexture0 [[ texture(0)]],
                                         sampler sampler0 [[sampler(0)]])
{
    Vertex3DTextureTessellatedIn vA = controlPoints[0];
    Vertex3DTextureTessellatedIn vB = controlPoints[1];
    Vertex3DTextureTessellatedIn vC = controlPoints[2];
    Vertex3DTextureTessellatedIn vD = controlPoints[3];
    half2 p = half2(positionInPatch);

    float2 uv = bilerp_fast(vA.uv, vB.uv, vC.uv, vD.uv, p);
    float3 localPosition = displacedLocalPosition(vA, vB, vC, vD, p, origin.xyz, originOffset.xyz, is3d, hasElevationTexture, elevationTexture0, sampler0);
    float3 localNormal = normalize(localPosition + origin.xyz + originOffset.xyz);

    if (is3d) {
        float2 uvDx = mix(vB.uv - vA.uv, vD.uv - vC.uv, float(p.y));
        float2 uvDy = mix(vC.uv - vA.uv, vD.uv - vB.uv, float(p.x));

        if (hasElevationTexture) {
            float texelUv = 1.0 / 512.0;
            uint maxDim = max(elevationTexture0.get_width(), elevationTexture0.get_height());
            texelUv = 1.0 / max(float(maxDim), 1.0);
            
            float patchStepX = clamp(texelUv / max(length(uvDx), 1e-5), texelUv, 0.25);
            float patchStepY = clamp(texelUv / max(length(uvDy), 1e-5), texelUv, 0.25);
            
            half2 pPlusX = half2(clamp(float2(p) + float2(patchStepX, 0.0), 0.0, 1.0));
            half2 pMinusX = half2(clamp(float2(p) - float2(patchStepX, 0.0), 0.0, 1.0));
            half2 pPlusY = half2(clamp(float2(p) + float2(0.0, patchStepY), 0.0, 1.0));
            half2 pMinusY = half2(clamp(float2(p) - float2(0.0, patchStepY), 0.0, 1.0));
            
            float3 localPosPlusX = displacedLocalPosition(vA, vB, vC, vD, pPlusX, origin.xyz, originOffset.xyz, is3d, hasElevationTexture, elevationTexture0, sampler0);
            float3 localPosMinusX = displacedLocalPosition(vA, vB, vC, vD, pMinusX, origin.xyz, originOffset.xyz, is3d, hasElevationTexture, elevationTexture0, sampler0);
            float3 localPosPlusY = displacedLocalPosition(vA, vB, vC, vD, pPlusY, origin.xyz, originOffset.xyz, is3d, hasElevationTexture, elevationTexture0, sampler0);
            float3 localPosMinusY = displacedLocalPosition(vA, vB, vC, vD, pMinusY, origin.xyz, originOffset.xyz, is3d, hasElevationTexture, elevationTexture0, sampler0);
            
            float3 tangentX = localPosPlusX - localPosMinusX;
            float3 tangentY = localPosPlusY - localPosMinusY;
            float3 slopeNormal = normalize(cross(tangentY, tangentX));
            if (all(isfinite(slopeNormal))) {
                localNormal = slopeNormal;
            }
            
            float3 radialNormal = normalize(localPosition + origin.xyz + originOffset.xyz);
            if (dot(localNormal, radialNormal) < 0.0) {
                localNormal = -localNormal;
            }
        }
    }

    float3 worldPosition = ((mMatrix * float4(localPosition, 1)) + originOffset).xyz;
    float3 worldNormal = normalize((mMatrix * float4(localNormal, 0)).xyz);

    VertexOutDisplaced out {
        .position = vpMatrix * float4(worldPosition, 1),
        .uv = uv,
        .worldPosition = worldPosition,
        .worldNormal = worldNormal
    };

    return out;
}
