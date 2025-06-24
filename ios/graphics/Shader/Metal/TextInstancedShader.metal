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

struct TextInstancedVertexOut {
  float4 position [[ position ]];
  float2 uv;
  float4 texureCoordinates;
  uint16_t styleIndex;
  float alpha;
};

vertex TextInstancedVertexOut
unitSphereTextInstancedVertexShader(const VertexIn vertexIn [[stage_in]],
                          constant float4x4 &vpMatrix [[buffer(1)]],
                          constant float4x4 &mMatrix [[buffer(2)]],
                          constant float2 *positions [[buffer(3)]],
                          constant float2 *scales [[buffer(4)]],
                          constant float *rotations [[buffer(5)]],
                          constant float4 *texureCoordinates [[buffer(6)]],
                          constant uint16_t *styleIndices [[buffer(7)]],
                          constant packed_float3 *referencePositions [[buffer(8)]],
                          constant float4 &originOffset [[buffer(9)]],
                          constant float4 &origin [[buffer(10)]],
                          constant float &aspectRatio [[buffer(11)]],
                          constant float *alphas [[buffer(12)]],
                          uint instanceId [[instance_id]])
{
    float alpha = alphas[instanceId];
    const float3 referencePosition = referencePositions[instanceId];
    const float2 offset = positions[instanceId];
    const float2 scale = scales[instanceId];
    const float rotation = rotations[instanceId];

    const float angle = rotation * M_PI_F / 180.0;

    float4 newVertex = float4(referencePosition + originOffset.xyz, 1.0);

    float4 earthCenter = vpMatrix * float4(0 - origin.x,
                                           0 - origin.y,
                                           0 - origin.z, 1.0);
    float4 screenPosition = vpMatrix * newVertex;

    earthCenter /= earthCenter.w;
    screenPosition /= screenPosition.w;

    auto diffCenter = screenPosition - earthCenter;

    float isVisible = float(alpha > 0.0);  // 0 if alpha == 0, 1 otherwise
    float isInFront = float(diffCenter.z <= 0); // 0 if behind globe, 1 otherwise

    const float mask = isVisible * isInFront;

    const float sinAngle = sin(angle) * mask;
    const float cosAngle = cos(angle) * mask;

    const float2 p = (vertexIn.position.xy);

    // apply scale, then rotation and aspect ratio correction
    const float2 pRot = float2(
        p.x * scale.x * cosAngle - p.y * scale.y * sinAngle,
        (p.x * scale.x * sinAngle + p.y * scale.y * cosAngle) * aspectRatio
    );

    const float4 offscreenPlaceholder = float4(-3.0, -3.0, -3.0, -3.0);
    float4 screenPositionWithOffset = float4(screenPosition.xy + offset.xy + pRot, 0.0, 1.0);
    screenPositionWithOffset /= screenPosition.w;
    const float4 finalPosition = mix(offscreenPlaceholder, screenPositionWithOffset, mask);

    TextInstancedVertexOut out {
      .position = finalPosition,
      .uv = vertexIn.uv,
      .texureCoordinates = texureCoordinates[instanceId],
      .styleIndex = styleIndices[instanceId],
      .alpha = alpha
    };

    return out;
}

struct TextInstanceStyle {
  float packedColor;
  float packedHaloColor;
    float haloWidth;
    float haloBlur;
} __attribute__((__packed__));


inline float4 unpackColor(float packedColor) {
    uint bits = as_type<uint>(packedColor);
    return float4(
        ((bits >> 24) & 0xFF) * (1.0 / 255.0),
        ((bits >> 16) & 0xFF) * (1.0 / 255.0),
        ((bits >>  8) & 0xFF) * (1.0 / 255.0),
        ((bits      ) & 0xFF) * (1.0 / 255.0)
    );
}

fragment half4
unitSphereTextInstancedFragmentShader(TextInstancedVertexOut in [[stage_in]],
                                      constant TextInstanceStyle *styles [[buffer(1)]],
                                      constant bool &isHalo [[buffer(2)]],
                       texture2d<half> texture0 [[ texture(0)]],
                       sampler textureSampler [[sampler(0)]])
{
    constant TextInstanceStyle *style = (constant TextInstanceStyle *)(styles + in.styleIndex);

    float4 color = unpackColor(style->packedColor);
    float4 haloColor = unpackColor(style->packedHaloColor);

    if ((color.a == 0 && !isHalo) || (haloColor.a == 0.0 && isHalo) || in.alpha == 0.0) {
        discard_fragment();
    }

    const float2 uv = in.texureCoordinates.xy + in.texureCoordinates.zw * float2(in.uv.x, in.uv.y);
    const half4 dist = texture0.sample(textureSampler, uv);

    const float median = max(min(dist.r, dist.g), min(max(dist.r, dist.g), dist.b)) / dist.a;
    const float w = fwidth(median);

    const float fillStart = 0.5 - w;
    const float fillEnd = 0.5 + w;

    const float innerFallOff = smoothstep(fillStart, fillEnd, median);

    half edgeAlpha = 0.0;

    if (isHalo) {
        float halfHaloBlur = 0.5 * style->haloBlur;

        if (style->haloWidth == 0.0 && halfHaloBlur == 0.0) {
            discard_fragment();
        }

        const float start = max(0.0, fillStart - (style->haloWidth + halfHaloBlur));
        const float end = max(0.0, fillStart - max(0.0, style->haloWidth - halfHaloBlur));

        const float sideSwitch = step(median, end);
        const float outerFallOff = smoothstep(start, end, median);

        // Combination of blurred outer falloff and inverse inner fill falloff
        edgeAlpha = (sideSwitch * outerFallOff + (1.0 - sideSwitch) * (1.0 - innerFallOff)) * haloColor.a;

        return half4(half3(haloColor.rgb), 1.0) * edgeAlpha;

    } else {
        edgeAlpha = innerFallOff * color.a;

        return half4(half3(color.rgb), 1.0) * edgeAlpha;
    }
}

