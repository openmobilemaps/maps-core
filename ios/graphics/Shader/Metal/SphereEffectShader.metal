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



fragment half4
sphereEffectFragmentShader(VertexOut in [[stage_in]],
                           constant const float4 *a [[buffer(0)]])
{
    float y1 = in.uv.x;
    float y2 = -in.uv.y;

    // extract elements of a (inverse projection matrix) for better readability
    float a11 = a[0][0];
    float a12 = a[1][0];
    float a13 = a[2][0];
    float a14 = a[3][0];
    float a21 = a[0][1];
    float a22 = a[1][1];
    float a23 = a[2][1];
    float a24 = a[3][1];
    float a31 = a[0][2];
    float a32 = a[1][2];
    float a33 = a[2][2];
    float a34 = a[3][2];
    float a41 = a[0][3];
    float a42 = a[1][3];
    float a43 = a[2][3];
    float a44 = a[3][3];

    float k1 = y1*a11 + y2*a12 + a14;
    float k2 = y1*a21 + y2*a22 + a24;
    float k3 = y1*a31 + y2*a32 + a34;
    float k4 = y1*a41 + y2*a42 + a44;

    float l1 = a13*a13 + a23*a23 + a33*a33;
    float l2 = 2*k1*a13 + 2*k2*a23 + 2*k3*a33;
    float l3 = k1*k1+k2*k2+k3*k3;

    float divident = l2*l2 - 4*l1*l3;
    float divisor = 4 * (l2 - (l3/k4)*a43 - (l1/a43)*k4);
    float Rsq = ((divident / k4) / a43) / divisor;
    float R = sqrt(Rsq);

    float L = clamp((R - 1.0) * 5.0, -1.0, 1.0);


    half4 white = half4(1.0,1.0,1.0,1.0);
    half4 blueOut = half4(44.0/255.0,166.0/255.0,1.0,1.0);
    half4 blueClear = half4(0.0,148.0/255.0,1.0,1.0);
    float opacity = 0.7;

    if (L > 0) {
      half t = clamp(L * 2.4, 0.0, 1.0);

        half4 c, c2;
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

        return ((1.0 - t) * alpha + t * alpha2) * ((1.0 - t) * c + t * c2) * opacity;
    }
    else {
        return half4(1.0, 1.0, 1.0, 1.0) * (1.0 + L * 10.0);
    }

    return white * opacity;
}

