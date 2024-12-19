/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "ColorLineGroup2dShaderOpenGl.h"
#include "OpenGlContext.h"
#include "OpenGlHelper.h"
#include <cassert>
#include <cstring>

ColorLineGroup2dShaderOpenGl::ColorLineGroup2dShaderOpenGl(bool projectOntoUnitSphere)
        : projectOntoUnitSphere(projectOntoUnitSphere),
          programName(projectOntoUnitSphere ? "UBMAP_ColorLineGroupUnitSphereShaderOpenGl" : "UBMAP_ColorLineGroupShaderOpenGl") {
    lineValues.resize(sizeLineValuesArray);
}

std::shared_ptr<ShaderProgramInterface> ColorLineGroup2dShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }

std::string ColorLineGroup2dShaderOpenGl::getProgramName() { return programName; }

void ColorLineGroup2dShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    // prepare shaders and OpenGL program
    int vertexShader = loadShader(GL_VERTEX_SHADER, getVertexShader());
    int fragmentShader = loadShader(GL_FRAGMENT_SHADER, getFragmentShader());

    int program = glCreateProgram();       // create empty OpenGL Program
    glAttachShader(program, vertexShader); // add the vertex shader to program
    glDeleteShader(vertexShader);
    glAttachShader(program, fragmentShader); // add the fragment shader to program
    glDeleteShader(fragmentShader);

    glLinkProgram(program); // create OpenGL program executables

    openGlContext->storeProgram(programName, program);
}

void ColorLineGroup2dShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    BaseShaderProgramOpenGl::preRender(context);
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(programName);

    {
        std::lock_guard<std::recursive_mutex> lock(styleMutex);
        if (numStyles == 0) {
            return;
        }
        int lineStylesHandle = glGetUniformLocation(program, "lineValues");
        glUniform1fv(lineStylesHandle, numStyles * sizeLineValues, &lineValues[0]);
        int numStylesHandle = glGetUniformLocation(program, "numStyles");
        glUniform1i(numStylesHandle, numStyles);
        int dashingScaleFactorHandle = glGetUniformLocation(program, "dashingScaleFactor");
        glUniform1f(dashingScaleFactorHandle, dashingScaleFactor);
    }
}

void ColorLineGroup2dShaderOpenGl::setStyles(const ::SharedBytes & styles) {
    assert(styles.elementCount <= maxNumStyles);
    assert(styles.elementCount * styles.bytesPerElement <= lineValues.size() * sizeof(float));
    {
        std::lock_guard<std::recursive_mutex> overlayLock(styleMutex);
        if(styles.elementCount > 0) {
            std::memcpy(lineValues.data(), (void *)styles.address, styles.elementCount * styles.bytesPerElement);
        }

        numStyles = styles.elementCount;
    }
}

void ColorLineGroup2dShaderOpenGl::setDashingScaleFactor(float factor) {
    {
        std::lock_guard<std::recursive_mutex> overlayLock(styleMutex);
        this->dashingScaleFactor = factor;
    }}

