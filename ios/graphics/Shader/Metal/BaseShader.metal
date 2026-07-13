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
baseVertexShader(const Vertex3DTextureIn vertexIn [[stage_in]],
                constant float4x4 &vpMatrix [[buffer(MC_GLOBAL_VP_MATRIX_BUFFER_INDEX)]],
                 constant float4x4 &mMatrix [[buffer(2)]],
                 constant float4 &originOffset [[buffer(3)]])
{
    float4x4 modelToWorldCameraCentric = mMatrix;
    modelToWorldCameraCentric[3] += originOffset;
    VertexOut out {
        .position = vpMatrix * (modelToWorldCameraCentric * vertexIn.position),
        .uv = vertexIn.uv
    };

    return out;
}

vertex VertexOut
baseVertexShaderModel(const Vertex3DTextureIn vertexIn [[stage_in]],
                constant float4x4 &vpMatrix [[buffer(MC_GLOBAL_VP_MATRIX_BUFFER_INDEX)]],
                 constant float4x4 &mMatrix [[buffer(2)]],
                 constant float4 &originOffset [[buffer(3)]])
{
    float4x4 modelToWorldCameraCentric = mMatrix;
    modelToWorldCameraCentric[3] += originOffset;
    VertexOut out {
        .position = vpMatrix * (modelToWorldCameraCentric * vertexIn.position),
        .uv = vertexIn.uv
    };

    return out;
}

vertex VertexOut
roundColorVertexShader(const Vertex3DTextureIn vertexIn [[stage_in]],
                       constant float4x4 &vpMatrix [[buffer(MC_GLOBAL_VP_MATRIX_BUFFER_INDEX)]],
                       constant float4x4 &mMatrix [[buffer(2)]],
                       constant float4 &originOffset [[buffer(3)]],
                       constant float &scalingFactor [[buffer(MC_GLOBAL_SCREEN_PIXEL_FACTOR_BUFFER_INDEX)]])
{
    float4x4 modelToWorldCameraCentric = mMatrix;
    modelToWorldCameraCentric[3] += originOffset;

    float4 scaledPosition = vertexIn.position;
    scaledPosition.xy *= scalingFactor;

    VertexOut out {
        .position = vpMatrix * (modelToWorldCameraCentric * scaledPosition),
        .uv = vertexIn.uv
    };

    return out;
}

vertex VertexOut
unitSphereRoundColorVertexShader(const Vertex3DTextureIn vertexIn [[stage_in]],
                constant float4x4 &vpMatrix [[buffer(MC_GLOBAL_VP_MATRIX_BUFFER_INDEX)]],
                constant float4 &originOffset [[buffer(3)]],
                constant float2 &viewportSize [[buffer(4)]],
                constant float4 &origin [[buffer(MC_GLOBAL_ORIGIN_BUFFER_INDEX)]])
{
    float4 earthCenter = vpMatrix * float4(-origin.x, -origin.y, -origin.z, 1.0);
    float4 screenPosition = vpMatrix * float4(originOffset.xyz, 1.0);

    earthCenter /= earthCenter.w;
    screenPosition /= screenPosition.w;

    const bool isVisible = screenPosition.z - earthCenter.z < 0.0;
    const float2 safeViewportSize = max(viewportSize, float2(1.0));
    const float2 screenSize = vertexIn.position.xy * (2.0 / safeViewportSize);

    VertexOut out {
        .position = isVisible
            ? float4(screenPosition.xy + screenSize, screenPosition.z, 1.0)
            : float4(-10.0, -10.0, -10.0, -10.0),
        .uv = vertexIn.uv
    };

    return out;
}

fragment half4
baseFragmentShader(VertexOut in [[stage_in]],
                      constant float &alpha [[buffer(1)]],
                      texture2d<half> texture0 [[ texture(0)]],
                      sampler textureSampler [[sampler(0)]])
{
    half4 color = texture0.sample(textureSampler, in.uv);

    float a = color.a * alpha;

    if (a == 0) {
       discard_fragment();
    }

    return half4(color.r * a, color.g * a, color.b * a, a);
}


vertex VertexOut
colorVertexShader(const Vertex3DIn vertexIn [[stage_in]],
                  constant float4x4 &vpMatrix [[buffer(MC_GLOBAL_VP_MATRIX_BUFFER_INDEX)]],
                   constant float4x4 &mMatrix [[buffer(2)]],
                  constant float4 &originOffset [[buffer(3)]])
{
    VertexOut out {
        .position = vpMatrix * (float4(vertexIn.position.xyz, 1.0) + originOffset),
    };

    return out;
}

fragment float4
colorFragmentShader(constant float4 &color [[buffer(1)]])
{
    float a = color.a;
    return float4(color.r * a, color.g * a, color.b * a, a);
}

fragment float4
wireframeFragmentShader()
{
    return float4(0.0, 0.0, 0.0, 1.0);
}

fragment half4
maskFragmentShader()
{
  return half4(0, 0, 0, 0);
}

fragment float4
roundColorFragmentShader(VertexOut in [[stage_in]],
                    constant float4 &fillColor [[buffer(1)]],
                    constant float4 &strokeColor [[buffer(2)]],
                    constant float &innerRadius [[buffer(3)]])
{
    const float dist = length(in.uv - float2(0.5));
    const float edgeWidth = fwidth(dist);
    const float innerRadiusTexCoord = innerRadius * 0.5;
    const float outerCoverage = 1.0 - smoothstep(0.5 - edgeWidth, 0.5, dist);
    const float fillCoverage = innerRadiusTexCoord > 0.0
        ? 1.0 - smoothstep(innerRadiusTexCoord - edgeWidth, innerRadiusTexCoord, dist)
        : 0.0;
    const float strokeCoverage = max(outerCoverage - fillCoverage, 0.0);
    const float alpha = fillColor.a * fillCoverage + strokeColor.a * strokeCoverage;

    if (alpha <= 0.0) {
        discard_fragment();
    }

    return float4(fillColor.rgb * fillColor.a * fillCoverage + strokeColor.rgb * strokeColor.a * strokeCoverage,
                  alpha);
}
