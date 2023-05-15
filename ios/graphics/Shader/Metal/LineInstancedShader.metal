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

struct InstancedVertexOut {
  float4 position [[ position ]];
  int style;
  bool keep;
};

struct LineStyling {
  float width;
  float4 color;
  float4 gapColor;
  char widthAsPixels;
  float opacity;
  float blur;
  char capType;
  char numDashValues;
  float dashArray[4];
  float offset;
};

vertex InstancedVertexOut
lineGroupInstancedVertexShader(const VertexIn vertexIn [[stage_in]],
                               constant float4x4 &mvpMatrix [[buffer(1)]],
                               constant float4 *positions [[buffer(2)]],
                               constant float &scalingFactor [[buffer(3)]],
                               constant LineStyling *styling [[buffer(4)]],
                               ushort instanceId [[instance_id]]) {

    float4 pA = positions[instanceId];
    float4 pB = positions[instanceId+1];
    float4 pC = positions[instanceId+2];
    float4 pD = positions[instanceId+3];

    float4 p0 = pA;
    float4 p1 = pB;
    float4 p2 = pC;

    float2 pos = vertexIn.position;
    if (pos.x == 1.0) {
        p0 = pD;
        p1 = pC;
        p2 = pB;
        pos = float2(1.0 - pos.x, -pos.y);
    }

    // Find the normal vector.
    float2 tangent = normalize(normalize(p2.xy - p1.xy) + normalize(p1.xy- p0.xy));
    float2 normal = float2(-tangent.y, tangent.x);

      // Find the vector perpendicular to p0 -> p1.
    float2 p01 = p1.xy - p0.xy;
    float2 p21 = p1.xy - p2.xy;
    float2 p01Norm = normalize(float2(-p01.y, p01.x));

    // Determine the bend direction.
    float sigma = sign(dot(p01 + p21, normal));

    int style = int(p0.a);

    float width = styling[style].width / 2.0;

    if (styling[style].widthAsPixels > 0.0) {
        width *= scalingFactor;
    }

    if (sign(pos.y) == -sigma) {
        // This is an intersecting vertex. Adjust the position so that there's no overlap.
        float2 point = 0.5 * normal * -sigma * width / dot(normal, p01Norm);
        float4 pos = float4(p1.xy + point, 0, 1);

        InstancedVertexOut out {
            .position = mvpMatrix * pos,
            .style = int(p0.a),
            .keep = fabs(p0.z - p1.z) < 0.5 && fabs(p1.z - p2.z) < 0.5
        };

        return out;
    } else {
        // This is a non-intersecting vertex. Treat it normally.
        float2 xBasis = p2.xy - p1.xy;
        float2 yBasis = normalize(float2(-xBasis.y, xBasis.x));
        xBasis = normalize(xBasis);
        float2 point = p1.xy + xBasis * pos.x + yBasis * width * pos.y;

        InstancedVertexOut out {
            .position = mvpMatrix * float4(point, 0.0, 1.0),
            .style = int(p0.a),
            .keep = fabs(p0.z - p1.z) < 0.5 && fabs(p1.z - p2.z) < 0.5
        };

        return out;
    }
}

fragment float4
lineGroupInstancedFragmentShader(InstancedVertexOut in [[stage_in]],
                                 constant LineStyling *styling [[buffer(1)]])
{
    if(in.keep == false) {
        discard_fragment();
    }

    LineStyling style = styling[in.style];


    return float4(style.color.r, style.color.g, style.color.b, 1.0);

}
