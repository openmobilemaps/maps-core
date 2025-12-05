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
    float2 extrude [[attribute(1)]];
    float lineSide [[attribute(2)]];
    float lengthPrefix [[attribute(3)]];
    float lengthCorrection [[attribute(4)]];
    float stylingIndex [[attribute(5)]];
};

struct LineVertexUnitSphereIn {
    float3 position [[attribute(0)]];
    float3 extrude [[attribute(1)]];
    float lineSide [[attribute(2)]];
    float lengthPrefix [[attribute(3)]];
    float lengthCorrection [[attribute(4)]];
    float stylingIndex [[attribute(5)]];
};

struct LineVertexOut {
    float4 position [[ position ]];
    int stylingIndex;
    float lineSide;
    float lengthPrefix;
};

struct LineStyling {
  half width; // 0
  packed_half4 color; // 1 2 3 4
  packed_half4 gapColor; // 5 6 7 8
  half widthAsPixels; // 9
  half opacity; // 10
  half blur; // 11
  half capType; // 12
  half numDashValues; // 13
  packed_half4 dashArray; // 14 15 16 17
  half dash_fade; // 18
  half dash_animation_speed; // 19
  half offset; // 20
  half dotted; // 21
  half dottedSkew; // 22
} __attribute__((__packed__));


struct SimpleLineVertexOut {
    float4 position [[ position ]];
    int stylingIndex;
};

struct SimpleLineStyling {
    half width; // 0
    packed_half4 color; // 1 2 3 4
    half widthAsPixels; // 5
    half opacity; // 6
    half capType; // 7
} __attribute__((__packed__));


/**

 var numDashValues: char

 var dashArray: [Float]
 */

vertex SimpleLineVertexOut
unitSphereSimpleLineGroupVertexShader(const LineVertexUnitSphereIn vertexIn [[stage_in]],
                      constant float4x4 &vpMatrix [[buffer(1)]],
                      constant float &scalingFactor [[buffer(2)]],
                      constant half *styling [[buffer(3)]],
                      constant float4 &originOffset [[buffer(4)]],
                      constant float4 &tileOrigin [[buffer(5)]])
{
    int styleIndex = (int(vertexIn.stylingIndex) & 0xFF) * 8;
    constant SimpleLineStyling *style = (constant SimpleLineStyling *)(styling + styleIndex);

    // extend position in extrude direction by width / 2.0
    float width = style->width / 2.0 * scalingFactor;

    const float4 extendedPosition = float4(vertexIn.position + vertexIn.extrude * width, 1.0) + originOffset;

    SimpleLineVertexOut out {
        .position = vpMatrix * extendedPosition,
        .stylingIndex = styleIndex,
    };

    return out;
}

vertex SimpleLineVertexOut
simpleLineGroupVertexShader(const LineVertexIn vertexIn [[stage_in]],
                      constant float4x4 &vpMatrix [[buffer(1)]],
                      constant float &scalingFactor [[buffer(2)]],
                      constant half *styling [[buffer(3)]],
                      constant float4 &originOffset [[buffer(4)]],
                      constant float4 &tileOrigin [[buffer(5)]])
{
    int styleIndex = (int(vertexIn.stylingIndex) & 0xFF) * 8;
    constant SimpleLineStyling *style = (constant SimpleLineStyling *)(styling + styleIndex);
  
    // extend position in extrude direction by width / 2.0
    float width = style->width / 2.0 * scalingFactor;

    const float4 extendedPosition = float4(vertexIn.position + vertexIn.extrude * width, 0.0, 1.0) + originOffset;

    SimpleLineVertexOut out {
        .position = vpMatrix * extendedPosition,
        .stylingIndex = styleIndex,
    };

    return out;
}


fragment half4
simpleLineGroupFragmentShader(SimpleLineVertexOut in [[stage_in]],
                        constant half *styling [[buffer(1)]])
{
  constant SimpleLineStyling *style = (constant SimpleLineStyling *)(styling + in.stylingIndex);

  const half opacity = style->opacity;
  const half colorA = style->color.a;

  half a = colorA * opacity;

  if(a == 0) {
    discard_fragment();
  }

  return half4(half3(style->color.rgb), 1.0) * a;
}



vertex LineVertexOut
unitSphereLineGroupVertexShader(const LineVertexUnitSphereIn vertexIn [[stage_in]],
                      constant float4x4 &vpMatrix [[buffer(1)]],
                      constant float &scalingFactor [[buffer(2)]],
                      constant half *styling [[buffer(3)]],
                      constant float4 &originOffset [[buffer(4)]],
                      constant float4 &tileOrigin [[buffer(5)]])
{
    int styleIndex = (int(vertexIn.stylingIndex) & 0xFF) * 23;
    constant LineStyling *style = (constant LineStyling *)(styling + styleIndex);

    // extend position in extrude direction by width / 2.0
    float width = style->width / 2.0 * scalingFactor;

    const float4 extendedPosition = float4(vertexIn.position + vertexIn.extrude * width, 1.0)  + originOffset;

    LineVertexOut out {
        .position = vpMatrix * extendedPosition,
        .stylingIndex = styleIndex,
        .lineSide = vertexIn.lineSide,
        .lengthPrefix = vertexIn.lengthPrefix + vertexIn.lengthCorrection * width,
    };

    return out;
}

