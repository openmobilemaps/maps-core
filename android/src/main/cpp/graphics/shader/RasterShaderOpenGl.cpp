/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "RasterShaderOpenGl.h"
#include "OpenGlContext.h"

std::string RasterShaderOpenGl::getProgramName() {
    return "UBMAP_RasterShaderOpenGl";
}

void RasterShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    std::string programName = getProgramName();

    int vertexShader = loadShader(GL_VERTEX_SHADER, getVertexShader());
    int fragmentShader = loadShader(GL_FRAGMENT_SHADER, getFragmentShader());

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glAttachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);
    glLinkProgram(program);

    checkGlProgramLinking(program);

    openGlContext->storeProgram(programName, program);
}

void RasterShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    BaseShaderProgramOpenGl::preRender(context);
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int styleValuesLocation = glGetUniformLocation(openGlContext->getProgram(getProgramName()), "styleValues");
    glUniform1fv(styleValuesLocation, (GLsizei) styleValues.size(), &styleValues[0]);
}

std::shared_ptr<ShaderProgramInterface> RasterShaderOpenGl::asShaderProgramInterface() {
    return shared_from_this();
}

void RasterShaderOpenGl::setStyle(const RasterShaderStyle &style) {
     styleValues[0] = style.opacity;
     styleValues[1] = style.contrast > 0.0f ? (1.0f / (1.0f - style.contrast)) : (1.0f + style.contrast);
     styleValues[2] = style.saturation > 0.0f ? (1.0f - 1.0f / (1.001f - style.saturation)) : (-style.saturation);
     styleValues[3] = style.brightnessMin;
     styleValues[4] = style.brightnessMax;
}

std::string RasterShaderOpenGl::getFragmentShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      precision mediump float;
                                      uniform sampler2D textureSampler;
                                      // [0] opacity, 0-1 | [1] contrast, 0-1 | [2] saturation, 1-0 | [3] brightnessMin, 0-1 | [4] brightnessMax, 0-1
                                      uniform highp float styleValues[5];
                                      in vec2 v_texcoord;
                                      out vec4 fragmentColor;

                                      void main() {
                                          vec4 color = texture(textureSampler, v_texcoord);
                                          if (styleValues[0] == 0.0) {
                                              discard;
                                          }
                                          color.a *= styleValues[0];
                                          float average = (color.r + color.g + color.b) / 3.0;
                                          vec3 rgb = color.rgb + (vec3(average) - color.rgb) * styleValues[2];
                                          rgb = (rgb - vec3(0.5)) * styleValues[1] + 0.5;

                                          vec3 brightnessMin = vec3(styleValues[3]);
                                          vec3 brightnessMax = vec3(styleValues[4]);
                                          fragmentColor = vec4(mix(brightnessMin, brightnessMax, rgb) * color.a, color.a);
                                      });
}