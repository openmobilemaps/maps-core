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

struct VertexIn {
    float2 position [[attribute(0)]];
    float2 uv [[attribute(1)]];
};

struct Vertex3DTextureIn {
    float4 position [[attribute(0)]];
    float2 uv [[attribute(1)]];
};

// NOTE: float3 also take up 4 * sizeOf(float) bytes
// therefore we always use float4 for better alignment and to reduce errors when filling the buffer
struct Vertex4FIn {
    float4 position [[attribute(0)]];
};

struct VertexOut {
    float4 position [[ position ]];
    float2 uv;
};

struct float4x4x2 {
  float4x4 matrices[2];
};
