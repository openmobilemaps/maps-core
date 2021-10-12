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

std::shared_ptr<ShaderProgramInterface>  ColorLineGroup2dShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }


std::string  ColorLineGroup2dShaderOpenGl::getProgramName() { return "UBMAP_ColorLineGroupShaderOpenGl"; }


void  ColorLineGroup2dShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    std::string programName = getProgramName();
    // prepare shaders and OpenGL program
    int vertexShader = loadShader(GL_VERTEX_SHADER, getVertexShader());
    int fragmentShader = loadShader(GL_FRAGMENT_SHADER, getFragmentShader());

    int program = glCreateProgram();       // create empty OpenGL Program
    glAttachShader(program, vertexShader); // add the vertex shader to program
    OpenGlHelper::checkGlError("glAttachShader Vertex  ColorLine Rect");
    glDeleteShader(vertexShader);
    glAttachShader(program, fragmentShader); // add the fragment shader to program
    OpenGlHelper::checkGlError("glAttachShader Fragment ColorLine Rect");
    glDeleteShader(fragmentShader);

    glLinkProgram(program); // create OpenGL program executables
    OpenGlHelper::checkGlError("glLinkProgram ColorLine Rect");

    openGlContext->storeProgram(programName, program);
}

void  ColorLineGroup2dShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(getProgramName());

    {
        std::lock_guard<std::recursive_mutex> lock(styleMutex);
        int lineStylesHandle = glGetUniformLocation(program, "lineStyles");
        glUniform1fv(lineStylesHandle, sizeStyleValuesArray, &lineStyles[0]);
        OpenGlHelper::checkGlError("glUniform1f lineStyles");
        int lineDashValuesHandle = glGetUniformLocation(program, "lineDashValues");
        glUniform1fv(lineDashValuesHandle, sizeDashValuesArray, &lineDashValues[0]);
        OpenGlHelper::checkGlError("glUniform1f lineDashValues");
        int numStylesHandle = glGetUniformLocation(program, "numStyles");
        glUniform1i(numStylesHandle, numStyles);
        OpenGlHelper::checkGlError("glUniform1f numStyles");
    }
}

void  ColorLineGroup2dShaderOpenGl::setStyles(const std::vector<::LineStyle> &lineStyles) {
    std::vector<float> styleValues(sizeStyleValuesArray, 0.0);
    std::vector<float> dashValues(sizeDashValuesArray, 0.0);
    int numStyles = lineStyles.size();
    for (int i = 0; i < lineStyles.size(); i++) {
        const auto &style = lineStyles[i];
        styleValues[sizeStyleValues * i] = style.width;
        styleValues[sizeStyleValues * i + 1] = style.widthType == SizeType::SCREEN_PIXEL ? 1.0f : 0.0f;
        styleValues[sizeStyleValues * i + 2] = style.color.normal.r;
        styleValues[sizeStyleValues * i + 3] = style.color.normal.g;
        styleValues[sizeStyleValues * i + 4] = style.color.normal.b;
        styleValues[sizeStyleValues * i + 5] = style.color.normal.a * style.opacity;
        styleValues[sizeStyleValues * i + 6] = (int) style.lineCap;

        int numDashInfo = std::min((int) style.dashArray.size(), maxNumDashValues); // Max num dash infos: 8 (4 dash/gap lengths)
        dashValues[sizeDashValues * i] = numDashInfo;
        float sum = 0.0;
        for (int iDash = 0; iDash < numDashInfo; iDash++) {
            sum += style.dashArray.at(iDash);
            dashValues[sizeDashValues * i + 1 + iDash] = sum;
        }
    }

    {
        std::lock_guard<std::recursive_mutex> overlayLock(styleMutex);
        this->lineStyles = styleValues;
        this->lineDashValues = dashValues;
        this->numStyles = numStyles;
    }
}


std::string  ColorLineGroup2dShaderOpenGl::getVertexShader() {
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
            // lineStyles: {float width, float isScaled, vec4 color, int capType} -> stride = 7
            uniform float lineStyles[7 * 32];
            uniform int numStyles;
            uniform float scaleFactor;

            varying float fLineIndex;
            varying float radius;
            varying float segmentStartLPos;
            varying float fSegmentType;
            varying vec2 pointDeltaA;
            varying vec2 pointBDeltaA;
            varying vec4 color;
            varying float capType;

            void main() {
                float fStyleIndex = mod(vStyleInfo, 256.0);
                int lineIndex = clamp(int(floor(fStyleIndex + 0.5)), 0, numStyles);
                int styleIndexBase = 7 * lineIndex;
                float width = lineStyles[styleIndexBase];
                float isScaled = lineStyles[styleIndexBase + 1];
                color = vec4(lineStyles[styleIndexBase + 2], lineStyles[styleIndexBase + 3], lineStyles[styleIndexBase + 4], lineStyles[styleIndexBase + 5]);
                capType = lineStyles[styleIndexBase + 6];
                segmentStartLPos = vSegmentStartLPos;
                fLineIndex = float(lineIndex);
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
            });
}

std::string  ColorLineGroup2dShaderOpenGl::getFragmentShader() {
    return UBRendererShaderCode(
            precision highp float;

            // lineDashValues: {int numDashInfo, vec8 dashArray} -> stride = 9
            uniform float lineDashValues[9 * 32];

            varying float fLineIndex;
            varying float radius;
            varying float segmentStartLPos;
            varying float fSegmentType; // 0: inner segment, 1: line start segment (i.e. A is first point in line), 2: line end segment, 3: start and end in segment
            varying vec2 pointDeltaA;
            varying vec2 pointBDeltaA;
            varying vec4 color;
            varying float capType; // 0: butt, 1: round, 2: square

            void main() {
                int segmentType = int(floor(fSegmentType + 0.5));
                int iCapType = int(floor(capType + 0.5));
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

                int dashBase = 9 * int(fLineIndex);
                int numDashInfos = int(floor(lineDashValues[dashBase] + 0.5));
                if (numDashInfos > 0) {
                    int baseDashInfos = dashBase + 1;
                    float factorToT = radius * 2.0 / lineLength;
                    float dashTotal = lineDashValues[baseDashInfos + (numDashInfos - 1)] * factorToT;
                    float startOffsetSegment = mod(segmentStartLPos / lineLength, dashTotal);
                    float intraDashPos = mod(t + startOffsetSegment, dashTotal);
                    // unrolled for efficiency reasons
                    if ((intraDashPos > lineDashValues[baseDashInfos + 0] * factorToT && intraDashPos < lineDashValues[baseDashInfos + 1] * factorToT) ||
                        (intraDashPos > lineDashValues[baseDashInfos + 2] * factorToT && intraDashPos < lineDashValues[baseDashInfos + 3] * factorToT) ||
                        (intraDashPos > lineDashValues[baseDashInfos + 4] * factorToT && intraDashPos < lineDashValues[baseDashInfos + 5] * factorToT) ||
                        (intraDashPos > lineDashValues[baseDashInfos + 6] * factorToT && intraDashPos < lineDashValues[baseDashInfos + 7] * factorToT)) {
                      discard;
                    }
                }

                gl_FragColor = color;
                gl_FragColor.a = 1.0;
                gl_FragColor *= color.a;
            });
}