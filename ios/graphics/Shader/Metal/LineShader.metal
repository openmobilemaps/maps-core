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
    float2 lineA [[attribute(0)]];
    float2 lineB [[attribute(1)]];
    float vertexIndex [[attribute(2)]];
    float lengthPrefix [[attribute(3)]];
    float stylingIndex [[attribute(4)]];
};

struct LineVertexUnitSphereIn {
    float3 lineA [[attribute(0)]];
    float3 lineB [[attribute(1)]];
    float vertexIndex [[attribute(2)]];
    float lengthPrefix [[attribute(3)]];
    float stylingIndex [[attribute(4)]];
};

struct LineVertexOut {
    float4 position [[ position ]];
    float2 uv;
    float3 lineA;
    float3 lineB;
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
  packed_float4 color; // 1 2 3 4
  packed_float4 gapColor; // 5 6 7 8
  float widthAsPixels; // 9
  float opacity; // 10
  float blur; // 11
  float capType; // 12
  float numDashValues; // 13
  packed_float4 dashArray; // 14 15 16 17
  float dash_fade; // 18
  float dash_animation_speed; // 19
  float offset; // 20
  float dotted; // 21
  float dottedSkew; // 22
} __attribute__((__packed__));

/**

 var numDashValues: char

 var dashArray: [Float]
 */

