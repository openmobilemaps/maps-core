/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "AlphaShaderOpenGl.h"
#include "OpenGlContext.h"
#include "OpenGlHelper.h"

std::string AlphaShaderOpenGl::getProgramName() { return "UBMAP_AlphaShaderOpenGl"; }

void AlphaShaderOpenGl::updateAlpha(float value) { alpha = value; }

void AlphaShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int alphaLocation = glGetUniformLocation(openGlContext->getProgram(getProgramName()), "alpha");
    OpenGlHelper::checkGlError("glGetUniformLocation alpha");
    glUniform1f(alphaLocation, alpha);
}

void AlphaShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    std::string programName = getProgramName();
    // prepare shaders and OpenGL program
    int vertexShader = loadShader(GL_VERTEX_SHADER, getVertexShader());
    int fragmentShader = loadShader(GL_FRAGMENT_SHADER, getFragmentShader());

    int program = glCreateProgram();       // create empty OpenGL Program
    glAttachShader(program, vertexShader); // add the vertex shader to program
    OpenGlHelper::checkGlError("glAttachShader Vertex  AlphaMap");
    glDeleteShader(vertexShader);
    glAttachShader(program, fragmentShader); // add the fragment shader to program
    OpenGlHelper::checkGlError("glAttachShader Fragment AlphaMap");
    glDeleteShader(fragmentShader);
    glLinkProgram(program); // create OpenGL program executables

    checkGlProgramLinking(program);
    OpenGlHelper::checkGlError("glLinkProgram AlphaMap");

    openGlContext->storeProgram(programName, program);
}

std::string AlphaShaderOpenGl::getFragmentShader() {
    return UBRendererShaderCode(precision mediump float; uniform sampler2D u_texture; varying vec2 v_texcoord;
                                uniform highp float alpha;

                                void main() {
                                    vec4 c = texture2D(u_texture, v_texcoord);
                                    highp float a = c.a * alpha;
                                    gl_FragColor = vec4(c.r * a, c.g * a, c.b * a, a);
                                });
}

std::shared_ptr<ShaderProgramInterface> AlphaShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }
