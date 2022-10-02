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

struct LineVertexIn {
    float2 position [[attribute(0)]];
    float2 widthNormal [[attribute(1)]];
    float2 lenghtNormal [[attribute(2)]];
    float2 lineA [[attribute(3)]];
    float2 lineB [[attribute(4)]];
    float lenghtPrefix [[attribute(5)]];
    float stylingIndex [[attribute(6)]];
};

struct LineVertexOut {
    float4 position [[ position ]];
    float2 uv;
    float2 lineA;
    float2 lineB;
    int stylingIndex;
    float width;
    int segmentType;
    float lenghtPrefix;
    float scalingFactor;
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
};


/**

 var numDashValues: char

 var dashArray: [Float]
 */

vertex LineVertexOut
lineGroupVertexShader(const LineVertexIn vertexIn [[stage_in]],
                      constant float4x4 &mvpMatrix [[buffer(1)]],
                      constant float &scalingFactor [[buffer(2)]],
                      constant LineStyling *styling [[buffer(3)]])
{
    int style = int(vertexIn.stylingIndex) & 0xFF;

    // extend position in width direction and in lenght direction by width / 2.0
    float width = styling[style].width / 2.0;
    if (styling[style].widthAsPixels > 0.0) {
        width *= scalingFactor;
    }

    float4 extendedPosition = float4(vertexIn.position.xy, 0.0, 1.0) + float4((vertexIn.lenghtNormal + vertexIn.widthNormal).xy, 0.0,0.0)
            * float4(width, width, width, 0.0);

    int segmentType = int(vertexIn.stylingIndex) >> 8;

    LineVertexOut out {
        .position = mvpMatrix * extendedPosition,
        .uv = extendedPosition.xy,
        .lineA = (extendedPosition.xy - vertexIn.lineA),
        .lineB = vertexIn.lineB - vertexIn.lineA,
        .stylingIndex = style,
        .width = width,
        .segmentType = segmentType,
        .lenghtPrefix = vertexIn.lenghtPrefix,
        .scalingFactor = scalingFactor
    };

    return out;
}


fragment float4
lineGroupFragmentShader(LineVertexOut in [[stage_in]],
                        constant LineStyling *styling [[buffer(1)]])
{
  float lineLength = length(in.lineB);
  float t = dot(in.lineA, normalize(in.lineB) / lineLength);
  float d;
  LineStyling style = styling[in.stylingIndex];

  float a = style.color.a * style.opacity;
  float aGap = style.gapColor.a * style.opacity;

  char capType = style.capType;
  char segmentType = in.segmentType;
  if (t < 0.0 || t > 1.0) {
    if (segmentType == 0 || capType == 1 || (segmentType == 2 && t < 0.0) || (segmentType == 1 && t > 1.0)) {
      d = min(length(in.lineA), length(in.lineA - in.lineB));
    } else if (capType == 2) {
      float dLen = t < 0.0 ? -t * lineLength : (t - 1.0) * lineLength;
      float2 intersectPt = t * in.lineB;
      float dOrth = abs(length(in.lineA - intersectPt));
      d = max(dLen, dOrth);
    } else {
      d = 0;
      discard_fragment();
    }
  } else {
    float2 intersectPt = t * in.lineB;
    d = abs(length(in.lineA - intersectPt));
  }

  if (d > in.width) {
    discard_fragment();
  }


  if (style.widthAsPixels && style.blur != 0) {
    float blurRange = (in.width - (style.blur * in.scalingFactor * 0.5));
    float2 intersectPt = t * in.lineB;
    float blurD = abs(length(in.lineA - intersectPt));
    if (blurD > blurRange) {
      a *= clamp(1 - (blurD - blurRange) / (style.blur * in.scalingFactor * 0.5) ,0.0, 1.0);
    }
  }


  if (style.numDashValues > 0) {
    float factorToT = (in.width * 2) / lineLength;
    float dashTotal = style.dashArray[3] * factorToT;
    float startOffsetSegment = fmod(in.lenghtPrefix / lineLength, dashTotal);
    float intraDashPos = fmod(t + startOffsetSegment, dashTotal);

    if ((intraDashPos > style.dashArray[0] * factorToT && intraDashPos < style.dashArray[1] * factorToT) ||
        (intraDashPos > style.dashArray[2] * factorToT && intraDashPos < style.dashArray[3] * factorToT)) {

      if(aGap == 0) {
        discard_fragment();
      }

      return float4(style.gapColor.r, style.gapColor.g, style.gapColor.b, 1.0) * style.opacity;
    }
  }


  if(a == 0) {
    discard_fragment();
  }

  return float4(style.color.r, style.color.g, style.color.b, 1.0) * a;
}
