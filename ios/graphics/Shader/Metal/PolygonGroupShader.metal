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

struct PolygonGroupVertexOut {
    float4 position [[ position ]];
    float stylingIndex;
};

struct PolygonGroupStripedVertexOut {
  float4 position [[ position ]];
  float2 uv;
  float stylingIndex;
};

struct PolygonGroupStyling {
    float color[4];
    float opacity;
};

struct PolygonGroupStripeStyling {
    float color[4];
    float opacity;
    float stripeInfoX;
    float stripeInfoY;
};

vertex PolygonGroupVertexOut
polygonGroupVertexShader(const Vertex4FIn vertexIn [[stage_in]],
                 constant float4x4x2 &vpMatrix [[buffer(1)]],
                         ushort amp_id [[amplification_id]],
                 constant float4 &originOffset [[buffer(2)]])
{
    PolygonGroupVertexOut out {
        .position = vpMatrix.matrices[amp_id] * (float4(vertexIn.position.xyz, 1.0) + originOffset),
        .stylingIndex = vertexIn.position.w,
    };

    return out;
}

fragment half4
polygonGroupFragmentShader(PolygonGroupVertexOut in [[stage_in]],
                        constant PolygonGroupStyling *styling [[buffer(1)]])
{
    PolygonGroupStyling s = styling[int(in.stylingIndex)];
    return half4(s.color[0], s.color[1], s.color[2], 1.0) * s.opacity * s.color[3];
}

struct PolygonPatternGroupVertexOut {
    float4 position [[ position ]];
    float stylingIndex;
    float2 pixelPosition;
};

vertex PolygonGroupStripedVertexOut
polygonStripedGroupVertexShader(const Vertex4FIn vertexIn [[stage_in]],
                                constant float4x4x2 &vpMatrix [[buffer(1)]],
                                ushort amp_id [[amplification_id]],
                                constant float4 &originOffset [[buffer(2)]],
                                constant float2 &posOffset [[buffer(3)]]
                                )
{
    PolygonGroupStripedVertexOut out {
        .position = vpMatrix.matrices[amp_id] * (float4(vertexIn.position.xyz, 1.0) + originOffset),
        .uv = vertexIn.position.xy - posOffset,
        .stylingIndex = vertexIn.position.w,
    };

    return out;
}


fragment half4
polygonGroupStripedFragmentShader(PolygonGroupStripedVertexOut in [[stage_in]],
                                  constant PolygonGroupStripeStyling *styling [[buffer(1)]],
                                  constant float2 &scaleFactors [[buffer(2)]])
{
    PolygonGroupStripeStyling s = styling[int(in.stylingIndex)];

    float disPx = ((in.uv.x - 1100000.0) + (in.uv.y + 6000000.0)) / scaleFactors.y;
    float totalPx = s.stripeInfoX + s.stripeInfoY;
    float adjLineWPx = s.stripeInfoX / scaleFactors.y * scaleFactors.x;
    if (fmod(disPx, totalPx) > adjLineWPx) {
        return half4(0.0, 0.0, 0.0, 0.0);
    }

    return half4(s.color[0], s.color[1], s.color[2], 1.0) * s.opacity * s.color[3];
}

vertex PolygonPatternGroupVertexOut
polygonPatternGroupVertexShader(const Vertex4FIn vertexIn [[stage_in]],
                                constant float4x4x2 &vpMatrix [[buffer(1)]],
                                ushort amp_id [[amplification_id]],
                                constant float2 &scalingFactor [[buffer(2)]],
                                constant float2 &posOffset [[buffer(3)]],
                                constant float4 &originOffset [[buffer(4)]]) {
    float2 pixelPosition = (vertexIn.position.xy - posOffset) * float2(1.0 / scalingFactor.x, 1 / scalingFactor.y);

    PolygonPatternGroupVertexOut out {
        .position = vpMatrix.matrices[amp_id] * (float4(vertexIn.position.xyz, 1.0) + originOffset),
        .stylingIndex = vertexIn.position.w,
        .pixelPosition = pixelPosition
    };

    return out;
}

fragment half4
polygonPatternGroupFragmentShader(PolygonPatternGroupVertexOut in [[stage_in]],
                                  texture2d<half> texture0 [[ texture(0)]],
                                  sampler textureSampler [[sampler(0)]],
                                  constant half *opacity [[buffer(0)]],
                                  constant float *texureCoordinates [[buffer(1)]])
{
    int offset = int(in.stylingIndex * 5);
    const float2 uvOrig = float2(texureCoordinates[offset + 0], texureCoordinates[offset + 1]);
    const float2 uvSize = float2(texureCoordinates[offset + 2], texureCoordinates[offset + 3]);
    const int combined = int(texureCoordinates[offset + 4]);
    const float2 pixelSize = float2(combined & 0xFF, combined >> 16);

    const float2 uv = fmod(fmod(in.pixelPosition, pixelSize) / pixelSize + float2(1.0, 1.0), float2(1.0, 1.0));
    const float2 texUv = uvOrig + uvSize * float2(uv.x, uv.y);
    const half4 color = texture0.sample(textureSampler, texUv);

    const half a = color.a * opacity[int(in.stylingIndex)];

    return half4(color.r * a, color.g * a, color.b * a, a);
}

