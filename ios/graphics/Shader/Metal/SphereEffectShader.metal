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


struct Ellipse {
    float a;
    float b;
    float c;
    float d;
    float e;
    float f;
};


fragment float4
sphereEffectFragmentShader(VertexOut in [[stage_in]],
                           constant Ellipse &E [[buffer(0)]])
{
    float x = in.uv.x;
    float y = -in.uv.y;
    float L = E.a*x*x + E.b*x*y + E.c*y*y + E.d*x + E.e*y + E.f;
    if (L > 0) {
        float a = clamp(1.0 - pow(L, 0.3), 0.0, 1.0);
        float b = clamp(1.0 - L, 0.0, 1.0);
//        return float4(a, 0.0, 0.0, 1.0);
       return a * float4(1.0, 1.0, 1.0, 1) + (1.0 - a) * b * float4(0.1718, 0.62641, 1.0, 1);
    }
    discard_fragment();
    return float4(0.0, 0.0, 0.0, 0.0);
}

