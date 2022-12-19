/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "ColorShaderOpenGl.h"
#include "OpenGlContext.h"
#include "OpenGlHelper.h"

std::string ColorShaderOpenGl::getProgramName() { return "UBMAP_ColorShaderOpenGl"; }

void ColorShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
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

void ColorShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &pass) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(getProgramName());

    int mColorHandle = glGetUniformLocation(program, "vColor");
    glUniform4fv(mColorHandle, 1, &color[0]);
}

void ColorShaderOpenGl::setColor(float red, float green, float blue, float alpha) {
    color = std::vector<float>{red, green, blue, alpha};
}

std::string ColorShaderOpenGl::getVertexShader() {
    return UBRendererShaderCode(precision highp float; uniform mat4 uMVPMatrix; attribute vec4 vPosition;

                                void main() { gl_Position = uMVPMatrix * vPosition; });
}

std::string ColorShaderOpenGl::getFragmentShader() {
    return UBRendererShaderCode(precision mediump float; uniform vec4 vColor;

                                void main() {
                                    gl_FragColor = vColor;
                                    gl_FragColor.a = 1.0;
                                    gl_FragColor *= vColor.a;
                                });
}

std::shared_ptr<ShaderProgramInterface> ColorShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }
