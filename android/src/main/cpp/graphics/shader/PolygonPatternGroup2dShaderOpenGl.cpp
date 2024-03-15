/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "PolygonPatternGroup2dShaderOpenGl.h"
#include "OpenGlContext.h"
#include "OpenGlHelper.h"

#include <algorithm>
#include <string>
#include <iostream>

PolygonPatternGroup2dShaderOpenGl::PolygonPatternGroup2dShaderOpenGl(bool fadeInPattern)
        : fadeInPattern(fadeInPattern),
          programName(std::string("UBMAP_PolygonPatternGroup2dShaderOpenGl_") + (fadeInPattern ? "std" : "fade")) {}

std::string PolygonPatternGroup2dShaderOpenGl::getProgramName() { return programName; }

void PolygonPatternGroup2dShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
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

void PolygonPatternGroup2dShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    BaseShaderProgramOpenGl::preRender(context);
}

std::string PolygonPatternGroup2dShaderOpenGl::getVertexShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      in vec2 vPosition;
                                      in float vStyleIndex;

                                      uniform mat4 uvpMatrix;
                                      uniform vec2 uScalingFactor;
    ) + (fadeInPattern ? OMMShaderCode(uniform float uScreenPixelAsRealMeterFactor;) : "")
    + OMMShaderCode(

                                      out vec2 pixelPosition;
                                      out flat uint styleIndex;

                                      void main() {
                         ) + (fadeInPattern ? OMMShaderCode(
                                        // fadeInPattern
                                        pixelPosition = vPosition.xy / vec2(uScreenPixelAsRealMeterFactor);
    ) : OMMShaderCode(
                                        // DefaultBehavior
                                        pixelPosition = vPosition.xy / uScalingFactor;
    )) + OMMShaderCode(
                                        styleIndex = uint(floor(vStyleIndex + 0.5));
                                        gl_Position = uvpMatrix * vec4(vPosition, 0.0, 1.0);
                                      }
    );
}

std::string PolygonPatternGroup2dShaderOpenGl::getFragmentShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      precision highp float;

                                      uniform sampler2D uTextureSampler;
                                      uniform vec2 uTextureFactor;
                                      uniform float textureCoordinates[5 * 16];
                                      uniform float opacities[16];
           ) + (fadeInPattern ? OMMShaderCode(
                                      uniform float uScreenPixelAsRealMeterFactor;
                                      uniform vec2 uScalingFactor;) : "")
           + OMMShaderCode(

                                      in vec2 pixelPosition;
                                      in flat uint styleIndex;

                                      out vec4 fragmentColor;

                                      void main() {
                                          float opacity = opacities[int(styleIndex)];
                                          if (opacity == 0.0) {
                                              discard;
                                          }

                                          int styleOffset = min(int(styleIndex) * 5, 16 * 5);
                                          vec2 uvSize =
                                                  vec2(textureCoordinates[styleOffset + 2], textureCoordinates[styleOffset + 3]) *
                                                  uTextureFactor;
                                          if (uvSize.x == 0.0 && uvSize.y == 0.0) {
                                              discard;
                                          }
                                          vec2 uvOrig = vec2(textureCoordinates[styleOffset], textureCoordinates[styleOffset + 1]) *
                                                        uTextureFactor;
                                          float combined = textureCoordinates[styleOffset + 4];
                                          vec2 pixelSize = vec2(mod(combined, 65536.0), combined / 65536.0);
                                          ) + (fadeInPattern ? OMMShaderCode(
                                          // fadeInPattern
                                          vec4 resultColor = vec4(0.0, 0.0, 0.0, 0.0);
                                          float scalingFactorFactor = (uScalingFactor.x / uScreenPixelAsRealMeterFactor) - 1.0;
                                          vec2 spacing = pixelSize * scalingFactorFactor;
                                          vec2 totalSize = pixelSize + spacing;
                                          vec2 adjustedPixelPosition = pixelPosition + pixelSize * 0.5;
                                          vec2 uvTot = mod(adjustedPixelPosition, totalSize);

                                          int yIndex = int(mod(adjustedPixelPosition.y / totalSize.y, 2.0));

                                          if (yIndex != 0 && uvTot.y <= pixelSize.y) {
                                              uvTot.x = mod(adjustedPixelPosition.x + totalSize.x * 0.5, totalSize.x);
                                          }

                                          if (uvTot.x > pixelSize.x || uvTot.y > pixelSize.y) {
                                              if (uvTot.x > pixelSize.x && uvTot.y < pixelSize.y) {
                                                  // top right
                                                  vec2 spacingTexSize = vec2(spacing.x, spacing.x);
                                                  float relative = uvTot.y - (pixelSize.y - spacing.x) / 2.0;
                                                  if (relative > 0.0 && relative < spacing.x) {
                                                      float xPos = uvTot.x - pixelSize.x;
                                                      vec2 uv = mod(vec2(xPos, relative) / spacingTexSize + vec2(1.0, 1.0),
                                                                    vec2(1.0, 1.0));

                                                      vec2 texUv = uvOrig + uvSize * vec2(uv.x, uv.y);
                                                      vec4 texColor = texture(uTextureSampler, texUv);
                                                      resultColor = texColor;
                                                  }
                                              } else {
                                                  uvTot.x = mod(adjustedPixelPosition.x + spacing.x * 0.5, totalSize.x);
                                                  if (uvTot.x > pixelSize.x && uvTot.y > pixelSize.y) {
                                                      // bottom right
                                                      vec2 uv = mod((uvTot - pixelSize) / spacing + vec2(1.0, 1.0), vec2(1.0, 1.0));

                                                      vec2 texUv = uvOrig + uvSize * vec2(uv.x, uv.y);
                                                      vec4 texColor = texture(uTextureSampler, texUv);
                                                      resultColor = texColor;
                                                  } else {
                                                      // bottom left
                                                      vec2 spacingTexSize = vec2(spacing.y, spacing.y);
                                                      float relativeX = uvTot.x - (pixelSize.x - spacing.x) / 2.0;

                                                      if (relativeX > 0.0 && relativeX < spacing.y) {
                                                          vec2 uv = mod(vec2(relativeX, uvTot.y - pixelSize.y) / spacingTexSize +
                                                                        vec2(1.0, 1.0), vec2(1.0, 1.0));
                                                          vec2 texUv = uvOrig + uvSize * vec2(uv.x, uv.y);
                                                          vec4 texColor = texture(uTextureSampler, texUv);
                                                          resultColor = texColor;
                                                      }
                                                  }
                                              }
                                          } else {
                                              vec2 uv = mod(uvTot / pixelSize + vec2(1.0, 1.0), vec2(1.0, 1.0));
                                              vec2 texUv = uvOrig + uvSize * vec2(uv.x, uv.y);
                                              vec4 texColor = texture(uTextureSampler, texUv);
                                              resultColor = texColor;
                                          }
                                          fragmentColor = resultColor;
                                      }
            ) : OMMShaderCode(
                                            // Default pattern behavior
                                            vec2 uv = mod(vec2(mod(pixelPosition.x, pixelSize.x), mod(pixelPosition.y, pixelSize.y)) / pixelSize + vec2(1.0, 1.0), vec2(1.0, 1.0));
                                            vec2 texUv = uvOrig + uvSize * uv;
                                            vec4 color = texture(uTextureSampler, texUv);

                                            float a = color.a * opacity;
                                            fragmentColor = vec4(color.rgb * a, a);
                                      }
));
}
std::shared_ptr<ShaderProgramInterface> PolygonPatternGroup2dShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }