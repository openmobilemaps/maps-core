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

struct IcosahedronVertexIn {
    float2 position [[attribute(0)]];
    float value [[attribute(1)]];
};

struct IcosahedronVertexOut {
    float4 position [[ position ]];
    float value [[attribute(1)]];
};

vertex IcosahedronVertexOut
icosahedronVertexShader(const IcosahedronVertexIn vertexIn [[stage_in]],
                 constant float4x4 &mvpMatrix [[buffer(1)]])
{
    const float phi = (vertexIn.position.y - 180.0) * M_PI_F / 180.0;
    const float th = (vertexIn.position.x - 90.0) * M_PI_F / 180.0;

    float4 adjPos = float4(1.0 * sin(th) * cos(phi),
                       1.0 * cos(th),
                       -1.0 * sin(th) * sin(phi),
                       1.0);
    
    IcosahedronVertexOut out {
        .position = mvpMatrix * adjPos,
        .value = vertexIn.value
    };

    return out;
}

fragment float4
icosahedronFragmentShader(IcosahedronVertexOut in [[stage_in]],
                          constant float4 &color [[buffer(1)]])
{
    if (in.value < 0.01) {
        discard_fragment();
        return float4(0,0,0,0);
    }
    float alpha = 0.5;
    return mix(float4(float3(0.149, 0.0, 0.941/*0.29, 0.56, 0.27*/) * 0.8, 0.8), float4(float3(0.51, 1.0, 0.36/*0.99, 1.0, 0.36*/) * 0.8, 0.8), in.value);

//    // Temperature in Kelvin
//    float temperature = in.value;
//
//    // Normalize temperature to a 0.0 - 1.0 range based on expected min/max values
//    // Adjust minTemp and maxTemp based on your application's expected temperature range
//    float minTemp = 253.15; // -20°C, adjust as needed
//    float maxTemp = 323.15; // 50°C, adjust as needed
//    float normalizedTemp = (temperature - minTemp) / (maxTemp - minTemp);
//
//    // Clamp value between 0.0 and 1.0 to ensure it stays within the gradient bounds
//    normalizedTemp = clamp(normalizedTemp, 0.0, 1.0);
//
//    // Define colors for cold (blue), medium (green), and hot (red)
//    float4 coldColor = float4(0, 0, 1, 1); // Blue
//    float4 mediumColor = float4(0, 1, 0, 1); // Green
//    float4 hotColor = float4(1, 0, 0, 1); // Red
//
//    // Interpolate between colors based on the normalized temperature
//    float4 colorRes;
//    if (normalizedTemp < 0.5)
//    {
//        // Transition from cold to medium
//        float t = normalizedTemp * 2.0; // Scale to 0.0 - 1.0 range
//        colorRes = mix(coldColor, mediumColor, t);
//    }
//    else
//    {
//        // Transition from medium to hot
//        float t = (normalizedTemp - 0.5) * 2.0; // Scale to 0.0 - 1.0 range
//        colorRes = mix(mediumColor, hotColor, t);
//    }
//
//    // Return the interpolated color with the original alpha value
//    return float4(colorRes.rgb * 0.5, 0.5);
}