std::string ColorLineGroup2dShaderOpenGl::getVertexShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      precision highp float;
                                      uniform mat4 umMatrix;
                                      uniform mat4 uvpMatrix;
                                      uniform vec4 uOriginOffset;
                                      ) + (projectOntoUnitSphere ? OMMShaderCode(
                                          uniform vec4 uLineOrigin;
                                          in vec3 vPointA;
                                          in vec3 vPointB;
                                      ) : OMMShaderCode(
                                          in vec2 vPointA;
                                          in vec2 vPointB;
                                      )) + OMMShaderCode(
                                      in float vVertexIndex;
                                      in float vSegmentStartLPos;
                                      in float vStyleInfo;
                                      // lineValues:
                                      //        struct LineStyling {
                                      //            float width; // 0
                                      //            float4 color; // 1 2 3 4
                                      //            float4 gapColor; // 5 6 7 8
                                      //            float widthAsPixels; // 9
                                      //            float opacity; // 10
                                      //            float blur; // 11
                                      //            float capType; // 12
                                      //            float numDashValues; // 13
                                      //            float dashArray[4]; // 14 15 16 17
                                      //            float offset; // 18
                                      //            float dotted; // 19
                                      //            float dottedSkew; // 20
                                      //        };
                                      uniform float lineValues[) + std::to_string(sizeLineValuesArray) + OMMShaderCode(];
                                      uniform int numStyles;
                                      uniform float scaleFactor;
                                      uniform float dashingScaleFactor;
                                      out float fStyleIndexBase;
                                      out float radius;
                                      out float segmentStartLPos;
                                      out float fSegmentType;
                                      out vec3 pointDeltaA;
                                      out vec3 pointBDeltaA;
                                      out vec4 color;
                                      out float dashingSize;
                                      out float scaledBlur;

                                       void main() {
                                           float fStyleIndex = mod(vStyleInfo, 256.0);
                                           int lineIndex = int(floor(fStyleIndex + 0.5));
                                           if (lineIndex < 0) {
                                                lineIndex = 0;
                                           } else if (lineIndex > numStyles) {
                                                lineIndex = numStyles;
                                           }
                                           int styleIndexBase = ) + std::to_string(sizeLineValues) + OMMShaderCode(* lineIndex;
                                           int colorIndexBase = styleIndexBase + 1;
                                           float width = lineValues[styleIndexBase];
                                           float isScaled = lineValues[styleIndexBase + 9];
                                           float blur = lineValues[styleIndexBase + 11];
                                           color = vec4(lineValues[colorIndexBase], lineValues[colorIndexBase + 1], lineValues[colorIndexBase + 2],
                                                        lineValues[colorIndexBase + 3]);
                                           segmentStartLPos = vSegmentStartLPos;
                                           fStyleIndexBase = float(styleIndexBase);
                                           fSegmentType = vStyleInfo / 256.0;

                                           ) + (projectOntoUnitSphere ? OMMShaderCode(
                                               vec3 vertexPosition = vPointB;
                                               vec3 lengthNormal = normalize(vPointB - vPointA);
                                           ) : OMMShaderCode(
                                               vec2 vertexPosition = vPointB;
                                               vec3 lengthNormal = vec3(normalize(vPointB - vPointA), 0.0);
                                               vec3 radialNormal = vec3(0.0, 0.0, 1.0);
                                           )) + OMMShaderCode(

                                           float widthNormalFactor = 1.0;
                                           float lengthNormalFactor = 1.0;
                                           if(vVertexIndex == 0.0) {
                                               lengthNormalFactor = -1.0;
                                               widthNormalFactor = -1.0;
                                               vertexPosition = vPointA;
                                           } else if(vVertexIndex == 1.0) {
                                               lengthNormalFactor = -1.0;
                                               vertexPosition = vPointA;
                                           } else if(vVertexIndex == 2.0) {
                                               // all fine
                                           } else if(vVertexIndex == 3.0) {
                                               widthNormalFactor = -1.0;
                                           }

                                           ) + (projectOntoUnitSphere ? OMMShaderCode(
                                               vec3 radialNormal = normalize(vertexPosition + uLineOrigin.xyz);
                                           ) : "") + OMMShaderCode(
                                           vec3 widthNormal = normalize(cross(radialNormal, lengthNormal));

                                           float offsetFloat = lineValues[styleIndexBase + 18] * scaleFactor;
                                           vec3 offset = vec3(widthNormal * offsetFloat);

                                           widthNormal *= widthNormalFactor;
                                           lengthNormal *= lengthNormalFactor;

                                           float scaledWidth = width * 0.5;
                                           dashingSize = width;
                                           if (isScaled > 0.0) {
                                               scaledWidth = scaledWidth * scaleFactor;
                                               blur = blur * scaleFactor;
                                               dashingSize *= dashingScaleFactor;
                                           }

                                           vec3 displ = uOriginOffset.xyz + vec3(lengthNormal + widthNormal) * vec3(scaledWidth) + offset;
                                           vec4 extendedPosition = umMatrix *
                                           ) + (projectOntoUnitSphere ? OMMShaderCode(
                                                   vec4(vertexPosition, 1.0);
                                           ) : OMMShaderCode(
                                                   vec4(vertexPosition, 0.0, 1.0);
                                           )) + OMMShaderCode(
                                           extendedPosition.xyz += displ;
                                           gl_Position = uvpMatrix * extendedPosition;

                                           radius = scaledWidth;
                                           scaledBlur = blur;
                                           ) + (projectOntoUnitSphere ? OMMShaderCode(
                                               pointDeltaA = extendedPosition.xyz - ((vPointA + uOriginOffset.xyz) + offset.xyz);
                                               pointBDeltaA = ((vPointB + uOriginOffset.xyz) + offset.xyz) - ((vPointA + uOriginOffset.xyz) + offset.xyz);
                                           ) : OMMShaderCode(
                                               pointDeltaA = extendedPosition.xyz - ((vec3(vPointA, 0.0) + uOriginOffset.xyz) + offset.xyz);
                                               pointBDeltaA = ((vec3(vPointB, 0.0) + uOriginOffset.xyz) + offset.xyz) - ((vec3(vPointA, 0.0) + uOriginOffset.xyz) + offset.xyz);
                                           )) + OMMShaderCode(
                                       }
                                       );
}

