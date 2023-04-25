/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "TextShaderOpenGl.h"
#include "OpenGlContext.h"
#include "OpenGlHelper.h"

std::string TextShaderOpenGl::getProgramName() { return "UBMAP_TextShaderOpenGl"; }

void TextShaderOpenGl::setColor(const ::Color & color) {
    this->color = std::vector<float>{color.r, color.g, color.b, color.a};
}

void TextShaderOpenGl::setHaloColor(const ::Color & color) {
    this->haloColor = std::vector<float>{color.r, color.g, color.b, color.a};
}

void TextShaderOpenGl::setOpacity(float opacity) { this->opacity = opacity; }

void TextShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
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

    checkGlProgramLinking(program);

    openGlContext->storeProgram(programName, program);
}

void TextShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(getProgramName());

    int colorHandle = glGetUniformLocation(program, "color");
    glUniform4fv(colorHandle, 1, &color[0]);

    int haloColorHandle = glGetUniformLocation(program, "haloColor");
    glUniform4fv(haloColorHandle, 1, &haloColor[0]);

    int opacityHandle = glGetUniformLocation(program, "opacity");
    glUniform1f(opacityHandle, opacity);
}

std::string TextShaderOpenGl::getVertexShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      uniform mat4 uMVPMatrix;
                                      uniform vec2 textureCoordScaleFactor;
                                      in vec2 vPosition;
                                      in vec2 texCoordinate;
                                      out vec2 vTextCoord;

                                      void main() {
                                          gl_Position = vec4((uMVPMatrix * vec4(vPosition.xy, 0.0, 1.0)).xy, 0.0, 1.0);
                                          vTextCoord = textureCoordScaleFactor * texCoordinate;
                                      });
}

std::string TextShaderOpenGl::getFragmentShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      precision highp float;
                                      uniform sampler2D textureSampler;
                                      uniform vec4 color;
                                      uniform vec4 haloColor;
                                      uniform float opacity;
                                      in vec2 vTextCoord;
                                      out vec4 fragmentColor;

                                      void main() {
                                          vec4 dist = texture(textureSampler, vTextCoord);
                                          if (haloColor.a == 0.0 && dist.x <= 0.5) {
                                              discard;
                                          }

                                          float delta = 0.1;
                                          float alpha = smoothstep(0.5 - delta, 0.5 + delta, dist.x);
                                          vec4 glyphColor = vec4(color.r, color.g, color.b, color.a * alpha);
                                          vec4 mixed = mix(haloColor, glyphColor, alpha);
                                          float a2 = smoothstep(0.40, 0.5, sqrt(dist.x)) * opacity;
                                          fragmentColor = mixed;
                                          fragmentColor.a = 1.0;
                                          fragmentColor *= a2;
                                      });
}

std::shared_ptr<ShaderProgramInterface> TextShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }
