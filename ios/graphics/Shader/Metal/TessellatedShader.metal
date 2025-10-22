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


/// Calculate a value by bilinearly interpolating among four control points.
/// The four values c00, c01, c10, and c11 represent, respectively, the
/// upper-left, upper-right, lower-left, and lower-right points of a quad
/// that is parameterized by a normalized space that runs from (0, 0)
/// in the upper left to (1, 1) in the lower right (similar to Metal's texture
/// space). The vector `uv` contains the influence of the points along the
/// x and y axes.
template <typename T>
T bilerp(T c00, T c01, T c10, T c11, float2 uv) {
    T c0 = mix(c00, c01, T(uv[0]));
    T c1 = mix(c10, c11, T(uv[0]));
    return mix(c0, c1, T(uv[1]));
}

// TODO: is3d
float4 transform(float4 coordinate, float4 origin) {
    if (true/*is3d*/) {
        float x = 1.0 * sin(coordinate.y) * cos(coordinate.x) - origin.x;
        float y = 1.0 * cos(coordinate.y) - origin.y;
        float z = -1.0 * sin(coordinate.y) * sin(coordinate.x) - origin.z;
        return float4(x, y, z, 1);
    } else {
        float x = coordinate.x - origin.x;
        float y = coordinate.y - origin.y;
        return float4(x, y, 0, 1);
    }
}

[[patch(quad, 4)]] vertex VertexOut
tessellationVertexShader(const patch_control_point<Vertex3DTextureIn> controlPoints [[stage_in]],
                         const float2 positionInPatch [[position_in_patch]],
                         constant float4x4 &vpMatrix [[buffer(1)]],
                         constant float4x4 &mMatrix [[buffer(2)]],
                         constant float4 &originOffset [[buffer(3)]],
                         constant float4 &origin [[buffer(4)]])
{
    Vertex3DTextureIn vA = controlPoints[0];
    Vertex3DTextureIn vB = controlPoints[1];
    Vertex3DTextureIn vC = controlPoints[2];
    Vertex3DTextureIn vD = controlPoints[3];
    
    float4 vertexPosition = bilerp(vA.position, vB.position, vC.position, vD.position, positionInPatch);
    float2 vertexUV = bilerp(vA.uv, vB.uv, vC.uv, vD.uv, positionInPatch);
    
    float4 position = transform(vertexPosition, origin) - originOffset;
    
    VertexOut out {
        .position = vpMatrix * ((mMatrix * position) + originOffset),
        .uv = vertexUV
    };

    return out;
}
