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
    float2 uv;
    float3 lineA;
    float3 lineB;
    int stylingIndex;
    float width;
    int segmentType;
    float scalingFactor;
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
                      constant float &dashingScalingFactor [[buffer(3)]],
                      constant half *styling [[buffer(4)]],
                      constant float4 &originOffset [[buffer(5)]],
                      constant float4 &tileOrigin [[buffer(6)]])
{
    int styleIndex = (int(vertexIn.stylingIndex) & 0xFF) * 8;
    constant SimpleLineStyling *style = (constant SimpleLineStyling *)(styling + styleIndex);

    // extend position in width direction and in length direction by width / 2.0
    float width = style->width / 2.0;

    if (style->width > 0.0) {
        width *= scalingFactor ;
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

    const float4 extendedPosition = float4(position + originOffset.xyz, 1.0) +
                              float4((lengthNormal + widthNormal).xyz,0.0)
                              * float4(width, width, width, 0.0);

    const int segmentType = int(vertexIn.stylingIndex) >> 8;// / 256.0;

    SimpleLineVertexOut out {
        .position = vpMatrix * extendedPosition,
        .uv = extendedPosition.xy,
        .lineA = extendedPosition.xyz - ((vertexIn.lineA + originOffset.xyz)),
        .lineB = ((vertexIn.lineB + originOffset.xyz)) - ((vertexIn.lineA + originOffset.xyz)),
        .stylingIndex = styleIndex,
        .width = width,
        .segmentType = segmentType,
        .scalingFactor = scalingFactor,
    };

    return out;
}

vertex SimpleLineVertexOut
simpleLineGroupVertexShader(const LineVertexIn vertexIn [[stage_in]],
                      constant float4x4 &vpMatrix [[buffer(1)]],
                      constant float &scalingFactor [[buffer(2)]],
                      constant float &dashingScalingFactor [[buffer(3)]],
                      constant half *styling [[buffer(4)]],
                      constant float4 &originOffset [[buffer(5)]],
                      constant float4 &tileOrigin [[buffer(6)]])
{
    int styleIndex = (int(vertexIn.stylingIndex) & 0xFF) * 8;
    constant SimpleLineStyling *style = (constant SimpleLineStyling *)(styling + styleIndex);
  
    // extend position in width direction and in length direction by width / 2.0
    float width = style->width / 2.0;

    if (style->width > 0.0) {
        width *= scalingFactor ;
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

    const float4 extendedPosition = float4(position.xy + originOffset.xy, 0.0, 1.0) +
                              float4((lengthNormal + widthNormal).xy, 0.0,0.0)
                              * float4(width, width, width, 0.0);

    const int segmentType = int(vertexIn.stylingIndex) >> 8;// / 256.0;

    SimpleLineVertexOut out {
        .position = vpMatrix * extendedPosition,
        .uv = extendedPosition.xy,
        .lineA = extendedPosition.xyz - ((lineA + originOffset.xyz)),
        .lineB = ((lineB + originOffset.xyz)) - ((lineA + originOffset.xyz)),
        .stylingIndex = styleIndex,
        .width = width,
        .segmentType = segmentType,
        .scalingFactor = scalingFactor,
    };

    return out;
}


fragment half4
simpleLineGroupFragmentShader(SimpleLineVertexOut in [[stage_in]],
                        constant half *styling [[buffer(1)]])
{
  constant SimpleLineStyling *style = (constant SimpleLineStyling *)(styling + in.stylingIndex);
  const float lineLength = length(in.lineB);
  const float t = dot(in.lineA, normalize(in.lineB) / lineLength);

  const int capType = int(style->capType);
  const char segmentType = in.segmentType;


  float d;

  if (t < 0.0 || t > 1.0) {
    if (segmentType == 0 || capType == 1 || (segmentType == 2 && t < 0.0) || (segmentType == 1 && t > 1.0)) {
      d = min(length(in.lineA), length(in.lineA - in.lineB));
    } else if (capType == 2) {
      const float dLen = t < 0.0 ? -t * lineLength : (t - 1.0) * lineLength;
      const float3 intersectPt = t * float3(in.lineB);
      const float dOrth = abs(length(float3(in.lineA) - intersectPt));
      d = max(dLen, dOrth);
    } else {
      d = 0;
      discard_fragment();
    }
  } else {
    const float3 intersectPt = t * float3(in.lineB);
    d = abs(length(float3(in.lineA) - intersectPt));
  }

  if (d > in.width) {
    discard_fragment();
  }

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
                      constant float &dashingScalingFactor [[buffer(3)]],
                      constant half *styling [[buffer(4)]],
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
                      constant half *styling [[buffer(4)]],
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
                        constant half *styling [[buffer(1)]],
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
      const float dLen = t < 0.0 ? -t * lineLength : (t - 1.0) * lineLength;
      const float3 intersectPt = t * float3(in.lineB);
      const float dOrth = abs(length(float3(in.lineA) - intersectPt));
      d = max(dLen, dOrth);
    } else {
      d = 0;
      discard_fragment();
    }
  } else {
    const float3 intersectPt = t * float3(in.lineB);
    d = abs(length(float3(in.lineA) - intersectPt));
  }

  if (d > in.width) {
    discard_fragment();
  }

  const float opacity = style->opacity;
  const float colorA = style->color.a;
  const float colorAGap = style->gapColor.a;

  float a = colorA * opacity;
  const float aGap = colorAGap * opacity;

  const int dottedLine = int(style->dotted);

  if (in.scaledBlur > 0 && t > 0.0 && t < 1.0) {
    const float nonBlurRange = (in.width - in.scaledBlur);
    if (d > nonBlurRange) {
      a *= clamp(1 - max(0.0, d - nonBlurRange) / (in.scaledBlur), 0.0, 1.0);
    }
  }

  if (dottedLine == 1) {
    const float skew = style->dottedSkew;

    const float factorToT = (in.width * 2) / lineLength * skew;
    const float dashOffset = (in.width - skew * in.width) / lineLength;

    const float dashTotalDotted =  2.0 * factorToT;
    const float offset = float(in.lengthPrefix) / lineLength;
    const float startOffsetSegmentDotted = fmod(offset, dashTotalDotted);
    const float pos = t + startOffsetSegmentDotted;

    const float intraDashPosDotted = fmod(pos, dashTotalDotted);
      if ((intraDashPosDotted > 1.0 * factorToT + dashOffset && intraDashPosDotted < dashTotalDotted - dashOffset) ||
                              (length(half2(min(abs(intraDashPosDotted - 0.5 * factorToT), 0.5 * factorToT + dashTotalDotted - intraDashPosDotted) / (0.5 * factorToT + dashOffset), d / in.width)) > 1.0)) {
          discard_fragment();
      }
  } else if(numDash > 0) {
    const float factorToT = (in.width * 2) / lineLength;
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
                  float wG = relG / (style->dash_fade * 0.5);
                  return half4(half3(style->gapColor.rgb), 1.0) * aGap * wG + half4(half3(style->color.rgb), 1.0) * a * (1.0 - wG);
              }
//              else if (1.0 - relG < (style->dash_fade * 0.5)) {
//                  half wG = (1.0 - relG) / (style->dash_fade * 0.5);
//                  return half4(half3(style->gapColor.rgb), 1.0) * aGap * wG + half4(half3(style->color.rgb), 1.0) * a * (1.0 - wG);
//              }
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
                  float wG = (1.0 - relG) / style->dash_fade;
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
