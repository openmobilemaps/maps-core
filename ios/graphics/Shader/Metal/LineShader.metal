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

/*
                                              ^
               position                       | widthNormal
              +-------------------------------+-------------------------------+
              |                                                               |
        <---  |  + lineA                                             lineB +  | -->
              |                                                               |
 lengthNormal +-------------------------------+-------------------------------+ lengthNormal
                                              |
                                              v  widthNormal
  */

struct LineVertexIn {
    float2 position [[attribute(0)]];
    float2 widthNormal [[attribute(1)]];
    float2 lineA [[attribute(2)]];
    float2 lineB [[attribute(3)]];
    float vertexIndex [[attribute(4)]];
    float lengthPrefix [[attribute(5)]];
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
    float lengthPrefix;
    float scalingFactor;
    float dashingSize;
    float scaledBlur;
};

struct LineStyling {
  float width; // 0
  float4 color; // 1 2 3 4
  float4 gapColor; // 5 6 7 8
  float widthAsPixels; // 9
  float opacity; // 10
  float blur; // 11
  float capType; // 12
  float numDashValues; // 13
  float dashArray[4]; // 14 15 16 17
  float offset; // 18
  float dotted; // 19
  float dottedSkew; // 20
};

/**

 var numDashValues: char

 var dashArray: [Float]
 */

vertex LineVertexOut
lineGroupVertexShader(const LineVertexIn vertexIn [[stage_in]],
                      constant float4x4 &vpMatrix [[buffer(1)]],
                      constant float &scalingFactor [[buffer(2)]],
                      constant float &dashingScalingFactor [[buffer(3)]],
                      constant float *styling [[buffer(4)]])
{
    int styleIndex = (int(vertexIn.stylingIndex) & 0xFF) * 21;

    // extend position in width direction and in length direction by width / 2.0
    float width = styling[styleIndex] / 2.0;
    float dashingSize = styling[styleIndex];

    float blur = styling[styleIndex + 11];

    if (styling[styleIndex + 9] > 0.0) {
        blur *= scalingFactor;
        width *= scalingFactor;
        dashingSize *= dashingScalingFactor;
    }

    float2 widthNormal = vertexIn.widthNormal;
    float2 lengthNormal = float2(widthNormal.y, -widthNormal.x);

    int index = int(vertexIn.vertexIndex);
    if(index == 0) {
      lengthNormal *= -1.0;
      widthNormal *= -1.0;
    } else if(index == 1) {
      lengthNormal *= -1.0;
    } else if(index == 2) {
      // all fine
    } else if(index == 3) {
      widthNormal *= -1.0;
    }

    float o = styling[styleIndex + 18] * scalingFactor;
    float4 offset = float4(vertexIn.widthNormal.x * o, vertexIn.widthNormal.y * o, 0.0, 0.0);

    float4 extendedPosition = float4(vertexIn.position.xy, 0.0, 1.0) +
                              float4((lengthNormal + widthNormal).xy, 0.0,0.0)
                              * float4(width, width, width, 0.0) + offset;

  int segmentType = int(vertexIn.stylingIndex) >> 8;// / 256.0;

    LineVertexOut out {
        .position = vpMatrix * extendedPosition,
        .uv = extendedPosition.xy,
        .lineA = extendedPosition.xy - (vertexIn.lineA + offset.xy),
        .lineB = (vertexIn.lineB + offset.xy) - (vertexIn.lineA + offset.xy),
        .stylingIndex = styleIndex,
        .width = width,
        .segmentType = segmentType,
        .lengthPrefix = vertexIn.lengthPrefix,
        .scalingFactor = scalingFactor,
        .dashingSize = dashingSize,
        .scaledBlur = blur
    };

    return out;
}

