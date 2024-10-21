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

struct Vertex3FIn {
    float3 position [[attribute(0)]];
};

struct Vertex4FIn {
    float4 position [[attribute(0)]];
};

struct VertexOut {
    float4 position [[ position ]];
    float2 uv;
};
