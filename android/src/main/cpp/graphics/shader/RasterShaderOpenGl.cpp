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


RasterShaderOpenGl::RasterShaderOpenGl(bool projectOntoUnitSphere)
        : programName(projectOntoUnitSphere ? "UBMAP_RasterShaderUnitSphereOpenGl" : "UBMAP_RasterShaderOpenGl")
{}

RasterShaderOpenGl::RasterShaderOpenGl(const std::string &programName)
        : programName(programName)
{}

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

void RasterShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context, bool isScreenSpaceCoords) {
    BaseShaderProgramOpenGl::preRender(context, isScreenSpaceCoords);
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int styleValuesLocation = glGetUniformLocation(openGlContext->getProgram(getProgramName()), "styleValues");
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
    if (style.brightnessMin < 0.0f) {
        // Negative brightnessMin is the internal texture-lookup sentinel.
        styleValues[1] = style.brightnessMin;
        // Lookup mode reuses the remaining style fields without color-adjustment transforms:
        styleValues[2] = style.brightnessMax; // lookupU
        styleValues[3] = style.contrast; // lookupV
        styleValues[4] = style.saturation; // lookupWidth
        styleValues[5] = style.gamma; // lookupHeight
        styleValues[6] = style.brightnessShift; // normalized zoom
        return;
    }
    styleValues[1] = style.contrast > 0.0f ? (1.0f / (1.0f - style.contrast)) : (1.0f + style.contrast);
    styleValues[2] = style.saturation > 0.0f ? (1.0f - 1.0f / (1.001f - style.saturation)) : (-style.saturation);
    styleValues[3] = style.brightnessMin;
    styleValues[4] = style.brightnessMax;
    styleValues[5] = style.gamma;
    styleValues[6] = style.brightnessShift;
}

std::string RasterShaderOpenGl::getVertexShader() {
    return BaseShaderProgramOpenGl::getVertexShader();
}

std::string RasterShaderOpenGl::getFragmentShader() {
    return OMMVersionedGlesShaderCode(320 es, 300 es,
                                      precision highp float;
                                      uniform sampler2D textureSampler;
                                      uniform sampler2D lookupTextureSampler;
                                      // Normal: [0] opacity, 0-1 | [1] contrast, 0-1 | [2] saturation, 1-0 | [3] brightnessMin, 0-1 | [4] brightnessMax, 0-1 | [5] gamma, 0.1-10 | [6] brightnessShift, -1-1
                                      // Lookup: [0] opacity | [1] mode sentinel: -1 mono, -2 dual, -3 quad | [2] lookupU | [3] lookupV | [4] lookupWidth | [5] lookupHeight | [6] lookupY
                                      uniform highp float styleValues[7];
                                      in vec2 v_texcoord;
                                      out vec4 fragmentColor;

                                      float lookupX(float value) {
                                          float lookupU = styleValues[2];
                                          float lookupWidth = styleValues[4];
                                          return lookupU + clamp(value, 0.0, 1.0) * lookupWidth;
                                      }

                                      float lookupY(float normalizedZoom) {
                                          float lookupV = styleValues[3];
                                          float lookupHeight = styleValues[5];
                                          return lookupV + clamp(normalizedZoom, 0.0, 1.0) * lookupHeight;
                                      }

                                      float dualLookupY(float normalizedZoom, bool lowerHalf) {
                                          vec2 texSize = vec2(textureSize(lookupTextureSampler, 0));
                                          float lookupV = styleValues[3];
                                          float lookupHeight = styleValues[5];
                                          float fullRows = lookupHeight * texSize.y + 1.0;
                                          float halfRows = max(floor(fullRows * 0.5), 1.0);
                                          float halfRange = max(halfRows - 1.0, 0.0) / texSize.y;
                                          float halfStart = lookupV + (lowerHalf ? halfRows / texSize.y : 0.0);
                                          return halfStart + clamp(normalizedZoom, 0.0, 1.0) * halfRange;
                                      }

                                      float quadLookupX(float value, bool rightHalf) {
                                          vec2 texSize = vec2(textureSize(lookupTextureSampler, 0));
                                          float lookupU = styleValues[2];
                                          float lookupWidth = styleValues[4];
                                          float fullColumns = lookupWidth * texSize.x + 1.0;
                                          float halfColumns = max(floor(fullColumns * 0.5), 1.0);
                                          float halfRange = max(halfColumns - 1.0, 0.0) / texSize.x;
                                          float halfStart = lookupU + (rightHalf ? halfColumns / texSize.x : 0.0);
                                          return halfStart + clamp(value, 0.0, 1.0) * halfRange;
                                      }

                                      void main() {
                                          vec4 color = texture(textureSampler, v_texcoord);
                                          if (styleValues[0] == 0.0 || color.a == 0.0) {
                                              fragmentColor = vec4(0.0);
                                              return;
                                          }
                                          if (styleValues[1] < 0.0) {
                                              vec4 lookupColor;
                                              float styleLookupY = styleValues[6];
                                              if (styleValues[1] <= -2.5) {
                                                  // Quad mode: red selects the horizontal value in both left/right halves,
                                                  // green blends top/bottom rows, blue blends left/right halves.
                                                  float blendY = clamp(color.g, 0.0, 1.0);
                                                  float blendX = clamp(color.b, 0.0, 1.0);
                                                  vec4 primaryColor = texture(lookupTextureSampler, vec2(quadLookupX(color.r, false), dualLookupY(styleLookupY, false)));
                                                  vec4 secondaryColor = texture(lookupTextureSampler, vec2(quadLookupX(color.r, false), dualLookupY(styleLookupY, true)));
                                                  vec4 tertiaryColor = texture(lookupTextureSampler, vec2(quadLookupX(color.r, true), dualLookupY(styleLookupY, false)));
                                                  vec4 quaternaryColor = texture(lookupTextureSampler, vec2(quadLookupX(color.r, true), dualLookupY(styleLookupY, true)));
                                                  lookupColor = mix(mix(primaryColor, secondaryColor, blendY), mix(tertiaryColor, quaternaryColor, blendY), blendX);
                                              } else if (styleValues[1] <= -1.5) {
                                                  // Dual mode: red selects the value in both halves, green blends top/bottom rows.
                                                  vec4 primaryColor = texture(lookupTextureSampler, vec2(lookupX(color.r), dualLookupY(styleLookupY, false)));
                                                  vec4 secondaryColor = texture(lookupTextureSampler, vec2(lookupX(color.r), dualLookupY(styleLookupY, true)));
                                                  lookupColor = mix(primaryColor, secondaryColor, clamp(color.g, 0.0, 1.0));
                                              } else {
                                                  // Mono mode: red selects the lookup value, y is the clamped zoom position.
                                                  lookupColor = texture(lookupTextureSampler, vec2(lookupX(color.r), lookupY(styleLookupY)));
                                              }
                                              fragmentColor = vec4(lookupColor.rgb * color.a, color.a) * styleValues[0];
                                              return;
                                          }
                                          color.rgb = clamp(color.rgb + styleValues[6], 0.0, 1.0); // brightness shift
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
