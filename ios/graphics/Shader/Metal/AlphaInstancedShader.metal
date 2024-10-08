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

struct InstancedVertexOut {
    float4 position [[ position ]];
    float2 uv;
    float2 uvOrig;
    float2 uvSize;
    float alpha;
};

vertex InstancedVertexOut
unitSphereAlphaInstancedVertexShader(const VertexIn vertexIn [[stage_in]],
                           constant float4x4 &vpMatrix [[buffer(1)]],
                           constant float4x4 &mMatrix [[buffer(2)]],
                           constant float2 *positions [[buffer(3)]],
                           constant float2 *scales [[buffer(4)]],
                           constant float *rotations [[buffer(5)]],
                           constant float2 *texureCoordinates [[buffer(6)]],
                           constant float *alphas [[buffer(7)]],
                           constant float2 *offsets [[buffer(8)]],
                           uint instanceId [[instance_id]])
{
    const float2 position = positions[instanceId];
    const float2 scale = scales[instanceId];
    const float2 offset = offsets[instanceId];
    const float rotation = rotations[instanceId];
    float alpha = alphas[instanceId];

    const float angle = rotation * M_PI_F / 180.0;

    const float x = 1.0 * sin(position.y) * cos(position.x);
    const float y = 1.0 * cos(position.y);
    const float z = -1.0 * sin(position.y) * sin(position.x);

    float4 earthCenter = vpMatrix * float4(0,0,0, 1.0);
    float4 screenPosition = vpMatrix * float4(x,y,z, 1.0);

    earthCenter /= earthCenter.w;
    screenPosition /= screenPosition.w;

    auto diffCenter = screenPosition - earthCenter;

    if (diffCenter.z > 0) {
        alpha = 0.0;
    }

    const float4x4 scaleRotateMatrix = float4x4(float4(cos(angle), -sin(angle), 0, 0),
                                           float4(sin(angle), cos(angle), 0, 0),
                                           float4(0, 0, 0, 0),
                                           float4(vertexIn.position.xy * scale + offset, 1.0, 1));


    InstancedVertexOut out {
        .position = scaleRotateMatrix * screenPosition,
        .uv = vertexIn.uv,
        .uvOrig = texureCoordinates[instanceId * 2],
        .uvSize = texureCoordinates[instanceId * 2 + 1],
        .alpha = alpha
    };

    return out;
}


fragment float4
unitSphereAlphaInstancedFragmentShader(InstancedVertexOut in [[stage_in]],
                             texture2d<float> texture0 [[ texture(0)]],
                             sampler textureSampler [[sampler(0)]])
{
    const float2 uv = in.uvOrig + in.uvSize * float2(in.uv.x, in.uv.y);
    float4 color = texture0.sample(textureSampler, uv);

    const float a = color.a * in.alpha;
    return float4(color.r * in.alpha, color.g * in.alpha, color.b * in.alpha, a);
}

vertex InstancedVertexOut
alphaInstancedVertexShader(const VertexIn vertexIn [[stage_in]],
                           constant float4x4 &vpMatrix [[buffer(1)]],
                           constant float4x4 &mMatrix [[buffer(2)]],
                           constant float2 *positions [[buffer(3)]],
                           constant float2 *scales [[buffer(4)]],
                           constant float *rotations [[buffer(5)]],
                           constant float2 *texureCoordinates [[buffer(6)]],
                           constant float *alphas [[buffer(7)]],
                           uint instanceId [[instance_id]])
{
  const float2 position = positions[instanceId];
  const float2 scale = scales[instanceId];
  const float rotation = rotations[instanceId];


  const float angle = rotation * M_PI_F / 180.0;

  const float4x4 model_matrix = float4x4(
                                            float4(cos(angle) * scale.x, -sin(angle) * scale.x, 0, 0),
                                            float4(sin(angle) * scale.y, cos(angle) * scale.y, 0, 0),
                                            float4(0, 0, 0, 0),
                                            float4(position.x, position.y, 0.0, 1)
                                            );

  const float4x4 matrix = vpMatrix * model_matrix;

  InstancedVertexOut out {
    .position = matrix * float4(vertexIn.position.xy, 0.0, 1.0),
    .uv = vertexIn.uv,
    .uvOrig = texureCoordinates[instanceId * 2],
    .uvSize = texureCoordinates[instanceId * 2 + 1],
    .alpha = alphas[instanceId]
  };

  return out;

}


fragment float4
alphaInstancedFragmentShader(InstancedVertexOut in [[stage_in]],
                             texture2d<float> texture0 [[ texture(0)]],
                             sampler textureSampler [[sampler(0)]])
{
    const float2 uv = in.uvOrig + in.uvSize * float2(in.uv.x, 1 - in.uv.y);
    float4 color = texture0.sample(textureSampler, uv);

    const float a = color.a * in.alpha;

    return float4(color.r * in.alpha, color.g * in.alpha, color.b * in.alpha, a);
}