fragment half4
polygonPatternGroupFadeInFragmentShader(PolygonPatternGroupVertexOut in [[stage_in]],
                                  texture2d<half> texture0 [[ texture(0)]],
                                  sampler textureSampler [[sampler(0)]],
                                  constant half *opacity [[buffer(0)]],
                                  constant float *texureCoordinates [[buffer(1)]],
                                  constant float &screenPixelAsRealMeterFactor [[buffer(2)]],
                                  constant float2 &scalingFactor [[buffer(3)]])
{

    int offset = int(in.stylingIndex * 5);
    const float2 uvOrig = float2(texureCoordinates[offset], texureCoordinates[offset + 1]);
    const float2 uvSize = float2(texureCoordinates[offset + 2], texureCoordinates[offset + 3]);
    const int combined = int(texureCoordinates[offset + 4]);
    const float2 pixelSize = float2(combined & 0xFF, combined >> 16);

    const float scalingFactorFactor = (scalingFactor.x / screenPixelAsRealMeterFactor) - 1.0;
    const float2 spacing = pixelSize * scalingFactorFactor;
    const float2 totalSize = pixelSize + spacing;
    const float2 adjustedPixelPosition = in.pixelPosition + pixelSize * 0.5;
    float2 uvTot = fmod(adjustedPixelPosition, totalSize);

    const int yIndex = int(adjustedPixelPosition.y / totalSize.y) % 2;

    if(yIndex != 0 && uvTot.y <= pixelSize.y) {
        uvTot.x = fmod(adjustedPixelPosition.x + totalSize.x * 0.5, totalSize.x);
    }

    half4 resultColor = half4(0.0,0.0,0.0,0.0);

    if(uvTot.x > pixelSize.x || uvTot.y > pixelSize.y) {
        if(uvTot.x > pixelSize.x && uvTot.y < pixelSize.y) {
            // topRight
            const float2 spacingTexSize = float2(spacing.x, spacing.x);
            float relative = uvTot.y - (pixelSize.y - spacing.x) / 2;
            if(relative > 0.0 && relative < spacing.x) {
                float xPos = uvTot.x - pixelSize.x;
                float2 uv = fmod(float2(xPos, relative) / spacingTexSize + float2(1.0,1.0), float2(1.0,1.0));

                const float2 texUv = uvOrig + uvSize * uv;
                resultColor = texture0.sample(textureSampler, texUv);
                resultColor = half4(resultColor.rgb * resultColor.a, resultColor.a);
            }
        } else {
            uvTot.x = fmod(adjustedPixelPosition.x + spacing.x * 0.5, totalSize.x);
            if(uvTot.x > pixelSize.x && uvTot.y > pixelSize.y) {
                // bottom right
                const float2 uv = fmod((uvTot - pixelSize) / spacing + float2(1.0, 1.0), float2(1.0,1.0));
                const float2 texUv = uvOrig + uvSize * uv;
                resultColor = texture0.sample(textureSampler, texUv);
                resultColor = half4(resultColor.rgb * resultColor.a, resultColor.a);
            } else {
                // bottom left
                const float2 spacingTexSize = float2(spacing.y, spacing.y);
                const float relativeX = uvTot.x - (pixelSize.x - spacing.x) / 2.0;

                if(relativeX > 0.0 && relativeX < spacing.y) {
                    const float2 uv = fmod(float2(relativeX, uvTot.y - pixelSize.y) / spacingTexSize + float2(1.0, 1.0), float2(1.0,1.0));
                    const float2 texUv = uvOrig + uvSize * uv;
                    resultColor = texture0.sample(textureSampler, texUv);
                    resultColor = half4(resultColor.rgb * resultColor.a, resultColor.a);
                }
            }
        }
    } else {
        const float2 uv = fmod(uvTot / pixelSize + float2(1.0,1.0), float2(1.0,1.0));
        const float2 texUv = uvOrig + uvSize * uv;
        resultColor = texture0.sample(textureSampler, texUv);
        resultColor = half4(resultColor.rgb * resultColor.a, resultColor.a);
    }

    return resultColor;
}
