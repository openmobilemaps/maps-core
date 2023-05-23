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
stretchVertexShader(const VertexIn vertexIn [[stage_in]],
                    constant float4x4 &mvpMatrix [[buffer(1)]])
{
    VertexOut out {
        .position = mvpMatrix * float4(vertexIn.position.xy, 0.0, 1.0),
        .uv = vertexIn.uv
    };

    return out;
}

struct StretchShaderInfo  {
    float scaleX;
    /** all stretch infos are between 0 and 1 */
    float stretchX0Begin;
    float stretchX0End;
    float stretchX1Begin;
    float stretchX1End;
    float scaleY;

    /** all stretch infos are between 0 and 1 */
    float stretchY0Begin;
    float stretchY0End;
    float stretchY1Begin;
    float stretchY1End;

    float2 uvOrig;
    float2 uvSize;
};

fragment float4
stretchFragmentShader(VertexOut in [[stage_in]],
                      constant float &alpha [[buffer(1)]],
                      constant StretchShaderInfo *stretchInfo [[buffer(2)]],
                      texture2d<float> texture0 [[ texture(0)]],
                      sampler textureSampler [[sampler(0)]])
{
    const StretchShaderInfo info = stretchInfo[0];

    // All computed in normalized uv space of this single sprite
    float2 texCoordNorm = (in.uv - info.uvOrig) / info.uvSize;

    // X
    if (info.stretchX0Begin != info.stretchX0End) {
        const float sumStretchedX = (info.stretchX0End - info.stretchX0Begin) + (info.stretchX1End - info.stretchX1Begin);
        const float scaleStretchX = (sumStretchedX * info.scaleX) / (1.0 - (info.scaleX - sumStretchedX * info.scaleX));

        const float totalOffset = min(texCoordNorm.x, info.stretchX0Begin) // offsetPre0Unstretched
                            + (clamp(texCoordNorm.x, info.stretchX0Begin, info.stretchX0End) - info.stretchX0Begin) / scaleStretchX // offset0Stretched
                            + (clamp(texCoordNorm.x, info.stretchX0End, info.stretchX1Begin) - info.stretchX0End) // offsetPre1Unstretched
                            + (clamp(texCoordNorm.x, info.stretchX1Begin, info.stretchX1End) - info.stretchX1Begin) / scaleStretchX // offset1Stretched
                            + (clamp(texCoordNorm.x, info.stretchX1End, 1.0) - info.stretchX1End); // offsetPost1Unstretched
        texCoordNorm.x = totalOffset * info.scaleX;
    }

    // Y
    if (info.stretchY0Begin != info.stretchY0End) {
        const float sumStretchedY = (info.stretchY0End - info.stretchY0Begin) + (info.stretchY1End - info.stretchY1Begin);
        const float scaleStretchY = (sumStretchedY * info.scaleY) / (1.0 - (info.scaleY - sumStretchedY * info.scaleY));

        const float totalOffset = min(texCoordNorm.y, info.stretchY0Begin) // offsetPre0Unstretched
                            + (clamp(texCoordNorm.y, info.stretchY0Begin, info.stretchY0End) - info.stretchY0Begin) / scaleStretchY // offset0Stretched
                            + (clamp(texCoordNorm.y, info.stretchY0End, info.stretchY1Begin) - info.stretchY0End) // offsetPre1Unstretched
                            + (clamp(texCoordNorm.y, info.stretchY1Begin, info.stretchY1End) - info.stretchY1Begin) / scaleStretchY // offset1Stretched
                            + (clamp(texCoordNorm.y, info.stretchY1End, 1.0) - info.stretchY1End); // offsetPost1Unstretched
        texCoordNorm.y = totalOffset * info.scaleY;
    }

    // remap final normalized uv to sprite atlas coordinates
    texCoordNorm = texCoordNorm * info.uvSize + info.uvOrig;

    const float4 color = texture0.sample(textureSampler, texCoordNorm);
    const float a = color.a * alpha;
    if (a == 0) {
        discard_fragment();
    }
    return float4(color.r * a, color.g * a, color.b * a, a);
}