std::string ColorLineGroup2dShaderOpenGl::getFragmentShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      precision highp float;
                                      uniform float lineValues[) + std::to_string(sizeLineValuesArray) + OMMShaderCode(];
                                      in float fStyleIndexBase;
                                      in float radius;
                                      in float segmentStartLPos;
                                      in float scaledBlur;
                                      in float fSegmentType; // 0: inner segment, 1: line start segment (i.e. A is first point in line), 2: line end segment, 3: start and end in segment
                                      in float dashingSize;
                                      in vec3 pointDeltaA;
                                      in vec3 pointBDeltaA;
                                      in vec4 color;

                                      out vec4 fragmentColor;

                                       void main() {
                                           int segmentType = int(floor(fSegmentType + 0.5));
                                           // 0: butt, 1: round, 2: square
                                           int iCapType = int(floor(lineValues[int(fStyleIndexBase) + 12] + 0.5));
                                           float lineLength = length(pointBDeltaA);
                                           float t = dot(pointDeltaA, normalize(pointBDeltaA)) / lineLength;

                                           int dashBase = int(fStyleIndexBase) + 13;
                                           // dash values: {int numDashInfo, vec4 dashArray} -> stride = 5
                                           int numDashInfos = int(floor(lineValues[dashBase] + 0.5));

                                           float d;
                                           if (t < 0.0 || t > 1.0) {
                                               if (numDashInfos > 0 && lineValues[int(fStyleIndexBase) + 14] < 1.0 && lineValues[int(fStyleIndexBase) + 14] > 0.0) {
                                                   discard;
                                               }
                                               if (segmentType == 0 || iCapType == 1 || (segmentType == 2 && t < 0.0) || (segmentType == 1 && t > 1.0)) {
                                                   d = min(length(pointDeltaA), length(pointDeltaA - pointBDeltaA));
                                               } else if (iCapType == 2) {
                                                   float dLen = t < 0.0 ? -t * lineLength : (t - 1.0) * lineLength;
                                                   vec3 intersectPt = t * pointBDeltaA;
                                                   float dOrth = abs(length(pointDeltaA - intersectPt));
                                                   d = max(dLen, dOrth);
                                               } else {
                                                   discard;
                                               }
                                           } else {
                                               vec3 intersectPt = t * pointBDeltaA;
                                               d = abs(length(pointDeltaA - intersectPt));
                                           }

                                           if (d > radius) {
                                               discard;
                                           }

                                           vec4 fragColor = color;
                                           float opacity = lineValues[int(fStyleIndexBase) + 10];
                                           float colorA = lineValues[int(fStyleIndexBase) + 4];
                                           float colorAGap = lineValues[int(fStyleIndexBase) + 8];

                                           float a = colorA * opacity;
                                           float aGap = colorAGap * opacity;

                                           int iDottedLine = int(floor(lineValues[int(fStyleIndexBase) + 19] + 0.5));

                                           if (iDottedLine == 1) {
                                               float skew = lineValues[int(fStyleIndexBase) + 20];

                                               float factorToT = (radius * 2.0) / lineLength * skew;
                                               float dashOffset = (radius - skew * radius) / lineLength;

                                               float dashTotalDotted = 2.0 * factorToT;
                                               float offset = segmentStartLPos / lineLength;
                                               float startOffsetSegmentDotted = mod(offset, dashTotalDotted);
                                               float pos = t + startOffsetSegmentDotted;

                                               float intraDashPosDotted = mod(pos, dashTotalDotted);
                                               if ((intraDashPosDotted > 1.0 * factorToT + dashOffset && intraDashPosDotted < dashTotalDotted - dashOffset) ||
                                               (length(vec2(min(abs(intraDashPosDotted - 0.5 * factorToT), 0.5 * factorToT + dashTotalDotted - intraDashPosDotted) / (0.5 * factorToT + dashOffset), d / radius)) > 1.0)) {
                                                   discard;
                                               }
                                           } else if (numDashInfos > 0) {
                                               float dashArray[4] = float[](lineValues[int(fStyleIndexBase) + 14], lineValues[int(fStyleIndexBase) + 15], lineValues[int(fStyleIndexBase) + 16], lineValues[int(fStyleIndexBase) + 17]);

                                               float factorToT = (radius * 2.0) / lineLength;
                                               float dashTotal = dashArray[3] * factorToT;
                                               float startOffsetSegment = mod(segmentStartLPos / lineLength, dashTotal);
                                               float intraDashPos = mod(t + startOffsetSegment, dashTotal);

                                               if ((intraDashPos > dashArray[0] * factorToT && intraDashPos < dashArray[1] * factorToT) ||
                                                   (intraDashPos > dashArray[2] * factorToT && intraDashPos < dashArray[3] * factorToT)) {
                                                   if (aGap == 0.0) {
                                                       discard;
                                                   }

                                                   int gapColorIndexBase = int(fStyleIndexBase) + 5;
                                                   vec4 gapColor = vec4(lineValues[gapColorIndexBase], lineValues[gapColorIndexBase + 1],
                                                                        lineValues[gapColorIndexBase + 2], lineValues[gapColorIndexBase + 3]);
                                                   fragColor = gapColor;
                                               }
                                            }


                                           if (scaledBlur > 0.0 && t > 0.0 && t < 1.0) {
                                               float nonBlurRange = radius - scaledBlur;
                                               if (d > nonBlurRange) {
                                                   opacity *= clamp(1.0 - max(0.0, d - nonBlurRange) / scaledBlur, 0.0, 1.0);
                                               }
                                           }


                                           fragmentColor = fragColor;
                                           fragmentColor.a = 1.0;
                                           fragmentColor *= fragColor.a * opacity;
                                       });
}
