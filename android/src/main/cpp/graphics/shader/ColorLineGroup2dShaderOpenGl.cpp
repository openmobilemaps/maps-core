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
        int lineStylesHandle = glGetUniformLocation(program, "lineStyles");
        glUniform1fv(lineStylesHandle, sizeStyleValuesArray, &lineStyles[0]);
        int lineColorsHandle = glGetUniformLocation(program, "lineColors");
        glUniform1fv(lineColorsHandle, sizeColorValuesArray, &lineColors[0]);
        int lineGapColorsHandle = glGetUniformLocation(program, "lineGapColors");
        glUniform1fv(lineGapColorsHandle, sizeGapColorValuesArray, &lineGapColors[0]);
        int lineDashValuesHandle = glGetUniformLocation(program, "lineDashValues");
        glUniform1fv(lineDashValuesHandle, sizeDashValuesArray, &lineDashValues[0]);
        int numStylesHandle = glGetUniformLocation(program, "numStyles");
        glUniform1i(numStylesHandle, numStyles);
    }
}

void ColorLineGroup2dShaderOpenGl::setStyles(const std::vector<::LineStyle> &lineStyles) {
    std::vector<float> styleValues(sizeStyleValuesArray, 0.0);
    std::vector<float> colorValues(sizeColorValuesArray, 0.0);
    std::vector<float> gapColorValues(sizeGapColorValuesArray, 0.0);
    std::vector<float> dashValues(sizeDashValuesArray, 0.0);
    int numStyles = std::min((int) lineStyles.size(), maxNumStyles);
    for (int i = 0; i < numStyles; i++) {
        const auto &style = lineStyles[i];
        styleValues[sizeStyleValues * i] = style.width;
        styleValues[sizeStyleValues * i + 1] = style.widthType == SizeType::SCREEN_PIXEL ? 1.0f : 0.0f;
        styleValues[sizeStyleValues * i + 2] = (int)style.lineCap;
        colorValues[sizeColorValues * i] = style.color.normal.r;
        colorValues[sizeColorValues * i + 1] = style.color.normal.g;
        colorValues[sizeColorValues * i + 2] = style.color.normal.b;
        colorValues[sizeColorValues * i + 3] = style.color.normal.a * style.opacity;
        gapColorValues[sizeGapColorValues * i] = style.gapColor.normal.r;
        gapColorValues[sizeGapColorValues * i + 1] = style.gapColor.normal.g;
        gapColorValues[sizeGapColorValues * i + 2] = style.gapColor.normal.b;
        gapColorValues[sizeGapColorValues * i + 3] = style.gapColor.normal.a * style.opacity;
        int numDashInfo = std::min((int)style.dashArray.size(), maxNumDashValues); // Max num dash infos: 4 (2 dash/gap lengths)
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
        this->lineColors = colorValues;
        this->lineGapColors = gapColorValues;
        this->lineDashValues = dashValues;
        this->numStyles = numStyles;
    }
}

std::string ColorLineGroup2dShaderOpenGl::getVertexShader() {
    return UBRendererShaderCode(
        precision highp float;

        uniform mat4 uMVPMatrix; attribute vec2 vPosition; attribute vec2 vWidthNormal; attribute vec2 vLengthNormal;
        attribute vec2 vPointA; attribute vec2 vPointB; attribute float vSegmentStartLPos; attribute float vStyleInfo;
        // lineStyles: {float width, float isScaled, int capType} -> stride = 3
        uniform float lineStyles[3 * ) + std::to_string(maxNumStyles) + UBRendererShaderCode(];
        // lineStyles: {vec4 color} -> stride = 4
        uniform float lineColors[4 * ) + std::to_string(maxNumStyles) + UBRendererShaderCode(];
        // lineStyles: {vec4 gapColor} -> stride = 4
        uniform float lineGapColors[4 * ) + std::to_string(maxNumStyles) + UBRendererShaderCode(];
        uniform int numStyles; uniform float scaleFactor;

        varying float fLineIndex; varying float radius; varying float segmentStartLPos; varying float fSegmentType;
        varying vec2 pointDeltaA; varying vec2 pointBDeltaA; varying vec4 color; varying vec4 gapColor; varying float capType;

        void main() {
            float fStyleIndex = mod(vStyleInfo, 256.0);
            int lineIndex = int(floor(fStyleIndex + 0.5));
            if (lineIndex < 0) {
                lineIndex = 0;
            } else if (lineIndex > numStyles) {
                lineIndex = numStyles;
            }
            int styleIndexBase = 3 * lineIndex;
            int colorIndexBase = 4 * lineIndex;
            int gapColorIndexBase = 4 * lineIndex;
            float width = lineStyles[styleIndexBase];
            float isScaled = lineStyles[styleIndexBase + 1];
            capType = lineStyles[styleIndexBase + 2];
            color = vec4(lineColors[colorIndexBase], lineColors[colorIndexBase + 1], lineColors[colorIndexBase + 2],
                         lineColors[colorIndexBase + 3]);
            gapColor = vec4(lineGapColors[gapColorIndexBase], lineGapColors[gapColorIndexBase + 1],
                            lineGapColors[gapColorIndexBase + 2], lineGapColors[gapColorIndexBase + 3]);
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

std::string ColorLineGroup2dShaderOpenGl::getFragmentShader() {
    return UBRendererShaderCode(precision highp float;

                                // lineDashValues: {int numDashInfo, vec4 dashArray} -> stride = 5
                                uniform float lineDashValues[5 * ) + std::to_string(maxNumStyles) + UBRendererShaderCode(];

                                varying float fLineIndex; varying float radius; varying float segmentStartLPos;
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
                                    int dashBase = 5 * int(fLineIndex);
                                    int numDashInfos = int(floor(lineDashValues[dashBase] + 0.5));
                                    if (numDashInfos > 0) {
                                        int baseDashInfos = dashBase + 1;
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
