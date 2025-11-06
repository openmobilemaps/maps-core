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

// ELEVATION PROTOTYPE TEST
/**
 * Decode elevation from RGBA encoded altitude texture
 * This matches the existing decoding logic in the shader
 */
/*
float decodeElevation(float4 rgbaAltitude) {
    return (rgbaAltitude.r * 256.0 * 256.0 * 255.0 +
            rgbaAltitude.g * 256.0 * 255.0 +
            rgbaAltitude.b * 255.0) / 10.0 - 10000.0;
}
*/

/**
 * Sample elevation with linear interpolation
 * This function first samples the four neighboring pixels, decodes each elevation value,
 * then performs bilinear interpolation on the decoded values
 *
 * @param altitudeTexture The altitude texture with encoded elevation data
 * @param uv The texture coordinates to sample
 * @param linearSampler A linear sampler for texture sampling
 * @return The linearly interpolated elevation value
 */
/*
float sampleLinearElevation(texture2d<float> altitudeTexture, float2 uv, sampler linearSampler) {
    // Get texture dimensions
    float2 texSize = float2(altitudeTexture.get_width(), altitudeTexture.get_height());

    // Convert UV to pixel coordinates
    float2 pixelCoord = uv * texSize - 0.5;
    float2 pixelFloor = floor(pixelCoord);
    float2 pixelFrac = pixelCoord - pixelFloor;

    // Calculate UV coordinates for the four neighboring pixels
    float2 texelSize = 1.0 / texSize;
    float2 uv00 = (pixelFloor + float2(0.0, 0.0)) * texelSize;
    float2 uv10 = (pixelFloor + float2(1.0, 0.0)) * texelSize;
    float2 uv01 = (pixelFloor + float2(0.0, 1.0)) * texelSize;
    float2 uv11 = (pixelFloor + float2(1.0, 1.0)) * texelSize;

    // Sample and decode the four neighboring pixels
    // Use nearest sampling to get the raw encoded values
    constexpr sampler nearestSampler(coord::normalized, address::clamp_to_edge, filter::nearest);

    float elevation00 = decodeElevation(altitudeTexture.sample(nearestSampler, uv00));
    float elevation10 = decodeElevation(altitudeTexture.sample(nearestSampler, uv10));
    float elevation01 = decodeElevation(altitudeTexture.sample(nearestSampler, uv01));
    float elevation11 = decodeElevation(altitudeTexture.sample(nearestSampler, uv11));

    // Perform bilinear interpolation on the decoded elevation values
    float elevation0 = mix(elevation00, elevation10, pixelFrac.x);
    float elevation1 = mix(elevation01, elevation11, pixelFrac.x);
    return mix(elevation0, elevation1, pixelFrac.y);
}
*/

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

float3 transform(float3 coordinate, float3 origin) {
    float x = 1.0 * sin(coordinate.y) * cos(coordinate.x) - origin.x;
    float y = 1.0 * cos(coordinate.y) - origin.y;
    float z = -1.0 * sin(coordinate.y) * sin(coordinate.x) - origin.z;
    return float3(x, y, z);
}

[[patch(quad, 4)]] vertex VertexOut
quadTessellationVertexShader(const patch_control_point<TessellatedVertex3DTextureIn> controlPoints [[stage_in]],
                             const float2 positionInPatch [[position_in_patch]],
                             constant float4x4 &vpMatrix [[buffer(1)]],
                             constant float4x4 &mMatrix [[buffer(2)]],
                             constant float3 &originOffset [[buffer(3)]],
                             constant float3 &origin [[buffer(4)]],
                             /* ELEVATION PROTOTYPE TEST
                             texture2d<float> texture0 [[ texture(0)]],
                             sampler sampler0 [[sampler(0)]], */
                             constant bool &is3d [[buffer(5)]])
{
    TessellatedVertex3DTextureIn vA = controlPoints[0];
    TessellatedVertex3DTextureIn vB = controlPoints[1];
    TessellatedVertex3DTextureIn vC = controlPoints[2];
    TessellatedVertex3DTextureIn vD = controlPoints[3];
    
    float3 vertexRelativePosition = bilerp(vA.relativePosition, vB.relativePosition, vC.relativePosition, vD.relativePosition, positionInPatch);
    float3 vertexAbsolutePosition = bilerp(vA.absolutePosition, vB.absolutePosition, vC.absolutePosition, vD.absolutePosition, positionInPatch);
    float2 vertexUV = bilerp(vA.uv, vB.uv, vC.uv, vD.uv, positionInPatch);
    
    float3 position = vertexRelativePosition;
    if (is3d) {
        float3 bent = transform(vertexAbsolutePosition.xyz, origin) - originOffset;
        float blend = saturate(length(originOffset) * BlendScale - BlendOffset);
        position = mix(position, bent, blend);
        
        /* ELEVATION PROTOTYPE TEST
        float3 normal = normalize(transform(vertexAbsolutePosition, float3(0, 0, 0)));
        position += normal * sampleLinearElevation(texture0, vertexUV, sampler0) * 0.00001 * 1.0; */
    }
     
    VertexOut out {
        .position = vpMatrix * ((mMatrix * float4(position, 1)) + float4(originOffset, 0)),
        .uv = vertexUV
    };
  
    return out;
}

[[patch(triangle, 3)]] vertex VertexOut
polygonTessellationVertexShader(const patch_control_point<Vertex4FIn> controlPoints [[stage_in]],
                                const float3 positionInPatch [[position_in_patch]],
                                constant float4x4 &vpMatrix [[buffer(1)]],
                                constant float4x4 &mMatrix [[buffer(2)]],
                                constant float4 &originOffset [[buffer(3)]])
{
    Vertex4FIn vA = controlPoints[0];
    Vertex4FIn vB = controlPoints[1];
    Vertex4FIn vC = controlPoints[2];
    
    float4 vertexPosition = baryinterp(vA.position, vB.position, vC.position, positionInPatch);
     
    VertexOut out {
        .position = vpMatrix * (float4(vertexPosition.xyz, 1) + originOffset),
    };

    return out;
}