vertex LineVertexOut
lineGroupVertexShader(const LineVertexIn vertexIn [[stage_in]],
                      constant float4x4 &vpMatrix [[buffer(1)]],
                      constant float &scalingFactor [[buffer(2)]],
                      constant half *styling [[buffer(3)]],
                      constant float4 &originOffset [[buffer(4)]],
                      constant float4 &tileOrigin [[buffer(5)]])
{
    int styleIndex = (int(vertexIn.stylingIndex) & 0xFF) * 23;
    constant LineStyling *style = (constant LineStyling *)(styling + styleIndex);

    // extend position in extrude direction by width / 2.0
    float width = style->width / 2.0 * scalingFactor;

    const float4 extendedPosition = float4(vertexIn.position + vertexIn.extrude * width, 0.0, 1.0) + originOffset;

    LineVertexOut out {
        .position = vpMatrix * extendedPosition,
        .stylingIndex = styleIndex,
        .lineSide = vertexIn.lineSide,
        .lengthPrefix = vertexIn.lengthPrefix + vertexIn.lengthCorrection * width,
    };

    return out;
}


fragment half4
lineGroupFragmentShader(LineVertexOut in [[stage_in]],
                        constant half *styling [[buffer(1)]],
                        constant float &time [[buffer(2)]],
                        constant float &scalingFactor [[buffer(3)]],
                        constant float &dashingScalingFactor [[buffer(4)]])
{
  constant LineStyling *style = (constant LineStyling *)(styling + in.stylingIndex);

  const float opacity = style->opacity;

  if (opacity == 0.0) {
      discard_fragment();
  }


  float a = style->color.a * opacity;
  float aGap = style->gapColor.a * opacity;

  const int dottedLine = int(style->dotted);

  const half numDash = style->numDashValues;

  if (style->blur > 0) {
      const float scaledWidth = style->width * scalingFactor;
      const float halfScaledWidth = scaledWidth / 2.0;
      const float blur = (style->blur) * scalingFactor; // screen units
      const float lineEdgeDistance = (1.0 - abs(in.lineSide)) * halfScaledWidth; // screen units
      const float blurAlpha = clamp(lineEdgeDistance / blur, 0.0, 1.0);

      if(blurAlpha == 0.0) {
        discard_fragment();
      }

      a *= blurAlpha;
      aGap *= blurAlpha;
  }

  half4 mainColor = half4(half3(style->color.rgb), 1.0) * a;
  half4 gapColor  = half4(half3(style->gapColor.rgb), 1.0) * aGap;

  if (dottedLine == 1) {
    const float skew = style->dottedSkew;

    const float scaledWidth = style->width * dashingScalingFactor;

    const float cycleLength = style->width * dashingScalingFactor * skew;

    const float timeOffset = time * style->dash_animation_speed * scaledWidth;
      const float skewOffset = (1.0 - skew) * style->width * dashingScalingFactor * 0.5;
    const float positionInCycle = fmod(in.lengthPrefix + timeOffset + skewOffset, 2.0 * cycleLength) / cycleLength * skew;

    const float scalingRatio =  dashingScalingFactor / scalingFactor;
    float2 pos = float2((positionInCycle * 2.0 - 1.0) * scalingRatio, in.lineSide);

    if(dot(pos, pos) >= 1.0) {
      discard_fragment();
    }
  } else if(numDash > 0) {

    const float scaledWidth = style->width * dashingScalingFactor;
    const float timeOffset = time * style->dash_animation_speed * scaledWidth;
    const float intraDashPos = fmod(in.lengthPrefix + timeOffset, (float)style->dashArray.w * scaledWidth);

    // here float has to be used for accuracy
    float4 dashArray = float4(style->dashArray) * scaledWidth;
    float dxt = dashArray.x;
    float dyt = dashArray.y;
    float dzt = dashArray.z;
    float dwt = dashArray.w;

    if (style->dash_fade == 0) {
        if ((intraDashPos > dxt && intraDashPos < dyt) || (intraDashPos > dzt && intraDashPos < dwt)) {
          // Simple case without fade
          return gapColor;
        } else {
          return mainColor;
        }
      }
      else {

          if (intraDashPos > dxt && intraDashPos < dyt) {

              half relG = (intraDashPos - dxt) / (dyt - dxt);
              if (relG < (style->dash_fade * 0.5)) {
                  float wG = relG / (style->dash_fade * 0.5);
                  return gapColor * wG + mainColor * (1.0 - wG);
              } else {
                return gapColor;
              }
          }

          if (intraDashPos > dzt && intraDashPos < dwt) {

              half relG = (intraDashPos - dzt) / (dwt - dzt);
              if (relG < style->dash_fade) {
                  half wG = relG / style->dash_fade;
                  return gapColor * wG + mainColor * (1.0 - wG);
              }
              else if (1.0 - relG < style->dash_fade) {
                  float wG = (1.0 - relG) / style->dash_fade;
                  return gapColor * wG + mainColor * (1.0 - wG);
              }
              else {
                return gapColor;
              }

          }
      }
  }

  if(a == 0) {
    discard_fragment();
  }

  return mainColor;
}
