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



fragment float4
sphereEffectFragmentShader(VertexOut in [[stage_in]],
                           constant const float4 *Q [[buffer(0)]])
{
    float y1 = in.uv.x;
    float y2 = -in.uv.y;

    float L = pow(y1*Q[0][2] + y2*Q[1][2] + Q[2][3], 2.0)
    - Q[2][2] * (y1*y1*Q[0][0] + 2*y1*y2*Q[0][1] + 2*y1*Q[0][3] + y2*y2*Q[1][1] + 2*y2*Q[1][3] + Q[3][3]);
    L = -L;
    if (L > 0) {
        float a = clamp(1.0 - pow(L, 0.3), 0.0, 1.0);
        float b = clamp(1.0 - L, 0.0, 1.0);
//        return float4(a, 0.0, 0.0, 1.0);
       return a * float4(1.0, 1.0, 1.0, 1) + (1.0 - a) * b * float4(0.1718, 0.62641, 1.0, 1);
    }
    discard_fragment();
    return float4(0.0, 0.0, 0.0, 0.0);
}