vertex LineVertexOut
unitSpherelineGroupVertexShader(const LineVertexUnitSphereIn vertexIn [[stage_in]],
                      constant float4x4 &vpMatrix [[buffer(1)]],
                      constant float &scalingFactor [[buffer(2)]],
                      constant float &dashingScalingFactor [[buffer(3)]],
                      constant float *styling [[buffer(4)]],
                      constant float4 &originOffset [[buffer(5)]],
                      constant float4 &tileOrigin [[buffer(6)]])
{
    int styleIndex = (int(vertexIn.stylingIndex) & 0xFF) * 23;
    constant LineStyling *style = (constant LineStyling *)(styling + styleIndex);

    // extend position in width direction and in length direction by width / 2.0
    float width = style->width / 2.0;
    float dashingSize = style->width;

    float blur = style->blur;

    if (style->width > 0.0) {
        blur *= scalingFactor;
        width *= scalingFactor ;
        dashingSize *= dashingScalingFactor;
    }

    int index = int(vertexIn.vertexIndex);

    const float3 lineA = vertexIn.lineA;
    const float3 lineB = vertexIn.lineB;

    float3 position = lineB;
    if(index == 0) {
      position = lineA;
    } else if(index == 1) {
      position = lineA;
    }

    float3 lengthNormal = normalize(lineB - lineA);
    const float3 radialVector = normalize(position + tileOrigin.xyz);
    const float3 widthNormalIn = normalize(cross(radialVector, lengthNormal));

    float3 widthNormal = widthNormalIn;

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

    const float o = style->offset * scalingFactor;
    const float4 lineOffset = float4(widthNormalIn.x * o, widthNormalIn.y * o, widthNormalIn.z * o, 0.0);
    const float4 extendedPosition = float4(position + originOffset.xyz, 1.0) +
                              float4((lengthNormal + widthNormal).xyz,0.0)
                              * float4(width, width, width, 0.0) + lineOffset;

    const int segmentType = int(vertexIn.stylingIndex) >> 8;// / 256.0;

    LineVertexOut out {
        .position = vpMatrix * extendedPosition,
        .uv = extendedPosition.xy,
        .lineA = extendedPosition.xyz - ((vertexIn.lineA + originOffset.xyz) + lineOffset.xyz),
        .lineB = ((vertexIn.lineB + originOffset.xyz) + lineOffset.xyz) - ((vertexIn.lineA + originOffset.xyz) + lineOffset.xyz),
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
lineGroupVertexShader(const LineVertexIn vertexIn [[stage_in]],
                      constant float4x4 &vpMatrix [[buffer(1)]],
                      constant float &scalingFactor [[buffer(2)]],
                      constant float &dashingScalingFactor [[buffer(3)]],
                      constant float *styling [[buffer(4)]],
                      constant float4 &originOffset [[buffer(5)]],
                      constant float4 &tileOrigin [[buffer(6)]])
{
    int styleIndex = (int(vertexIn.stylingIndex) & 0xFF) * 23;
    constant LineStyling *style = (constant LineStyling *)(styling + styleIndex);

    // extend position in width direction and in length direction by width / 2.0
    float width = style->width / 2.0;
    float dashingSize = style->width;

    float blur = style->blur;

    if (style->width > 0.0) {
        blur *= scalingFactor;
        width *= scalingFactor ;
        dashingSize *= dashingScalingFactor;
    }

    const float3 lineA = float3(vertexIn.lineA, 0);
    const float3 lineB = float3(vertexIn.lineB, 0);

    int index = int(vertexIn.vertexIndex);
    float3 position = lineB;
    if(index == 0) {
      position = lineA;
    } else if(index == 1) {
      position = lineA;
    }

    float3 lengthNormal = normalize(lineB - lineA);
    const float3 radialVector = float3(0,0,1);
    const float3 widthNormalIn = normalize(cross(radialVector, lengthNormal));

    float3 widthNormal = widthNormalIn;

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

    const float o = style->offset * scalingFactor;
    const float4 lineOffset = float4(widthNormalIn.x * o, widthNormalIn.y * o, 0, 0.0);
    const float4 extendedPosition = float4(position.xy + originOffset.xy, 0.0, 1.0) +
                              float4((lengthNormal + widthNormal).xy, 0.0,0.0)
                              * float4(width, width, width, 0.0) + lineOffset;

    const int segmentType = int(vertexIn.stylingIndex) >> 8;// / 256.0;

    LineVertexOut out {
        .position = vpMatrix * extendedPosition,
        .uv = extendedPosition.xy,
        .lineA = extendedPosition.xyz - ((lineA + originOffset.xyz) + lineOffset.xyz),
        .lineB = ((lineB + originOffset.xyz) + lineOffset.xyz) - ((lineA + originOffset.xyz) + lineOffset.xyz),
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


fragment half4
lineGroupFragmentShader(LineVertexOut in [[stage_in]],
                        constant float *styling [[buffer(1)]],
                        constant float &time [[buffer(2)]])
{
  constant LineStyling *style = (constant LineStyling *)(styling + in.stylingIndex);
  const float lineLength = length(in.lineB);
  const float t = dot(in.lineA, normalize(in.lineB) / lineLength);

  const int capType = int(style->capType);
  const char segmentType = in.segmentType;

  const half numDash = style->numDashValues;

  float d;

  if (t < 0.0 || t > 1.0) {
    if (numDash > 0 && style->dashArray.x < 1.0 && style->dashArray.x > 0.0) {
      discard_fragment();
    }
    if (segmentType == 0 || capType == 1 || (segmentType == 2 && t < 0.0) || (segmentType == 1 && t > 1.0)) {
      d = min(length(in.lineA), length(in.lineA - in.lineB));
    } else if (capType == 2) {
      const half dLen = t < 0.0 ? -t * lineLength : (t - 1.0) * lineLength;
      const half3 intersectPt = t * half3(in.lineB);
      const half dOrth = abs(length(half3(in.lineA) - intersectPt));
      d = max(dLen, dOrth);
    } else {
      d = 0;
      discard_fragment();
    }
  } else {
    const half3 intersectPt = t * half3(in.lineB);
    d = abs(length(half3(in.lineA) - intersectPt));
  }

  if (d > in.width) {
    discard_fragment();
  }

  const half opacity = style->opacity;
  const half colorA = style->color.a;
  const half colorAGap = style->gapColor.a;

  half a = colorA * opacity;
  const half aGap = colorAGap * opacity;

  const int dottedLine = int(style->dotted);

  if (in.scaledBlur > 0 && t > 0.0 && t < 1.0) {
    const half nonBlurRange = (in.width - in.scaledBlur);
    if (d > nonBlurRange) {
      a *= clamp(1 - max(0.0, d - nonBlurRange) / (in.scaledBlur), 0.0, 1.0);
    }
  }

  if (dottedLine == 1) {
    const half skew = style->dottedSkew;

    const half factorToT = (in.width * 2) / lineLength * skew;
    const half dashOffset = (in.width - skew * in.width) / lineLength;

    const half dashTotalDotted =  2.0 * factorToT;
    const half offset = half(in.lengthPrefix) / lineLength;
    const half startOffsetSegmentDotted = fmod(offset, dashTotalDotted);
    const half pos = t + startOffsetSegmentDotted;

    const half intraDashPosDotted = fmod(pos, dashTotalDotted);
      if ((intraDashPosDotted > 1.0 * factorToT + dashOffset && intraDashPosDotted < dashTotalDotted - dashOffset) ||
                              (length(half2(min(abs(intraDashPosDotted - 0.5 * factorToT), 0.5 * factorToT + dashTotalDotted - intraDashPosDotted) / (0.5 * factorToT + dashOffset), d / in.width)) > 1.0)) {
          discard_fragment();
      }
  } else if(numDash > 0) {
    const half factorToT = (in.width * 2) / lineLength;
    const float dashTotal = style->dashArray.w * factorToT;
    const float startOffsetSegment = fmod(in.lengthPrefix / lineLength, dashTotal);

      const float timeOffset = time * style->dash_animation_speed * factorToT;

    const float intraDashPos = fmod(t + startOffsetSegment + timeOffset, dashTotal);

      float dxt = style->dashArray.x * factorToT;
      float dyt = style->dashArray.y * factorToT;
      float dzt = style->dashArray.z * factorToT;
      float dwt = style->dashArray.w * factorToT;
      if (style->dash_fade == 0 &&
          ((intraDashPos > dxt && intraDashPos < dyt) || (intraDashPos > dzt && intraDashPos < dwt))) {
          // Simple case without fade
          return half4(half3(style->gapColor.rgb), 1.0) * aGap;
      }
      else {
          if (intraDashPos > dxt && intraDashPos < dyt) {

              half relG = (intraDashPos - dxt) / (dyt - dxt);
              if (relG < (style->dash_fade * 0.5)) {
                  half wG = relG / (style->dash_fade * 0.5);
                  return half4(half3(style->gapColor.rgb), 1.0) * aGap * wG + half4(half3(style->color.rgb), 1.0) * a * (1.0 - wG);
              }
              else if (1.0 - relG < (style->dash_fade * 0.5)) {
                  half wG = (1.0 - relG) / (style->dash_fade * 0.5);
                  return half4(half3(style->gapColor.rgb), 1.0) * aGap * wG + half4(half3(style->color.rgb), 1.0) * a * (1.0 - wG);
              }
              else {
                  return half4(half3(style->gapColor.rgb), 1.0) * aGap;
              }
          }

          if (intraDashPos > dzt && intraDashPos < dwt) {

              half relG = (intraDashPos - dzt) / (dwt - dzt);
              if (relG < style->dash_fade) {
                  half wG = relG / style->dash_fade;
                  return half4(half3(style->gapColor.rgb), 1.0) * aGap * wG + half4(half3(style->color.rgb), 1.0) * a * (1.0 - wG);
              }
              else if (1.0 - relG < style->dash_fade) {
                  half wG = (1.0 - relG) / style->dash_fade;
                  return half4(half3(style->gapColor.rgb), 1.0) * aGap * wG + half4(half3(style->color.rgb), 1.0) * a * (1.0 - wG);
              }
              else {
                  return half4(half3(style->gapColor.rgb), 1.0) * aGap;
              }

          }
      }
  }

  if(a == 0) {
    discard_fragment();
  }

  return half4(half3(style->color.rgb), 1.0) * a;
}
