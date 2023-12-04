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
using namespace metal;

struct PolygonGroupVertexIn {
    float2 position [[attribute(0)]];
    float stylingIndex [[attribute(1)]];
};

struct PolygonGroupVertexOut {
    float4 position [[ position ]];
    float stylingIndex;
};

struct PolygonGroupStyling {
    float color[4];
    float opacity;
};


vertex PolygonGroupVertexOut
polygonGroupVertexShader(const PolygonGroupVertexIn vertexIn [[stage_in]],
                 constant float4x4 &mvpMatrix [[buffer(1)]])
{
    PolygonGroupVertexOut out {
        .position = mvpMatrix * float4(vertexIn.position.xy, 0.0, 1.0),
        .stylingIndex = vertexIn.stylingIndex,
    };

    return out;
}

fragment float4
polygonGroupFragmentShader(PolygonGroupVertexOut in [[stage_in]],
                        constant PolygonGroupStyling *styling [[buffer(1)]])
{
    PolygonGroupStyling s = styling[int(in.stylingIndex)];
    return float4(s.color[0], s.color[1], s.color[2], 1.0) * s.opacity * s.color[3];
}

struct PolygonPatternGroupVertexOut {
    float4 position [[ position ]];
    float stylingIndex;
    float2 pixelPosition;
};


vertex PolygonPatternGroupVertexOut
polygonPatternGroupVertexShader(const PolygonGroupVertexIn vertexIn [[stage_in]],
                                constant float4x4 &mvpMatrix [[buffer(1)]],
                                constant float2 &scalingFactor [[buffer(2)]])
{
  float2 pixelPosition = vertexIn.position.xy *  float2(1 / scalingFactor.x, 1 / scalingFactor.y);

  PolygonPatternGroupVertexOut out {
        .position = mvpMatrix * float4(vertexIn.position.xy, 0.0, 1.0),
        .stylingIndex = vertexIn.stylingIndex,
        .pixelPosition = pixelPosition
    };

    return out;
}

fragment float4
polygonPatternGroupFragmentShader(PolygonPatternGroupVertexOut in [[stage_in]],
                                  texture2d<float> texture0 [[ texture(0)]],
                                  sampler textureSampler [[sampler(0)]],
                                  constant float *opacity [[buffer(0)]],
                                  constant float *texureCoordinates [[buffer(1)]])
{
    int offset = int(in.stylingIndex * 5);
    const float2 uvOrig = float2(texureCoordinates[offset + 0], texureCoordinates[offset + 1]);
    const float2 uvSize = float2(texureCoordinates[offset + 2], texureCoordinates[offset + 3]);
    const int combined = int(texureCoordinates[offset + 4]);
    const float2 pixelSize = float2(combined & 0xFF, combined >> 16);

    const float2 uv = fmod(fmod(in.pixelPosition, pixelSize) / pixelSize + float2(1.0, 1.0), float2(1.0, 1.0));
    const float2 texUv = uvOrig + uvSize * float2(uv.x, uv.y);
    const float4 color = texture0.sample(textureSampler, texUv);

    const float a = color.a * opacity[int(in.stylingIndex)];

    return float4(color.r * a, color.g * a, color.b * a, a);
}
