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

fragment half4
skySphereFragmentShader(VertexOut in [[stage_in]],
                        constant const float4 *alpha [[buffer(0)]],
                        constant float4 &cameraPosition [[buffer(1)]],
                        constant float4x4 &inverseVPMatrix [[buffer(2)]],
                        texture2d<half> texture0 [[ texture(0)]],
                        sampler textureSampler [[sampler(0)]])
{
    // Transform screen position to Cartesian coordinates
    float4 posCart = inverseVPMatrix * float4(in.uv.x, -in.uv.y, 1.0, 1.0);
    posCart /= posCart.w;

    // Compute direction relative to the camera
    float3 dirCamera = normalize(posCart.xyz - cameraPosition.xyz);

    // Calculate spherical coordinates (right ascension and declination)
    float rasc = atan2(dirCamera.z, dirCamera.x) + M_PI_F;
    float decl = asin(dirCamera.y);

    // Texture coordinates
    float2 texCoords = float2(
        -(rasc / (2.0 * M_PI_F)) + 1.0,
        -decl / M_PI_F + 0.5
    );

    // Sample the texture
    return texture0.sample(textureSampler, texCoords);
}

//vec4 posCart = uInverseVPMatrix * vec4(v_screenPos.x, v_screenPos.y, 1.0, 1.0);
//posCart /= posCart.w;
//
//// Assume the sky on a sphere around the camera (and not the earth's center)
//vec3 dirCamera = normalize(posCart.xyz - uCameraPosition.xyz);
//
//float rasc = atan(dirCamera.z, dirCamera.x) + PI;
//float decl = asin(dirCamera.y);
//
//vec2 texCoords = vec2(
//        -(rasc / (2.0 * PI)) + 1.0,
//        -decl / PI + 0.5
//) * v_textureScaleFactors;
