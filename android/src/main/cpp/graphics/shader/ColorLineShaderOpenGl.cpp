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

    int mColorHandle = glGetUniformLocation(program, "vColor");
    // Set color for drawing the triangle
    std::vector<float> color = {lineColor.r, lineColor.g, lineColor.b, lineColor.a};
    glUniform4fv(mColorHandle, 1, &color[0]);

    int mMiterHandle = glGetUniformLocation(program, "width");
    glUniform1f(mMiterHandle, miter);

    int isScaledHandle = glGetUniformLocation(program, "isScaled");
    glUniform1f(isScaledHandle, lineStyle.widthType == SizeType::SCREEN_PIXEL ? 1.0f : 0.0f);
}

void ColorLineShaderOpenGl::setStyle(const LineStyle &lineStyle) {
    this->lineStyle = lineStyle;
    lineColor = isHighlighted ? lineStyle.color.highlighted : lineStyle.color.normal;
    miter = lineStyle.width;
}

void ColorLineShaderOpenGl::setHighlighted(bool highlighted) {
    lineColor = highlighted ? lineStyle.color.highlighted : lineStyle.color.normal;
    isHighlighted = highlighted;
}

std::string ColorLineShaderOpenGl::getVertexShader() {
    return UBRendererShaderCode(
            precision highp float;
            uniform mat4 uMVPMatrix;
            attribute vec3 vPosition;
            attribute vec3 vWidthNormal;
            attribute vec3 vLengthNormal;
            attribute vec3 vPointA;
            attribute vec3 vPointB;
            uniform float width;
            uniform float isScaled;
            uniform float scaleFactor;

            varying float radius;
            varying vec3 pointDeltaA;
            varying vec3 pointBDeltaA;
            void main() {
                float scaledWidth = width * 0.5;
                if (isScaled > 0.0) {
                    scaledWidth = scaledWidth * scaleFactor;
                }
                vec4 extendedPosition = vec4(vPosition.xyz, 1.0) + vec4((vLengthNormal + vWidthNormal).xyz, 0.0)
                        * vec4(scaledWidth, scaledWidth, scaledWidth, 0.0);
                radius = scaledWidth;
                pointDeltaA = (extendedPosition.xyz - vPointA);
                pointBDeltaA = vPointB - vPointA;
                gl_Position = uMVPMatrix * extendedPosition;
            });
}

std::string ColorLineShaderOpenGl::getFragmentShader() {
    return UBRendererShaderCode(precision highp float;
            uniform vec4 vColor;
            varying float radius;
            varying vec3 pointDeltaA;
            varying vec3 pointBDeltaA;

            void main() {
                float t = dot(pointDeltaA, normalize(pointBDeltaA)) / length(pointBDeltaA);
                float d;
                if (t <= 0.0 || t >= 1.0) {
                    d = min(length(pointDeltaA), length(pointDeltaA - pointBDeltaA));
                } else {
                    vec3 intersectPt = t * pointBDeltaA;
                    d = abs(length(pointDeltaA - intersectPt));
                }

                if (d > radius) {
                    discard;
                }

                gl_FragColor = vColor;
                gl_FragColor.a = 1.0;
                gl_FragColor *= vColor.a;
            });
}