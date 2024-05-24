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
    float4 white = float4(1.0,1.0,1.0,1.0);
    float4 blueOut = float4(44.0/255.0,166.0/255.0,1.0,1.0);
    float4 blueClear = float4(0.0,148.0/255.0,1.0,1.0);

    float x = in.uv.x;
    float y = -in.uv.y;
    float L = E.a*x*x + E.b*x*y + E.c*y*y + E.d*x + E.e*y + E.f;
    if (L > 0) {
        float t = clamp(L * 2.4, 0.0, 1.0);

        float4 c, c2;
        float alpha, alpha2;

        if(t < 0.5) {
            t = t / 0.5;
            c = white;
            alpha = 1.0;
            c2 = blueOut;
            alpha2 = 0.5;
        } else {
            t = (t - 0.5) / 0.5;
            c = blueOut;
            c2 = blueClear;
            alpha = 0.5;
            alpha2 = 0.0;
        }

        return ((1.0 - t) * alpha + t * alpha2) * ((1.0 - t) * c + t * c2);
    }

    discard_fragment();
    return float4(0.0,0.0,0.0,0.0);
  //  return discard_fragment();
}

