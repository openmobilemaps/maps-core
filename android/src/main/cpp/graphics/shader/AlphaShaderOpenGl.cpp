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

AlphaShaderOpenGl::AlphaShaderOpenGl(bool projectOntoUnitSphere)
: projectOntoUnitSphere(projectOntoUnitSphere),
programName(projectOntoUnitSphere ? "UBMAP_AlphaUnitSphereShaderOpenGl" : "UBMAP_AlphaShaderOpenGl") {}

std::string AlphaShaderOpenGl::getProgramName() { return programName; }

void AlphaShaderOpenGl::updateAlpha(float value) { alpha = value; }

void AlphaShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    BaseShaderProgramOpenGl::preRender(context);
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int alphaLocation = glGetUniformLocation(openGlContext->getProgram(programName), "alpha");
    glUniform1f(alphaLocation, alpha);
}

void AlphaShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
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

    checkGlProgramLinking(program);

    openGlContext->storeProgram(programName, program);
}

std::string AlphaShaderOpenGl::getVertexShader() {
    return projectOntoUnitSphere ?
           // Vertices projected onto unit sphere
           BaseShaderProgramOpenGl::getUnitSphereVertexShader()
           // Default Shader
           : BaseShaderProgramOpenGl::getVertexShader();
}

std::string AlphaShaderOpenGl::getFragmentShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      precision mediump float;
                                      uniform sampler2D textureSampler;
                                      uniform highp float alpha;
                                      in vec2 v_texcoord;
                                      out vec4 fragmentColor;

                                      void main() {
                                          vec4 c = texture(textureSampler, v_texcoord);
                                          fragmentColor = c * alpha;
                                      }
    );
}

std::shared_ptr<ShaderProgramInterface> AlphaShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }
