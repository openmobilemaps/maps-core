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

void TextShaderOpenGl::setHaloColor(const ::Color & color, float width, float blur) {
    std::lock_guard<std::mutex> lock(dataMutex);
    haloColor[0] = color.r;
    haloColor[1] = color.g;
    haloColor[2] = color.b;
    haloColor[3] = color.a;
    haloWidth = width;
    haloBlur = blur;
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
    int haloBlurHandle = glGetUniformLocation(program, "haloBlur");
    int opacityHandle = glGetUniformLocation(program, "opacity");
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        glUniform4fv(colorHandle, 1, &color[0]);
        glUniform4fv(haloColorHandle, 1, &haloColor[0]);
        glUniform1f(haloWidthHandle, haloWidth);
        glUniform1f(haloBlurHandle, haloBlur);
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
                                      uniform float haloBlur;
                                      uniform float isHalo; // 0.0 = false, 1.0 = true
                                      in vec2 vTextCoord;
                                      out vec4 fragmentColor;

                                      void main() {
                                          vec4 finalColor = isHalo * haloColor + (1.0 - isHalo) * color; // fill/halo color switch

                                          if (opacity == 0.0 || finalColor.a == 0.0) {
                                              discard;
                                          }

                                          vec4 dist = texture(textureSampler, vTextCoord);

                                          float median = max(min(dist.r, dist.g), min(max(dist.r, dist.g), dist.b)) / dist.a;
                                          float w = fwidth(median);

                                          float fillStart = 0.5 - w;
                                          float fillEnd = 0.5 + w;

                                          float innerFallOff = smoothstep(fillStart, fillEnd, median);

                                          float edgeAlpha = 0.0;

                                          if(bool(isHalo)) {
                                              if (haloWidth == 0.0 && haloBlur == 0.0) {
                                                  discard;
                                              }

                                              float start = max(0.0, fillStart - (haloWidth + 0.5 * haloBlur));
                                              float end = fillStart - max(0.0, haloWidth - 0.5 * haloBlur);

                                              float sideSwitch = step(median, end);
                                              float outerFallOff = smoothstep(start, end, median);

                                              // Combination of blurred outer falloff and inverse inner fill falloff
                                              edgeAlpha = (sideSwitch * outerFallOff + (1.0 - sideSwitch) * (1.0 - innerFallOff)) * finalColor.a;
                                          } else {
                                              edgeAlpha = innerFallOff * finalColor.a;
                                          }

                                          fragmentColor = finalColor;
                                          fragmentColor.a = 1.0;
                                          fragmentColor *= edgeAlpha;
                                      });
}

std::shared_ptr<ShaderProgramInterface> TextShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }
