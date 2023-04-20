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

    const float2 normalizedUV = (in.uv - info.uvOrig) / info.uvSize * float2(info.scaleX, info.scaleY);

    float2 mappedUV = in.uv;

    const float countRegionX = float(info.stretchX0Begin != info.stretchX0End) + float(info.stretchX1Begin != info.stretchX1End);
    const float countRegionY = float(info.stretchY0Begin != info.stretchY0End) + float(info.stretchY1Begin != info.stretchY1End);

    //X
    if (countRegionX != 0) {
        const float strechedRegionX = info.stretchX0End - info.stretchX0Begin + info.stretchX1End - info.stretchX1Begin;

        const float notStrechedRegionX = 1 - strechedRegionX;

        const float overflowRegion0X = (info.scaleX - notStrechedRegionX) / countRegionX;
        const float overflowRegion1X = countRegionX == 2.0 ? overflowRegion0X : 0;

        const float startXRegion0 = info.stretchX0Begin;
        const float endXRegion0 = startXRegion0 + overflowRegion0X;
        const float startXRegion1 = endXRegion0 + (info.stretchX1Begin - info.stretchX0End);
        const float endXRegion1 = startXRegion1 + overflowRegion1X;
        const float endX = info.scaleX;

        if (normalizedUV.x < startXRegion0) {;
            mappedUV.x = (normalizedUV.x / startXRegion0) * info.stretchX0Begin * info.uvSize.x + info.uvOrig.x;
        } else if (normalizedUV.x >= startXRegion0 && normalizedUV.x < endXRegion0) {
            mappedUV.x = ((((normalizedUV.x - startXRegion0)  / (endXRegion0 - startXRegion0)) * (info.stretchX0End - info.stretchX0End)) + info.stretchX0End) * info.uvSize.x + info.uvOrig.x;
        } else if (normalizedUV.x >= endXRegion0 && normalizedUV.x < startXRegion1) {
            mappedUV.x = ((((normalizedUV.x - startXRegion1)  / (endXRegion1 - startXRegion1)) * (1 - info.stretchX1End)) + info.stretchX1End) * info.uvSize.x + info.uvOrig.x;
        } else if (normalizedUV.x >= startXRegion1 && normalizedUV.x < endXRegion1) {
            mappedUV.x = ((((normalizedUV.x - startXRegion1)  / (endXRegion1 - startXRegion1)) * (info.stretchX1End - info.stretchX1End)) + info.stretchX1End) * info.uvSize.x + info.uvOrig.x;
        } else {
            mappedUV.x = ((((normalizedUV.x - endXRegion1)  / (endX - endXRegion1)) * (1 - info.stretchX1End)) + info.stretchX1End) * info.uvSize.x + info.uvOrig.x;
        }
    }


    //Y
    if (countRegionY != 0) {
        const float strechedRegionY = info.stretchY0End - info.stretchY0Begin + info.stretchY1End - info.stretchY1Begin;

        const float notStrechedRegionY = 1 - strechedRegionY;

        const float overflowRegion0Y = (info.scaleY - notStrechedRegionY) / countRegionY;
        const float overflowRegion1Y = countRegionY == 2.0 ? overflowRegion0Y : 0;

        const float startYRegion0 = info.stretchY0Begin;
        const float endYRegion0 = startYRegion0 + overflowRegion0Y;
        const float startYRegion1 = endYRegion0 + (info.stretchY1Begin - info.stretchY0End);
        const float endYRegion1 = startYRegion1 + overflowRegion1Y;
        const float endY = info.scaleY;

        if (normalizedUV.y < startYRegion0) {
            mappedUV.y = (normalizedUV.y / startYRegion0) * info.stretchY0Begin * info.uvSize.y + info.uvOrig.y;
        } else if (normalizedUV.y >= startYRegion0 && normalizedUV.y < endYRegion0) {
            mappedUV.y = ((((normalizedUV.y - startYRegion0)  / (endYRegion0 - startYRegion0)) * (info.stretchY0End - info.stretchY0End)) + info.stretchY0End) * info.uvSize.y + info.uvOrig.y;
        } else if (normalizedUV.y >= endYRegion0 && normalizedUV.y < startYRegion1) {
            mappedUV.y = ((((normalizedUV.y - startYRegion1)  / (endYRegion1 - startYRegion1)) * (1 - info.stretchY1End)) + info.stretchY1End) * info.uvSize.y + info.uvOrig.y;
        } else if (normalizedUV.y >= startYRegion1 && normalizedUV.y < endYRegion1) {
            mappedUV.y = ((((normalizedUV.y - startYRegion1)  / (endYRegion1 - startYRegion1)) * (info.stretchY1End - info.stretchY1End)) + info.stretchY1End) * info.uvSize.y + info.uvOrig.y;
        } else {
            mappedUV.y = ((((normalizedUV.y - endYRegion1)  / (endY - endYRegion1)) * (1 - info.stretchY1End)) + info.stretchY1End) * info.uvSize.y + info.uvOrig.y;
        }
    }

    const float4 color = texture0.sample(textureSampler, mappedUV);
    
    const float a = color.a * alpha;

    if (a == 0) {
        discard_fragment();
    }

    return float4(color.r * a, color.g * a, color.b * a, a);
}
