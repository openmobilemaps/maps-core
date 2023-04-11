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

    openGlContext->storeProgram(programName, program);
}

void ColorLineGroup2dShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(getProgramName());

    {
        std::lock_guard<std::recursive_mutex> lock(styleMutex);
        if (numStyles == 0) {
            return;
        }
        int lineStylesHandle = glGetUniformLocation(program, "lineValues");
        glUniform1fv(lineStylesHandle, sizeLineValuesArray, &lineValues[0]);
        int numStylesHandle = glGetUniformLocation(program, "numStyles");
        glUniform1i(numStylesHandle, numStyles);
    }
}

void ColorLineGroup2dShaderOpenGl::setStyles(const std::vector<::LineStyle> &lineStyles) {
    std::vector<float> lineValues(sizeLineValuesArray, 0.0);
    int numStyles = std::min((int) lineStyles.size(), maxNumStyles);
    for (int i = 0; i < numStyles; i++) {
        const auto &style = lineStyles[i];
        lineValues[sizeLineValues * i] = style.width;
        lineValues[sizeLineValues * i + 1] = style.widthType == SizeType::SCREEN_PIXEL ? 1.0f : 0.0f;
        lineValues[sizeLineValues * i + 2] = (int)style.lineCap;
        lineValues[sizeLineValues * i + 3] = style.color.normal.r;
        lineValues[sizeLineValues * i + 4] = style.color.normal.g;
        lineValues[sizeLineValues * i + 5] = style.color.normal.b;
        lineValues[sizeLineValues * i + 6] = style.color.normal.a * style.opacity;
        lineValues[sizeLineValues * i + 7] = style.gapColor.normal.r;
        lineValues[sizeLineValues * i + 8] = style.gapColor.normal.g;
        lineValues[sizeLineValues * i + 9] = style.gapColor.normal.b;
        lineValues[sizeLineValues * i + 10] = style.gapColor.normal.a * style.opacity;
        int numDashInfo = std::min((int)style.dashArray.size(), maxNumDashValues); // Max num dash infos: 4 (2 dash/gap lengths)
        lineValues[sizeLineValues * i + 11] = numDashInfo;
        float sum = 0.0;
        for (int iDash = 0; iDash < numDashInfo; iDash++) {
            sum += style.dashArray.at(iDash);
            lineValues[sizeLineValues * i + 12 + iDash] = sum;
        }
    }

    {
        std::lock_guard<std::recursive_mutex> overlayLock(styleMutex);
        this->lineValues = lineValues;
        this->numStyles = numStyles;
    }
}

std::string ColorLineGroup2dShaderOpenGl::getVertexShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      precision highp float;
                                      uniform mat4 uMVPMatrix; in vec2 vPosition; in vec2 vWidthNormal; in vec2 vLengthNormal;
                                      in vec2 vPointA; in vec2 vPointB; in float vSegmentStartLPos; in float vStyleInfo;
                                      // lineValues:
                                      // {float width (0),
                                      // float isScaled (1),
                                      // int capType (2),
                                      // vec4 color (3),
                                      // vec4 gapColor (7),
                                      // int numDashInfo (11),
                                      // vec4 dashArray (12)}
                                      // -> stride = 16
                                      uniform float lineValues[) + std::to_string(sizeLineValuesArray) + OMMShaderCode(];
                                      uniform int numStyles; uniform float scaleFactor;
                                      out float fStyleIndexBase; out float radius; out float segmentStartLPos; out float fSegmentType;
                                      out vec2 pointDeltaA; out vec2 pointBDeltaA; out vec4 color;

                                       void main() {
                                       float fStyleIndex = mod(vStyleInfo, 256.0);
                                       int lineIndex = int(floor(fStyleIndex + 0.5));
                                       if (lineIndex < 0) {
                                            lineIndex = 0;
                                       } else if (lineIndex > numStyles) {
                                            lineIndex = numStyles;
                                       }
                                       int styleIndexBase = ) + std::to_string(sizeLineValues) + OMMShaderCode(* lineIndex;
                                       int colorIndexBase = styleIndexBase + 3;
                                       float width = lineValues[styleIndexBase];
                                       float isScaled = lineValues[styleIndexBase + 1];
                                       color = vec4(lineValues[colorIndexBase], lineValues[colorIndexBase + 1], lineValues[colorIndexBase + 2],
                                                    lineValues[colorIndexBase + 3]);
                                       segmentStartLPos = vSegmentStartLPos;
                                       fStyleIndexBase = float(styleIndexBase);
                                       fSegmentType = vStyleInfo / 256.0;

                                       float scaledWidth = width * 0.5;
                                       if (isScaled > 0.0) {
                                       scaledWidth = scaledWidth * scaleFactor;
                                       }
                                               vec4 trfPosition = uMVPMatrix * vec4(vPosition.xy, 0.0, 1.0);
                                               vec4 displ = vec4((vLengthNormal + vWidthNormal).xy, 0.0, 0.0) * vec4(scaledWidth, scaledWidth, 0.0, 0.0);
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
                                      uniform float lineValues[) + std::to_string(sizeLineValuesArray) + OMMShaderCode(];
                                      in float fStyleIndexBase; in float radius; in float segmentStartLPos;
                                      in float fSegmentType; // 0: inner segment, 1: line start segment (i.e. A is first point in line), 2: line end segment, 3: start and end in segment
                                      in vec2 pointDeltaA; in vec2 pointBDeltaA; in vec4 color;

                                      out vec4 fragmentColor;

                                       void main() {
                                           int segmentType = int(floor(fSegmentType + 0.5));
                                           // 0: butt, 1: round, 2: square
                                           int iCapType = int(floor(lineValues[int(fStyleIndexBase) + 2] + 0.5));
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

                                           int dashBase = int(fStyleIndexBase) + 3 + 4 + 4;
                                           // dash values: {int numDashInfo, vec4 dashArray} -> stride = 5
                                           int numDashInfos = int(floor(lineValues[dashBase] + 0.5));
                                           if (numDashInfos > 0) {
                                               int gapColorIndexBase = int(fStyleIndexBase) + 3 + 4;
                                               vec4 gapColor = vec4(lineValues[gapColorIndexBase], lineValues[gapColorIndexBase + 1],
                                                                    lineValues[gapColorIndexBase + 2], lineValues[gapColorIndexBase + 3]);

                                               int baseDashInfos = dashBase + 1;
                                               float factorToT = radius * 2.0 / lineLength;
                                               float dashTotal = lineValues[baseDashInfos + (numDashInfos - 1)] * factorToT;
                                               float startOffsetSegment = mod(segmentStartLPos / lineLength, dashTotal);
                                               float intraDashPos = mod(t + startOffsetSegment, dashTotal);
                                               // unrolled for efficiency reasons
                                               if ((intraDashPos > lineValues[baseDashInfos + 0] * factorToT &&
                                                intraDashPos < lineValues[baseDashInfos + 1] * factorToT) ||
                                                (intraDashPos > lineValues[baseDashInfos + 2] * factorToT &&
                                                intraDashPos < lineValues[baseDashInfos + 3] * factorToT)) {
                                                    fragColor = gapColor;
                                                }
                                            }

                                           fragmentColor = fragColor;
                                           fragmentColor.a = 1.0;
                                           fragmentColor *= fragColor.a;
                                       });
}
