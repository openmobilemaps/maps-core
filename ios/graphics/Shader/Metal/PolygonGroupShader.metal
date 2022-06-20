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
    int stylingIndex [[attribute(1)]];
};

struct PolygonGroupVertexOut {
    float4 position [[ position ]];
    int stylingIndex;
};


struct PolygonGroupStyling {
    float4 color;
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
    PolygonGroupStyling style = styling[in.stylingIndex];
    float a = style.color.a * style.opacity;
    if (a == 0) {
        discard_fragment();
    }
    return style.color * style.opacity;
}
