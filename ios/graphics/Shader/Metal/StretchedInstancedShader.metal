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

struct StretchedInstancedVertexOut {
    float4 position [[ position ]];
    float2 uv;
    float4 texureCoordinates;
    float alpha;
    uint16_t stretchInfoIndex;
};

vertex StretchedInstancedVertexOut
stretchInstancedVertexShader(const VertexIn vertexIn [[stage_in]],
                             constant float4x4 &mvpMatrix [[buffer(1)]],
                             constant float2 *positions [[buffer(2)]],
                             constant float2 *scales [[buffer(3)]],
                             constant float *rotations [[buffer(4)]],
                             constant float4 *texureCoordinates [[buffer(5)]],
                             constant float *alphas [[buffer(6)]],
                             ushort instanceId [[instance_id]])
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


    const float4x4 matrix = mvpMatrix * model_matrix;

    StretchedInstancedVertexOut out {
        .position = matrix * float4(vertexIn.position.xy, 0.0, 1.0),
        .uv = vertexIn.uv,
        .texureCoordinates = texureCoordinates[instanceId],
        .alpha = alphas[instanceId],
        .stretchInfoIndex = instanceId
    };

    return out;
}

fragment float4
stretchInstancedFragmentShader(StretchedInstancedVertexOut in [[stage_in]],
                               constant float *stretchInfos [[buffer(1)]],
                               texture2d<float> texture0 [[ texture(0)]],
                               sampler textureSampler [[sampler(0)]])
{
    if (in.alpha == 0) {
        discard_fragment();
    }

    const int infoOffset = in.stretchInfoIndex * 10;
const float scaleX = stretchInfos[infoOffset + 0];
const float scaleY = stretchInfos[infoOffset + 1];
const float stretchX0Begin = stretchInfos[infoOffset + 2];
const float stretchX0End = stretchInfos[infoOffset + 3];
const float stretchX1Begin = stretchInfos[infoOffset + 4];
const float stretchX1End = stretchInfos[infoOffset + 5];
    const float stretchY0Begin = stretchInfos[infoOffset + 6];
    const float stretchY0End = stretchInfos[infoOffset + 7];
    const float stretchY1Begin = stretchInfos[infoOffset + 8];
    const float stretchY1End = stretchInfos[infoOffset + 9];

    // All computed in normalized uv space of this single sprite
    float2 texCoordNorm = in.uv;

    // X
    if (stretchX0Begin != stretchX0End) {
        const float sumStretchedX = (stretchX0End - stretchX0Begin) + (stretchX1End - stretchX1Begin);
        const float scaleStretchX = (sumStretchedX * scaleX) / (1.0 - (scaleX - sumStretchedX * scaleX));

        const float totalOffset = min(texCoordNorm.x, stretchX0Begin) // offsetPre0Unstretched
        + (clamp(texCoordNorm.x, stretchX0Begin, stretchX0End) - stretchX0Begin) / scaleStretchX // offset0Stretched
        + (clamp(texCoordNorm.x, stretchX0End, stretchX1Begin) - stretchX0End) // offsetPre1Unstretched
        + (clamp(texCoordNorm.x, stretchX1Begin, stretchX1End) - stretchX1Begin) / scaleStretchX // offset1Stretched
        + (clamp(texCoordNorm.x, stretchX1End, 1.0) - stretchX1End); // offsetPost1Unstretched
        texCoordNorm.x = totalOffset * scaleX;
    }

    // Y
    if (stretchY0Begin != stretchY0End) {
        const float sumStretchedY = (stretchY0End - stretchY0Begin) + (stretchY1End - stretchY1Begin);
        const float scaleStretchY = (sumStretchedY * scaleY) / (1.0 - (scaleY - sumStretchedY * scaleY));

        const float totalOffset = min(texCoordNorm.y, stretchY0Begin) // offsetPre0Unstretched
        + (clamp(texCoordNorm.y, stretchY0Begin, stretchY0End) - stretchY0Begin) / scaleStretchY // offset0Stretched
        + (clamp(texCoordNorm.y, stretchY0End, stretchY1Begin) - stretchY0End) // offsetPre1Unstretched
        + (clamp(texCoordNorm.y, stretchY1Begin, stretchY1End) - stretchY1Begin) / scaleStretchY // offset1Stretched
        + (clamp(texCoordNorm.y, stretchY1End, 1.0) - stretchY1End); // offsetPost1Unstretched
        texCoordNorm.y = totalOffset * scaleY;
    }

    // remap final normalized uv to sprite atlas coordinates

    const float4 color = texture0.sample(textureSampler, in.texureCoordinates.xy + in.texureCoordinates.zw * float2(texCoordNorm.x, 1 - texCoordNorm.y));
    const float a = color.a * in.alpha;
    if (a == 0) {
        discard_fragment();
    }
    return float4(color.r * a, color.g * a, color.b * a, a);
}
