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

struct PolygonGroupVertexIn {
    float2 position [[attribute(0)]];
    float stylingIndex [[attribute(1)]];
};

struct PolygonGroupVertexOut {
    float4 position [[ position ]];
    float stylingIndex;
};


struct PolygonGroupStyling {
    float color[4];
    float opacity;
};


vertex PolygonGroupVertexOut
polygonGroupVertexShader(const PolygonGroupVertexIn vertexIn [[stage_in]],
                 constant float4x4 &mvpMatrix [[buffer(1)]])
{
    PolygonGroupVertexOut out {
        .position = mvpMatrix * float4(vertexIn.position.xy, 0.0, 1.0),
        .stylingIndex = vertexIn.stylingIndex,
    };

    return out;
}


fragment float4
polygonGroupFragmentShader(PolygonGroupVertexOut in [[stage_in]],
                        constant PolygonGroupStyling *styling [[buffer(1)]])
{
    PolygonGroupStyling s = styling[int(in.stylingIndex)];
    return float4(s.color[0], s.color[1], s.color[2], s.color[3]) * s.opacity;
}
