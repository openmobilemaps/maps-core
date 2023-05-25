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

std::shared_ptr<ShaderProgramInterface> ColorLineGroup2dShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }

std::string ColorLineGroup2dShaderOpenGl::getProgramName() { return "UBMAP_ColorLineGroupShaderOpenGl"; }

void ColorLineGroup2dShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    std::string programName = getProgramName();
    // prepare shaders and OpenGL program
    int vertexShader = loadShader(GL_VERTEX_SHADER, getVertexShader());
    int fragmentShader = loadShader(GL_FRAGMENT_SHADER, getFragmentShader());

    int program = glCreateProgram();       // create empty OpenGL Program
    glAttachShader(program, vertexShader); // add the vertex shader to program
    glDeleteShader(vertexShader);
    glAttachShader(program, fragmentShader); // add the fragment shader to program
    glDeleteShader(fragmentShader);

    glLinkProgram(program); // create OpenGL program executables

    styleBufferHandle = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, "lineGroupStyleBuffer");
    glGenBuffers(1, &styleBuffer);

    openGlContext->storeProgram(programName, program);
}

void ColorLineGroup2dShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(getProgramName());

    {
        std::lock_guard<std::recursive_mutex> lock(styleMutex);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, styleBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, styleBufferHandle, styleBuffer);

        int dashingScaleFactorHandle = glGetUniformLocation(program, "dashingScaleFactor");
        glUniform1f(dashingScaleFactorHandle, dashingScaleFactor);
    }
}

void ColorLineGroup2dShaderOpenGl::setStyles(const ::SharedBytes &styles) {
    std::lock_guard<std::recursive_mutex> overlayLock(styleMutex);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, styleBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, styles.elementCount * styles.bytesPerElement,
                 (void *) styles.address, GL_DYNAMIC_DRAW);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ColorLineGroup2dShaderOpenGl::setDashingScaleFactor(float factor) {
    std::lock_guard<std::recursive_mutex> overlayLock(styleMutex);
    this->dashingScaleFactor = factor;
}

std::string ColorLineGroup2dShaderOpenGl::getVertexShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      precision highp float;
                                      uniform mat4 uMVPMatrix;
                                      in vec2 vPosition;
                                      in vec2 vWidthNormal;
                                      in vec2 vPointA;
                                      in vec2 vPointB;
                                      in float vVertexIndex;
                                      in float vSegmentStartLPos;
                                      in float vStyleInfo;

                                      layout(std430, binding = 0) buffer lineGroupStyleBuffer {
                                              float styles[];
                                      };
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
                                      //        };

                                      uniform float scaleFactor;
                                      uniform float dashingScaleFactor;
                                      out float fStyleIndexBase;
                                      out float radius;
                                      out float segmentStartLPos;
                                      out float fSegmentType;
                                      out vec2 pointDeltaA;
                                      out vec2 pointBDeltaA;
                                      out vec4 color;
                                      out float dashingSize;

                                       void main() {
                                       int styleIndex = int(vStyleInfo) & 0xFF;
                                       styleIndex *= 19;
                                       int styleIndexBase = styleIndex;
                                       int colorIndexBase = styleIndexBase + 1;
                                       float width = styles[styleIndexBase];
                                       float isScaled = styles[styleIndexBase + 9];
                                       color = vec4(styles[colorIndexBase], styles[colorIndexBase + 1], styles[colorIndexBase + 2],
                                                    styles[colorIndexBase + 3]);
                                       segmentStartLPos = vSegmentStartLPos;
                                       fStyleIndexBase = float(styleIndexBase);
                                       fSegmentType = vStyleInfo / 256.0;

                                           vec2 widthNormal = vWidthNormal;
                                           vec2 lengthNormal = vec2(widthNormal.y, -widthNormal.x);

                                           if(vVertexIndex == 0.0) {
                                               lengthNormal *= -1.0;
                                               widthNormal *= -1.0;
                                           } else if(vVertexIndex == 1.0) {
                                               lengthNormal *= -1.0;
                                           } else if(vVertexIndex == 2.0) {
                                               // all fine
                                           } else if(vVertexIndex == 3.0) {
                                               widthNormal *= -1.0;
                                           }

                                       float offsetFloat = styles[styleIndexBase + 18] * scaleFactor;
                                       vec4 offset = vec4(vWidthNormal.x * offsetFloat, vWidthNormal.y * offsetFloat, 0.0, 0.0);

                                           float scaledWidth = width * 0.5;
                                           dashingSize = width;
                                           if (isScaled > 0.0) {
                                               scaledWidth = scaledWidth * scaleFactor;
                                               dashingSize *= dashingScaleFactor;
                                           }

                                           vec4 trfPosition = uMVPMatrix * vec4(vPosition.xy, 0.0, 1.0);
                                           vec4 displ = vec4((lengthNormal + widthNormal).xy, 0.0, 0.0) * vec4(scaledWidth, scaledWidth, 0.0, 0.0) + offset;
                                           vec4 trfDispl = uMVPMatrix * displ;
                                           vec4 extendedPosition = vec4(vPosition.xy, 0.0, 1.0) + displ;
                                           radius = scaledWidth;
                                           pointDeltaA = (extendedPosition.xy - vPointA);
                                           pointBDeltaA = vPointB - vPointA;
                                           gl_Position = trfPosition + trfDispl;
                                       }
                                       );
}

