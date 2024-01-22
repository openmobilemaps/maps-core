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

const std::string TextShaderOpenGl::programName = "UBMAP_TextShaderOpenGl";

std::string TextShaderOpenGl::getProgramName() { return programName; }

void TextShaderOpenGl::setColor(const ::Color & c) {
    std::lock_guard<std::mutex> lock(dataMutex);
    color[0] = c.r;
    color[1] = c.g;
    color[2] = c.b;
    color[3] = c.a;
}

void TextShaderOpenGl::setHaloColor(const ::Color & color, double width) {
    std::lock_guard<std::mutex> lock(dataMutex);
    haloColor[0] = color.r;
    haloColor[1] = color.g;
    haloColor[2] = color.b;
    haloColor[3] = color.a;
    haloWidth = width;
}

void TextShaderOpenGl::setOpacity(float opacity) { this->opacity = opacity; }

void TextShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
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

void TextShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    BaseShaderProgramOpenGl::preRender(context);
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(programName);

    int colorHandle = glGetUniformLocation(program, "color");
    int haloColorHandle = glGetUniformLocation(program, "haloColor");
    int haloWidthHandle = glGetUniformLocation(program, "haloWidth");
    int opacityHandle = glGetUniformLocation(program, "opacity");
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        glUniform4fv(colorHandle, 1, &color[0]);
        glUniform4fv(haloColorHandle, 1, &haloColor[0]);
        glUniform1f(haloWidthHandle, haloWidth);
        glUniform1f(opacityHandle, opacity);
    }



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
                                      uniform float haloWidth;
                                      in vec2 vTextCoord;
                                      out vec4 fragmentColor;

                                      void main() {
                                          vec4 dist = texture(textureSampler, vTextCoord);

                                          if(opacity == 0.0) {
                                              discard;
                                          }

                                          float median = max(min(dist.r, dist.g), min(max(dist.r, dist.g), dist.b)) / dist.a;

                                          float w = fwidth(median);
                                          float alpha = smoothstep(0.5 - w, 0.5 + w, median);

                                          vec4 mixed = mix(haloColor, color, alpha);

                                          if(haloWidth > 0.0) {
                                              float start = (0.0 + 0.5 * (1.0 - haloWidth)) - w;
                                              float end = start + w;
                                              float a2 = smoothstep(start, end, median) * opacity;
                                              fragmentColor = mixed;
                                              fragmentColor.a = 1.0;
                                              fragmentColor *= a2;
                                          } else {
                                              fragmentColor = mixed;
                                          }
                                      });
}

std::shared_ptr<ShaderProgramInterface> TextShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }
