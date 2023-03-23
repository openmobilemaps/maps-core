/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "ColorLineShaderOpenGl.h"
#include "OpenGlContext.h"
#include "OpenGlHelper.h"

std::shared_ptr<ShaderProgramInterface> ColorLineShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }

std::string ColorLineShaderOpenGl::getProgramName() { return "UBMAP_ColorLineShaderOpenGl"; }

void ColorLineShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
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

void ColorLineShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(getProgramName());

    {
        std::lock_guard<std::recursive_mutex> lock(styleMutex);
        int lineStylesHandle = glGetUniformLocation(program, "lineStyles");
        glUniform1fv(lineStylesHandle, sizeStyleValues, &lineStyles[0]);
        int lineColorsHandle = glGetUniformLocation(program, "lineColors");
        glUniform1fv(lineColorsHandle, sizeColorValues, &lineColors[0]);
        int lineGapColorsHandle = glGetUniformLocation(program, "lineGapColors");
        glUniform1fv(lineGapColorsHandle, sizeGapColorValues, &lineGapColors[0]);
        int lineDashValuesHandle = glGetUniformLocation(program, "lineDashValues");
        glUniform1fv(lineDashValuesHandle, sizeDashValues, &lineDashValues[0]);
    }
}

void ColorLineShaderOpenGl::setStyle(const LineStyle &lineStyle) {
    std::vector<float> styleValues(sizeStyleValues, 0.0);
    std::vector<float> colorValues(sizeColorValues, 0.0);
    std::vector<float> gapColorValues(sizeGapColorValues, 0.0);
    std::vector<float> dashValues(sizeDashValues, 0.0);

    styleValues[0] = lineStyle.width;
    styleValues[1] = lineStyle.widthType == SizeType::SCREEN_PIXEL ? 1.0f : 0.0f;
    styleValues[2] = (int)lineStyle.lineCap;
    colorValues[0] = (isHighlighted) ? lineStyle.color.highlighted.r : lineStyle.color.normal.r;
    colorValues[1] = (isHighlighted) ? lineStyle.color.highlighted.g : lineStyle.color.normal.g;
    colorValues[2] = (isHighlighted) ? lineStyle.color.highlighted.b : lineStyle.color.normal.b;
    colorValues[3] = ((isHighlighted) ? lineStyle.color.highlighted.a : lineStyle.color.normal.a) * lineStyle.opacity;
    gapColorValues[0] = (isHighlighted) ? lineStyle.gapColor.highlighted.r : lineStyle.gapColor.normal.r;
    gapColorValues[1] = (isHighlighted) ? lineStyle.gapColor.highlighted.g : lineStyle.gapColor.normal.g;
    gapColorValues[2] = (isHighlighted) ? lineStyle.gapColor.highlighted.b : lineStyle.gapColor.normal.b;
    gapColorValues[3] = ((isHighlighted) ? lineStyle.gapColor.highlighted.a : lineStyle.gapColor.normal.a) * lineStyle.opacity;
    int numDashInfo = std::min((int)lineStyle.dashArray.size(), maxNumDashValues); // Max num dash infos: 4 (2 dash/gap lengths)
    dashValues[0] = numDashInfo;
    float sum = 0.0;
    for (int iDash = 0; iDash < numDashInfo; iDash++) {
        sum += lineStyle.dashArray.at(iDash);
        dashValues[1 + iDash] = sum;
    }

    {
        std::lock_guard<std::recursive_mutex> overlayLock(styleMutex);
        this->lineStyle = lineStyle;
        this->lineStyles = styleValues;
        this->lineColors = colorValues;
        this->lineGapColors = gapColorValues;
        this->lineDashValues = dashValues;
    }
}

void ColorLineShaderOpenGl::setHighlighted(bool highlighted) {
    isHighlighted = highlighted;
    if (lineStyle) setStyle(*lineStyle);
}

std::string ColorLineShaderOpenGl::getVertexShader() {
    return UBRendererShaderCode(
            precision highp float;

            uniform mat4 uMVPMatrix;
            attribute vec2 vPosition;
            attribute vec2 vWidthNormal;
            attribute vec2 vLengthNormal;
            attribute vec2 vPointA;
            attribute vec2 vPointB;
            attribute float vSegmentStartLPos;
            attribute float vStyleInfo;
            // lineStyles: {float width, float isScaled, int capType} -> stride = 3
            uniform float lineStyles[3];
            // lineStyles: {vec4 color} -> stride = 4
            uniform float lineColors[4];
            // lineStyles: {vec4 gapColor} -> stride = 4
            uniform float lineGapColors[4];
            uniform int numStyles;
            uniform float scaleFactor;

            varying float radius; varying float segmentStartLPos; varying float fSegmentType;
            varying vec2 pointDeltaA; varying vec2 pointBDeltaA; varying vec4 color; varying vec4 gapColor; varying float capType;

            void main() {
                float width = lineStyles[0];
                float isScaled = lineStyles[0 + 1];
                capType = lineStyles[0 + 2];
                color = vec4(lineColors[0], lineColors[0 + 1], lineColors[0 + 2],
                             lineColors[0 + 3]);
                gapColor = vec4(lineGapColors[0], lineGapColors[0 + 1],
                                lineGapColors[0 + 2], lineGapColors[0 + 3]);
                segmentStartLPos = vSegmentStartLPos;
                fSegmentType = vStyleInfo;

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
            });
}

std::string ColorLineShaderOpenGl::getFragmentShader() {
    return UBRendererShaderCode(precision highp float;

                                        // lineDashValues: {int numDashInfo, vec4 dashArray} -> stride = 5
                                        uniform float lineDashValues[5];

                                        varying float radius; varying float segmentStartLPos;
                                        varying float fSegmentType; // 0: inner segment, 1: line start segment (i.e. A is first point in
                                        // line), 2: line end segment, 3: start and end in segment
                                        varying vec2 pointDeltaA; varying vec2 pointBDeltaA; varying vec4 color;
                                        varying float capType; // 0: butt, 1: round, 2: square
                                        varying vec4 gapColor;

                                        void main() {
                                            int segmentType = int(floor(fSegmentType + 0.5));
                                            int iCapType = int(floor(capType + 0.5));
                                            float lineLength = length(pointBDeltaA);
                                            float t = dot(pointDeltaA, normalize(pointBDeltaA)) / lineLength;
                                            float d;
                                            if (t < 0.0 || t > 1.0) {
                                                if (segmentType == 0 || iCapType == 1 || (segmentType == 2 && t < 0.0) ||
                                                    (segmentType == 1 && t > 1.0)) {
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
                                            int numDashInfos = int(floor(lineDashValues[0] + 0.5));
                                            if (numDashInfos > 0) {
                                                int baseDashInfos = 1;
                                                float factorToT = radius * 2.0 / lineLength;
                                                float dashTotal = lineDashValues[baseDashInfos + (numDashInfos - 1)] * factorToT;
                                                float startOffsetSegment = mod(segmentStartLPos / lineLength, dashTotal);
                                                float intraDashPos = mod(t + startOffsetSegment, dashTotal);
                                                // unrolled for efficiency reasons
                                                if ((intraDashPos > lineDashValues[baseDashInfos + 0] * factorToT &&
                                                     intraDashPos < lineDashValues[baseDashInfos + 1] * factorToT) ||
                                                    (intraDashPos > lineDashValues[baseDashInfos + 2] * factorToT &&
                                                     intraDashPos < lineDashValues[baseDashInfos + 3] * factorToT)) {
                                                    fragColor = gapColor;
                                                }
                                            }

                                            gl_FragColor = fragColor;
                                            gl_FragColor.a = 1.0;
                                            gl_FragColor *= fragColor.a;
                                        });
}