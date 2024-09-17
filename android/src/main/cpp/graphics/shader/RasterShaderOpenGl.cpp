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

const std::string RasterShaderOpenGl::programName = "UBMAP_RasterShaderOpenGl";

std::string RasterShaderOpenGl::getProgramName() {
    return programName;
}

void RasterShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);

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
    int styleValuesLocation = glGetUniformLocation(openGlContext->getProgram(programName), "styleValues");
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        glUniform1fv(styleValuesLocation, (GLsizei) styleValues.size(), &styleValues[0]);
    }
}

std::shared_ptr<ShaderProgramInterface> RasterShaderOpenGl::asShaderProgramInterface() {
    return shared_from_this();
}

void RasterShaderOpenGl::setStyle(const RasterShaderStyle &style) {
    std::lock_guard<std::mutex> lock(dataMutex);
     styleValues[0] = style.opacity;
     styleValues[1] = style.contrast > 0.0f ? (1.0f / (1.0f - style.contrast)) : (1.0f + style.contrast);
     styleValues[2] = style.saturation > 0.0f ? (1.0f - 1.0f / (1.001f - style.saturation)) : (-style.saturation);
     styleValues[3] = style.brightnessMin;
     styleValues[4] = style.brightnessMax;
     styleValues[5] = style.gamma;
     styleValues[6] = style.brightnessShift;
}

std::string RasterShaderOpenGl::getFragmentShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      precision highp float;
                                      uniform sampler2D textureSampler;
                                      // [0] opacity, 0-1 | [1] contrast, 0-1 | [2] saturation, 1-0 | [3] brightnessMin, 0-1 | [4] brightnessMax, 0-1 | [5] gamma, 0.1-10 | [6] brightnessShift, -1-1
                                      uniform highp float styleValues[7];
                                      in vec2 v_texcoord;
                                      out vec4 fragmentColor;

                                      void main() {
                                          vec4 color = texture(textureSampler, v_texcoord);
                                          if (styleValues[0] == 0.0 || color.a == 0.0) {
                                              discard;
                                          }
                                          color.rgb = clamp(color.rgb + styleValues[6], 0.0, 1.0); // brighntess shift
                                          float average = (color.r + color.g + color.b) / 3.0;
                                          vec3 rgb = color.rgb + (vec3(average) - color.rgb) * styleValues[2]; // saturation
                                          rgb = clamp((rgb - vec3(0.5)) * styleValues[1] + 0.5, 0.0, 1.0); // contrast (notice range 0-1)

                                          vec3 brightnessMin = vec3(styleValues[3]);
                                          vec3 brightnessMax = vec3(styleValues[4]);

                                          rgb = pow(rgb, vec3(1.0 / styleValues[5])); // gamma
                                          rgb = mix(brightnessMin, brightnessMax, min(rgb / color.a, vec3(1.0))); // brightness min/max mix
                                          fragmentColor = vec4(rgb * color.a, color.a) * styleValues[0];
                                      });
}
