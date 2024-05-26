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
//    return float4(divisor, 0, 0, 1.0);


//    // Compute A1, A2, A3, A4
//    float A1 = a[0][0] * y_1 + a[0][1] * y_2 + a[0][3];
//    float A2 = a[1][0] * y_1 + a[1][1] * y_2 + a[1][3];
//    float A3 = a[2][0] * y_1 + a[2][1] * y_2 + a[2][3];
//    float A4 = a[3][0] * y_1 + a[3][1] * y_2 + a[3][3];
//
//    // Compute M and N
//    float M = a[0][2] * A1 + a[1][2] * A2 + a[2][2] * A3;
//    float N = a[3][2] * A4;
//
//    // Compute the squares of aij
//    float a13_sq = a[0][2] * a[0][2];
//    float a23_sq = a[1][2] * a[1][2];
//    float a33_sq = a[2][2] * a[2][2];
//    float a43_sq = a[3][2] * a[3][2];
//
//    // Compute the numerator and the denominator
//    float num = (a13_sq + a23_sq + a33_sq) * (A1 * A1 + A2 * A2 + A3 * A3) - M * M;
//    float denom = (a13_sq + a23_sq + a33_sq) * (A4 * A4) + a43_sq * (A1 * A1 + A2 * A2 + A3 * A3) - 2 * M * N;
//
//
//    // Compute R
//    float R = sqrt(num / denom);

//    float A1 = a[0][1]*a[0][1] + a[1][2]*a[1][2] + a[2][2]*a[2][2];
//    float B1 = y1 * (a[0][0]*a[0][2] + a[1][0]*a[1][2] + a[2][0]*a[2][2])
//             + y2 * (a[0][1]*a[0][2] + a[1][1]*a[1][2] + a[2][1]*a[2][2])
//             +      (a[0][3]*a[0][2] + a[1][3]*a[1][2] + a[2][3]*a[2][2]);
//    float B2 = y1*a[3][0]*a[3][2] + y2*a[3][1]*a[3][2] + a[3][3]*a[3][2];
//    float C1 = y1*y1 * (a[0][0]*a[0][0] + a[1][0]*a[1][0] + a[2][0]*a[2][0])
//             + y1*y2 * (a[0][0]*a[0][1] + a[1][0]*a[1][1] + a[2][0]*a[2][1])
//             +    y1 * (a[0][0]*a[0][3] + a[1][0]*a[1][3] + a[2][0]*a[2][3])
//             + y2*y2 * (a[0][1]*a[0][1] + a[1][1]*a[1][1] + a[2][1]*a[2][1])
//             +    y2 * (a[0][1]*a[0][3] + a[1][1]*a[1][3] + a[2][1]*a[2][3])
//            +         (a[0][3]*a[0][3] + a[1][3]*a[1][3] + a[2][3]*a[2][3]);
//    float C2 = y1*y1*a[3][0]*a[3][0] + y1*y2*a[3][0]*a[3][1] + y1*a[3][0]*a[3][3]
//             + y2*y2*a[3][1]*a[3][1] + y2*a[3][1]*a[3][3] + a[3][3]*a[3][3];
//
//    float D = -2*B1*B2 + 4*A1*C2 + 4*a[3][2]*a[3][2]*C1;
//    float E = 2*(B2*B2 - 4*a[3][2]*a[3][2]*C2);
//    float F = D*D - 4*E*(B1*B1 - 4*A1*C1);
//
//    // Solve for R^2
//    float R_squared_1 = (D + sqrt(F)) / E;
//    float R_squared_2 = (D - sqrt(F)) / E;

    // Since R should be positive, we take the positive root of R^2
//    float R = sqrt(fmax(R_squared_1, R_squared_2));

//    float D = -2*B1*B2 + 4*A1*C2 + 4*a[3][2]*a[3][2]*C1;
//    float E = 2*(B2*B2 - 4*a[3][2]*a[3][2]*C2);
//    float F = D*D - 4*E*(B1*B1 - 4*A1*C1);
//    float R = (D + sqrt(F)) / E;

//    float D = 2*B1*B2 - 4*A1*C1 - 4*a[3][2]*a[3][2]*C1;
//    float E = 2*(B2*B2 - 4*a[3][2]*a[3][2]*C2);
//    float F = D*D - 2*E*(B1*B1 - 4*A1*C1);
//    float R = (D + sqrt(F)) / E;
//    float z2 = pow(2*B1*B2 - 4*A1*C2 - 4*a[3][2]*a[3][2]*C1, 2.0) - 4*(B2*B2 - 4*a[3][2]*a[3][2]*C2)*(B1*B1 - 4*A1*C1);
//    float z3 = 2*(B2*B2 - 4*a[3][2]*a[3][2]*C2);

//    float z1 = -2*B1*B2 + 4*A1*C2 + 4*a[3][2]*a[3][2]*C1;
//    float z2 = pow(z1, 2.0) - 4*(B2*B2 - 4*a[3][2]*a[3][2]*C2)*(B1*B1 - 4*A1*C1);
//
//    float zA = (z1 - sqrt(z2)) / z3;
//    float zB = (z1 + sqrt(z2)) / z3;
//
//    float L1 = sqrt(zA) - 1.0;
//    float L2 = sqrt(zB) - 1.0;


    float L = R - 1.0;


    float4 white = float4(1.0,1.0,1.0,1.0);
    float4 blueOut = float4(44.0/255.0,166.0/255.0,1.0,1.0);
    float4 blueClear = float4(0.0,148.0/255.0,1.0,1.0);

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

    return white;
//    return float4(0.0,0.0,0.0,0.0);
}

