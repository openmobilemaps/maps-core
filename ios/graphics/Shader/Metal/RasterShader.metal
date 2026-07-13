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

struct RasterStyle {
    float opacity;
    float brightnessMin;
    float brightnessMax;
    float contrast;
    float saturation;
    float gamma;
    float brightnessShift;
};

struct LookupStyle {
    float mode;
    float u;
    float v;
    float width;
    float height;
    float y;
};

static LookupStyle makeLookupStyle(constant RasterStyle &style) {
    // RasterStyle fields are reused in lookup mode to keep the shared style buffer unchanged.
    return LookupStyle{
        style.brightnessMin,
        style.brightnessMax,
        style.contrast,
        style.saturation,
        style.gamma,
        style.brightnessShift
    };
}

static float lookupX(float value, LookupStyle style) {
    return style.u + clamp(value, 0.0, 1.0) * style.width;
}

static float lookupY(float normalizedZoom, LookupStyle style) {
    return style.v + clamp(normalizedZoom, 0.0, 1.0) * style.height;
}

static float dualLookupY(float normalizedZoom, bool lowerHalf, LookupStyle style, texture2d<half> lookupTexture) {
    float fullRows = style.height * float(lookupTexture.get_height()) + 1.0;
    float halfRows = max(floor(fullRows * 0.5), 1.0);
    float halfRange = max(halfRows - 1.0, 0.0) / float(lookupTexture.get_height());
    float halfStart = style.v + (lowerHalf ? halfRows / float(lookupTexture.get_height()) : 0.0);
    return halfStart + clamp(normalizedZoom, 0.0, 1.0) * halfRange;
}

static float quadLookupX(float value, bool rightHalf, LookupStyle style, texture2d<half> lookupTexture) {
    float fullColumns = style.width * float(lookupTexture.get_width()) + 1.0;
    float halfColumns = max(floor(fullColumns * 0.5), 1.0);
    float halfRange = max(halfColumns - 1.0, 0.0) / float(lookupTexture.get_width());
    float halfStart = style.u + (rightHalf ? halfColumns / float(lookupTexture.get_width()) : 0.0);
    return halfStart + clamp(value, 0.0, 1.0) * halfRange;
}

static half4 applyLookupStyle(half4 color, constant RasterStyle &rasterStyle, texture2d<half> lookupTexture, sampler textureSampler) {
    LookupStyle style = makeLookupStyle(rasterStyle);
    half4 lookupColor;
    if (style.mode <= -2.5) {
        // Quad mode: red selects the horizontal value in both left/right halves,
        // green blends top/bottom rows, blue blends left/right halves.
        half blendY = half(clamp(float(color.g), 0.0, 1.0));
        half blendX = half(clamp(float(color.b), 0.0, 1.0));
        half4 primaryColor = lookupTexture.sample(textureSampler, float2(quadLookupX(float(color.r), false, style, lookupTexture), dualLookupY(style.y, false, style, lookupTexture)));
        half4 secondaryColor = lookupTexture.sample(textureSampler, float2(quadLookupX(float(color.r), false, style, lookupTexture), dualLookupY(style.y, true, style, lookupTexture)));
        half4 tertiaryColor = lookupTexture.sample(textureSampler, float2(quadLookupX(float(color.r), true, style, lookupTexture), dualLookupY(style.y, false, style, lookupTexture)));
        half4 quaternaryColor = lookupTexture.sample(textureSampler, float2(quadLookupX(float(color.r), true, style, lookupTexture), dualLookupY(style.y, true, style, lookupTexture)));
        lookupColor = mix(mix(primaryColor, secondaryColor, blendY), mix(tertiaryColor, quaternaryColor, blendY), blendX);
    } else if (style.mode <= -1.5) {
        // Dual mode: red selects the value in both halves, green blends top/bottom rows.
        half4 primaryColor = lookupTexture.sample(textureSampler, float2(lookupX(float(color.r), style), dualLookupY(style.y, false, style, lookupTexture)));
        half4 secondaryColor = lookupTexture.sample(textureSampler, float2(lookupX(float(color.r), style), dualLookupY(style.y, true, style, lookupTexture)));
        lookupColor = mix(primaryColor, secondaryColor, half(clamp(float(color.g), 0.0, 1.0)));
    } else {
        // Mono mode: red selects the lookup value, y is the clamped zoom position.
        lookupColor = lookupTexture.sample(textureSampler, float2(lookupX(float(color.r), style), lookupY(style.y, style)));
    }
    return half4(lookupColor.rgb * color.a, color.a) * rasterStyle.opacity;
}

static half4 applyNormalStyle(half4 color, constant RasterStyle &style) {
    half3 rgb = color.rgb;

    rgb = clamp(rgb + style.brightnessShift, half3(0.0), half3(1.0));

    float average = (color.r + color.g + color.b) / 3.0;

    rgb += (average - rgb) * style.saturation;

    rgb = clamp((rgb - 0.5) * style.contrast + 0.5, half3(0.0), half3(1.0));

    half3 brightnessMin = half3(style.brightnessMin, style.brightnessMin, style.brightnessMin);
    half3 brightnessMax = half3(style.brightnessMax, style.brightnessMax, style.brightnessMax);

    float gamma = style.gamma;
    rgb = pow(rgb, (1.0 / (gamma)));
    rgb = mix(brightnessMin, brightnessMax, min(rgb / color.a, half3(1.0)));
    return half4(rgb * color.a, color.a) * style.opacity;
}

fragment half4
rasterFragmentShader(VertexOut in [[stage_in]],
                   constant RasterStyle *styling [[buffer(1)]],
                   texture2d<half> texture0 [[ texture(0)]],
                   texture2d<half> lookupTexture [[ texture(1)]],
                   sampler textureSampler [[sampler(0)]])
{
    half4 color = texture0.sample(textureSampler, in.uv);

    if (color.a == 0.0 || styling[0].opacity == 0.0) {
        return half4(0.0);
    }

    if (styling[0].brightnessMin < 0.0) {
        return applyLookupStyle(color, styling[0], lookupTexture, textureSampler);
    }

    return applyNormalStyle(color, styling[0]);
}
