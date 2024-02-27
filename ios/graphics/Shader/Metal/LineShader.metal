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
    int segmentType; // (0 inner, 1 start, 2 end, 3 single segment
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
    float dashCapType; // 19
};

/**
 
 var numDashValues: char
 
 var dashArray: [Float]
 */

vertex LineVertexOut
lineGroupVertexShader(const LineVertexIn vertexIn [[stage_in]],
                      constant float4x4 &mvpMatrix [[buffer(1)]],
                      constant float &scalingFactor [[buffer(2)]],
                      constant float &dashingScalingFactor [[buffer(3)]],
                      constant float *styling [[buffer(4)]])
{
    int styleIndex = (int(vertexIn.stylingIndex) & 0xFF) * 20;
    
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
        .position = mvpMatrix * extendedPosition,
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
    float lineLength = length(in.lineB); // segment length in meter
    float t = dot(in.lineA, normalize(in.lineB) / lineLength); //
    
    int capType = int(styling[in.stylingIndex + 12]);
    char segmentType = in.segmentType;
    
    float numDash = styling[in.stylingIndex + 13];
    
    float d;
    
    if (t < 0.0 || t > 1.0) {
//        if (numDash > 0) {
//            discard_fragment();
//        }
        if ((segmentType == 0 /* Inner */ || capType == 1 /* Round */ || (segmentType == 2 /* End */ && t < 0.0) || (segmentType == 1 /* Start */ && t > 1.0))) {
            d = min(length(in.lineA), length(in.lineA - in.lineB));
        } else if (capType == 2 /* Butt */) {
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
    
    if(in.scaledBlur > 0 && t > 0.0 && t < 1.0) {
        float nonBlurRange = (in.width - in.scaledBlur);
        if (d > nonBlurRange) {
            a *= clamp(1 - max(0.0, d - nonBlurRange) / (in.scaledBlur) ,0.0, 1.0);
        }
    }

    if(numDash > 0) {
      
        
        float dashArray[4] = { styling[in.stylingIndex + 14],
            styling[in.stylingIndex + 15],
            styling[in.stylingIndex + 16],
            styling[in.stylingIndex + 17] };
        
        float factorToT = (in.width * 2) / lineLength; // linienbreite / meter
        
        float dashTotal = dashArray[3] /* LInienbreite*/ * factorToT; // Länge des ganzen pattern in points / meter
        float startOffsetSegment = fmod(half(in.lengthPrefix) / lineLength, dashTotal); // in t
        float intraDashPos = fmod(t + startOffsetSegment, dashTotal); // in t

        if ((intraDashPos > dashArray[0] * factorToT && intraDashPos < dashArray[1] * factorToT) ||
            (intraDashPos > dashArray[2] * factorToT && intraDashPos < dashArray[3] * factorToT)) {
            
            if(aGap == 0) {
                discard_fragment();
            }
            
            return float4(styling[in.stylingIndex + 5],
                          styling[in.stylingIndex + 6],
                          styling[in.stylingIndex + 7],
                          1.0) * aGap;
        } else {

            // inside segment
            int dashedCapType = int(styling[in.stylingIndex + 19]);
            
            

            // Rounded dash caps
            if (true) { //dashedCapType == 0) {
                float s = in.width / lineLength; //
          
                float R = s; // Radius in t
                float mid = R - (intraDashPos) + t * lineLength;
                float start = in.lengthPrefix;
                float end = in.lengthPrefix + lineLength;

                if (intraDashPos < s) {
                    // Linke seite
                    float dy = d / in.width;
                    float dx = 1 - intraDashPos / s;
                    
                    
                    
                    float a = t + half(in.lengthPrefix) - s;
//                    if (mid > start) {
//                      discard_fragment();
//
//                        return float4(1,
//                                      1,
//                                      0,
//                                      0.3) * a;
//                    }
                    if (dy * dy + dx * dx >  0.99 && dy * dy + dx * dx <  1.01) {
                        return float4(0, 0,0,1);

                    } else if (dy * dy + dx * dx > 1) {
//                        return float4(1,
//                                      0,
//                                      1,
//                                      0.8) * a;
                        discard_fragment();
                    } else {
                        return float4(styling[in.stylingIndex + 1],
                                      styling[in.stylingIndex + 2],
                                      styling[in.stylingIndex + 3],
                                      1.0) * a;
                    }
                    
                } else if (intraDashPos > dashArray[0] * factorToT - s && intraDashPos < dashArray[0] * factorToT) {
                   //discard_fragment();
//                    if (t + s > 1.0) {
//                        discard_fragment();
//                    }
                    
//                    if (mid < end) {
//                        return float4(1,
//                                      0,
//                                      1,
//                                      0.3) * a;
//                    }
                    
                    float dy = d / in.width;
                    float dx = 1 - (dashArray[0] * factorToT - intraDashPos) / s;
                    
                    if (dy * dy + dx * dx >  0.99 && dy * dy + dx * dx <  1.01) {
                        return float4(0, 0,0,1);

                    } else if (dy * dy + dx * dx > 1) {
                        discard_fragment();
                    } else {
                        return float4(styling[in.stylingIndex + 1],
                                      styling[in.stylingIndex + 2],
                                      styling[in.stylingIndex + 3],
                                      1.0) * a;
                    }
                } else {
                    return float4(styling[in.stylingIndex + 1],
                                  styling[in.stylingIndex + 2],
                                  styling[in.stylingIndex + 3],
                                  1.0) * a;
                }
            }
            
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