vertex TextInstancedVertexOut
textInstancedVertexShader(const VertexIn vertexIn [[stage_in]],
                          constant float4x4 &vpMatrix [[buffer(1)]],
                          constant float4x4 &mMatrix [[buffer(2)]],
                          constant float2 *positions [[buffer(3)]],
                          constant float2 *scales [[buffer(4)]],
                          constant float *rotations [[buffer(5)]],
                          constant float4 *texureCoordinates [[buffer(6)]],
                          constant uint16_t *styleIndices [[buffer(7)]],
                          constant float2 *referencePositions [[buffer(8)]],
                          constant float4 &originOffset [[buffer(9)]],
                          constant float *alphas [[buffer(12)]],
                          uint instanceId [[instance_id]])
{
    float alpha = alphas[instanceId];
    float mask = float(alpha > 0.0);  // 0 if alpha == 0, 1 otherwise

    float2 pos = (positions[instanceId] + originOffset.xy) * mask;
    float2 scale = scales[instanceId] * mask;
    float angle = rotations[instanceId] * mask * M_PI_F / 180.0;

    float sinAngle = sin(angle) * mask;
    float cosAngle = cos(angle) * mask;

    // Apply scale and rotation directly to vertex
    float2 local = vertexIn.position.xy;
    float2 rotated = float2(
        local.x * scale.x * cosAngle - local.y * scale.y * sinAngle,
        local.x * scale.x * sinAngle + local.y * scale.y * cosAngle
    );

    float4 worldPosition = float4(pos + rotated, 0.0, 1.0);
    float4 clipPosition = mix(float4(-3.0, -3.0, -3.0, -3.0), vpMatrix * worldPosition, mask);

    return TextInstancedVertexOut {
        .position = clipPosition,
        .uv = vertexIn.uv,
        .texureCoordinates = texureCoordinates[instanceId],
        .styleIndex = styleIndices[instanceId],
        .alpha = alpha
    };
}


fragment half4
textInstancedFragmentShader(TextInstancedVertexOut in [[stage_in]],
                            constant TextInstanceStyle *styles [[buffer(1)]],
                            constant bool &isHalo [[buffer(2)]],
                       texture2d<half> texture0 [[ texture(0)]],
                       sampler textureSampler [[sampler(0)]])
{
    constant TextInstanceStyle *style = (constant TextInstanceStyle *)(styles + in.styleIndex);

    float4 color = unpackColor(style->packedColor);
    float4 haloColor = unpackColor(style->packedHaloColor);

    if ((isHalo && haloColor.a == 0.0) || (!isHalo && color.a == 0) || in.alpha == 0.0) {
        discard_fragment();
    }

    const float2 uv = in.texureCoordinates.xy + in.texureCoordinates.zw * float2(in.uv.x, 1 - in.uv.y);
    const half4 dist = texture0.sample(textureSampler, uv);

    const float median = max(min(dist.r, dist.g), min(max(dist.r, dist.g), dist.b)) / dist.a;
    const float w = fwidth(median);

    const float fillStart = 0.5 - w;
    const float fillEnd = 0.5 + w;

    const float innerFallOff = smoothstep(fillStart, fillEnd, median);

    if (isHalo) {
        float halfHaloBlur = 0.5 * style->haloBlur;

        if (style->haloWidth == 0.0 && halfHaloBlur == 0.0) {
            discard_fragment();
        }

        const float start = max(0.0, fillStart - (style->haloWidth + halfHaloBlur));
        const float end = max(0.0, fillStart - max(0.0, style->haloWidth - halfHaloBlur));

        const float sideSwitch = step(median, end);
        const float outerFallOff = smoothstep(start, end, median);

        // Combination of blurred outer falloff and inverse inner fill falloff
        const float edgeAlpha = (sideSwitch * outerFallOff + (1.0 - sideSwitch) * (1.0 - innerFallOff)) * haloColor.a;

        return half4(half3(haloColor.rgb), 1.0) * edgeAlpha;
    } else {
        const float edgeAlpha = innerFallOff * color.a;

        return half4(half3(color.rgb), 1.0) * edgeAlpha;
    }
}