std::string ColorLineGroup2dShaderOpenGl::getFragmentShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      precision highp float;

                                      layout(std430, binding = 0) buffer lineGroupStyleBuffer {
                                              float styles[];
                                      };

                                      in float fStyleIndexBase;
                                      in float radius;
                                      in float segmentStartLPos;
                                      in float fSegmentType; // 0: inner segment, 1: line start segment (i.e. A is first point in line), 2: line end segment, 3: start and end in segment
                                      in float dashingSize;
                                      in vec2 pointDeltaA;
                                      in vec2 pointBDeltaA;
                                      in vec4 color;

                                      out vec4 fragmentColor;

                                       void main() {
                                           int segmentType = int(floor(fSegmentType + 0.5));
                                           // 0: butt, 1: round, 2: square
                                           int iCapType = int(floor(styles[int(fStyleIndexBase) + 12] + 0.5));
                                           float lineLength = length(pointBDeltaA);
                                           float t = dot(pointDeltaA, normalize(pointBDeltaA)) / lineLength;
                                           float d;
                                           if (t < 0.0 || t > 1.0) {
                                               if (segmentType == 0 || iCapType == 1 || (segmentType == 2 && t < 0.0) || (segmentType == 1 && t > 1.0)) {
                                                   d = min(length(pointDeltaA), length(pointDeltaA - pointBDeltaA));
                                               } else if (iCapType == 2) {
                                                   float dLen = t < 0.0 ? -t * lineLength : (t - 1.0) * lineLength;
                                                   vec2 intersectPt = t * pointBDeltaA;
                                                   float dOrth = abs(length(pointDeltaA - intersectPt));
                                                   d = max(dLen, dOrth);
                                               } else {
                                                   discard;
                                               }
                                           } else {
                                               vec2 intersectPt = t * pointBDeltaA;
                                               d = abs(length(pointDeltaA - intersectPt));
                                           }

                                           if (d > radius) {
                                               discard;
                                           }

                                           vec4 fragColor = color;
                                           float opacity = styles[int(fStyleIndexBase) + 10];

                                           int dashBase = int(fStyleIndexBase) + 13;
                                           // dash values: {int numDashInfo, vec4 dashArray} -> stride = 5
                                           int numDashInfos = int(floor(styles[dashBase] + 0.5));
                                           if (numDashInfos > 0) {
                                               int gapColorIndexBase = int(fStyleIndexBase) + 5;
                                               vec4 gapColor = vec4(styles[gapColorIndexBase], styles[gapColorIndexBase + 1],
                                                                    styles[gapColorIndexBase + 2], styles[gapColorIndexBase + 3]);

                                               int baseDashInfos = dashBase + 1;
                                               float factorToT = dashingSize / lineLength;
                                               float dashTotal = styles[baseDashInfos + (numDashInfos - 1)] * factorToT;
                                               float startOffsetSegment = mod(segmentStartLPos / lineLength, dashTotal);
                                               float intraDashPos = mod(t + startOffsetSegment, dashTotal);
                                               // unrolled for efficiency reasons
                                               if ((intraDashPos > styles[baseDashInfos + 0] * factorToT &&
                                                intraDashPos < styles[baseDashInfos + 1] * factorToT) ||
                                                (intraDashPos > styles[baseDashInfos + 2] * factorToT &&
                                                intraDashPos < styles[baseDashInfos + 3] * factorToT)) {
                                                    fragColor = gapColor;
                                                }
                                            }

                                           fragmentColor = fragColor;
                                           fragmentColor.a = 1.0;
                                           fragmentColor *= fragColor.a * opacity;
                                           fragmentColor = vec4(1.0, 0.0, 0.0, 1.0);
                                       });
}

void ColorLineGroup2dShaderOpenGl::clear() {
    glDeleteBuffers(1, &styleBuffer);
}