vertex LineVertexOut
unitSphereLineGroupVertexShader(const LineVertexIn vertexIn [[stage_in]],
                      constant float4x4 &vpMatrix [[buffer(1)]],
                      constant float &scalingFactor [[buffer(2)]],
                      constant float &dashingScalingFactor [[buffer(3)]],
                      constant float *styling [[buffer(4)]])
{
    int styleIndex = (int(vertexIn.stylingIndex) & 0xFF) * 21;

    // extend position in width direction and in length direction by width / 2.0
    float width = styling[styleIndex] / 2.0;
    float dashingSize = styling[styleIndex];

    float blur = styling[styleIndex + 11];

    if (styling[styleIndex + 9] > 0.0) {
        blur *= scalingFactor;
        width *= scalingFactor;
        dashingSize *= dashingScalingFactor;
    }

    float2 widthNormal = vertexIn.widthNormal;
    float2 lengthNormal = float2(widthNormal.y, -widthNormal.x);

    int index = int(vertexIn.vertexIndex);
    if(index == 0) {
      lengthNormal *= -1.0;
      widthNormal *= -1.0;
    } else if(index == 1) {
      lengthNormal *= -1.0;
    } else if(index == 2) {
      // all fine
    } else if(index == 3) {
      widthNormal *= -1.0;
    }

    float o = styling[styleIndex + 18] * scalingFactor;
    float4 offset = float4(vertexIn.widthNormal.x * o, vertexIn.widthNormal.y * o, 0.0, 0.0);

    float4 extendedPosition = float4(vertexIn.position.xy, 1.0, 1.0) +
                              float4((lengthNormal + widthNormal).xy, 1.0,0.0)
                              * float4(width, width, 0.0, 1.0) + offset;

    int segmentType = int(vertexIn.stylingIndex) >> 8;// / 256.0;

    extendedPosition.x /= extendedPosition.w;
    extendedPosition.y /= extendedPosition.w;
    extendedPosition.z /= extendedPosition.w;

    const float x = extendedPosition.z * sin(extendedPosition.y) * cos(extendedPosition.x);
    const float y = extendedPosition.z * cos(extendedPosition.y);
    const float z = -extendedPosition.z * sin(extendedPosition.y) * sin(extendedPosition.x);

    LineVertexOut out {
        .position = vpMatrix * float4(x,y,z, 1.0),
        .uv = extendedPosition.xy,
        .lineA = extendedPosition.xy - (vertexIn.lineA + offset.xy),
        .lineB = (vertexIn.lineB + offset.xy) - (vertexIn.lineA + offset.xy),
        .stylingIndex = styleIndex,
        .width = width,
        .segmentType = segmentType,
        .lengthPrefix = vertexIn.lengthPrefix,
        .scalingFactor = scalingFactor,
        .dashingSize = dashingSize,
        .scaledBlur = blur
    };

    return out;
}


fragment float4
lineGroupFragmentShader(LineVertexOut in [[stage_in]],
                        constant float *styling [[buffer(1)]])
{
  float lineLength = length(in.lineB);
  float t = dot(in.lineA, normalize(in.lineB) / lineLength);

  int capType = int(styling[in.stylingIndex + 12]);
  char segmentType = in.segmentType;

  float numDash = styling[in.stylingIndex + 13];

  float d;

  if (t < 0.0 || t > 1.0) {
    if (numDash > 0 && styling[in.stylingIndex + 14] < 1.0 && styling[in.stylingIndex + 14] > 0.0) {
      discard_fragment();
    }
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

  float opacity = styling[in.stylingIndex + 10];
  float colorA = styling[in.stylingIndex + 4];
  float colorAGap = styling[in.stylingIndex + 8];

  float a = colorA * opacity;
  float aGap = colorAGap * opacity;
    
  int dottedLine = int(styling[in.stylingIndex + 19]);
    
  if (in.scaledBlur > 0 && t > 0.0 && t < 1.0) {
    float nonBlurRange = (in.width - in.scaledBlur);
    if (d > nonBlurRange) {
      a *= clamp(1 - max(0.0, d - nonBlurRange) / (in.scaledBlur), 0.0, 1.0);
    }
  }

  if (dottedLine == 1) {
    float skew = styling[in.stylingIndex + 20];

    half factorToT = (in.width * 2) / lineLength * skew;
    half dashOffset = (in.width - skew * in.width) / lineLength;

    half dashTotalDotted =  2.0 * factorToT;
    half offset = half(in.lengthPrefix) / lineLength;
    half startOffsetSegmentDotted = fmod(offset, dashTotalDotted);
    half pos = t + startOffsetSegmentDotted;

    half intraDashPosDotted = fmod(pos, dashTotalDotted);
      if ((intraDashPosDotted > 1.0 * factorToT + dashOffset && intraDashPosDotted < dashTotalDotted - dashOffset) ||
                              (length(half2(min(abs(intraDashPosDotted - 0.5 * factorToT), 0.5 * factorToT + dashTotalDotted - intraDashPosDotted) / (0.5 * factorToT + dashOffset), d / in.width)) > 1.0)) {
          discard_fragment();
      }
  } else if(numDash > 0) {
    float dashArray[4] = { styling[in.stylingIndex + 14],
                           styling[in.stylingIndex + 15],
                           styling[in.stylingIndex + 16],
                           styling[in.stylingIndex + 17] };


    float factorToT = (in.width * 2) / lineLength;
    float dashTotal = dashArray[3] * factorToT;
    float startOffsetSegment = fmod(in.lengthPrefix / lineLength, dashTotal);
    float intraDashPos = fmod(t + startOffsetSegment, dashTotal);

    if ((intraDashPos > dashArray[0] * factorToT && intraDashPos < dashArray[1] * factorToT) ||
        (intraDashPos > dashArray[2] * factorToT && intraDashPos < dashArray[3] * factorToT)) {

      if(aGap == 0) {
        discard_fragment();
      }

      return float4(styling[in.stylingIndex + 5],
                    styling[in.stylingIndex + 6],
                    styling[in.stylingIndex + 7],
                    1.0) * aGap;
    }
  }

  if(a == 0) {
    discard_fragment();
  }

  return float4(styling[in.stylingIndex + 1],
                styling[in.stylingIndex + 2],
                styling[in.stylingIndex + 3],
                1.0) * a;
}
