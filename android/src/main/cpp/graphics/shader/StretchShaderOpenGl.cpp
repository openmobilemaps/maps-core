/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "StretchShaderOpenGl.h"
#include "OpenGlContext.h"
#include "OpenGlHelper.h"

std::string StretchShaderOpenGl::getProgramName() { return "UBMAP_StretchShaderOpenGl"; }

void StretchShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int alphaLocation = glGetUniformLocation(openGlContext->getProgram(getProgramName()), "alpha");
    glUniform1f(alphaLocation, alpha);
    int uvSpriteLocation = glGetUniformLocation(openGlContext->getProgram(getProgramName()), "uvSprite");
    glUniform4f(uvSpriteLocation, (float) stretchShaderInfo.uv.x, (float) stretchShaderInfo.uv.y, (float) stretchShaderInfo.uv.width, (float) stretchShaderInfo.uv.height);
    int scalesLocation = glGetUniformLocation(openGlContext->getProgram(getProgramName()), "scales");
    glUniform2f(scalesLocation, stretchShaderInfo.scaleX, stretchShaderInfo.scaleY);
    int stretchXLocation = glGetUniformLocation(openGlContext->getProgram(getProgramName()), "stretchX");
    glUniform4f(stretchXLocation, stretchShaderInfo.stretchX0Begin, stretchShaderInfo.stretchX0End, stretchShaderInfo.stretchX1Begin, stretchShaderInfo.stretchX1End);
    int stretchYLocation = glGetUniformLocation(openGlContext->getProgram(getProgramName()), "stretchY");
    glUniform4f(stretchYLocation, stretchShaderInfo.stretchX0Begin, stretchShaderInfo.stretchX0End, stretchShaderInfo.stretchX1Begin, stretchShaderInfo.stretchX1End);
}

void StretchShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
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

void StretchShaderOpenGl::updateAlpha(float value) { alpha = value; }

void StretchShaderOpenGl::updateStretchInfo(const StretchShaderInfo &info) {
    stretchShaderInfo = info;
}

std::string StretchShaderOpenGl::getFragmentShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      precision highp float;
                                      uniform sampler2D textureSampler;
                                      uniform float alpha;
                                      //layout(std140) uniform StretchInfoBlock TODO: when shader are properly disposed of on the GLThread
                                      //    {
                                      uniform vec4 uvSprite; // symbol uv origin x, y; width, height
                                      uniform vec2 scales; // scale x, y
                                      uniform vec4 stretchX; // x0 begin, x0 end, x1 begin, x1 end
                                      uniform vec4 stretchY;
                                      //    };

                                      in vec2 v_texcoord;
                                      out vec4 fragmentColor;

                                      void main() {
                                          vec2 normalizedUV = (v_texcoord - uvSprite.xy) / uvSprite.zw * vec2(scales.x, scales.y);

                                          vec2 mappedUV = v_texcoord;

                                          float countRegionX = float(stretchX.x != stretchX.y) + float(stretchX.z != stretchX.w);
                                          float countRegionY = float(stretchY.x != stretchY.y) + float(stretchY.z != stretchY.w);

                                          // X
                                          if (countRegionX > 0.0) {
                                              float stretchedRegionX = stretchX.y - stretchX.x + stretchX.w - stretchX.z;

                                              float notStretchedRegionX = 1.0 - stretchedRegionX;

                                              float overflowRegion0X = (scales.x - notStretchedRegionX) / countRegionX;
                                              float overflowRegion1X = countRegionX == 2.0 ? overflowRegion0X : 0.0;

                                              float startXRegion0 = stretchX.x;
                                              float endXRegion0 = startXRegion0 + overflowRegion0X;
                                              float startXRegion1 = endXRegion0 + (stretchX.z - stretchX.y);
                                              float endXRegion1 = startXRegion1 + overflowRegion1X;
                                              float endX = scales.x;

                                              if (normalizedUV.x < startXRegion0) {;
                                                  mappedUV.x = (normalizedUV.x / startXRegion0) * stretchX.x * uvSprite.z + uvSprite.x;
                                              } else if (normalizedUV.x >= startXRegion0 && normalizedUV.x < endXRegion0) {
                                                  mappedUV.x = ((((normalizedUV.x - startXRegion0)  / (endXRegion0 - startXRegion0)) * (stretchX.y - stretchX.y)) + stretchX.y) * uvSprite.z + uvSprite.x;
                                              } else if (normalizedUV.x >= endXRegion0 && normalizedUV.x < startXRegion1) {
                                                  mappedUV.x = ((((normalizedUV.x - startXRegion1)  / (endXRegion1 - startXRegion1)) * (1.0 - stretchX.w)) + stretchX.w) * uvSprite.z + uvSprite.x;
                                              } else if (normalizedUV.x >= startXRegion1 && normalizedUV.x < endXRegion1) {
                                                  mappedUV.x = ((((normalizedUV.x - startXRegion1)  / (endXRegion1 - startXRegion1)) * (stretchX.w - stretchX.w)) + stretchX.w) * uvSprite.z + uvSprite.x;
                                              } else {
                                                  mappedUV.x = ((((normalizedUV.x - endXRegion1)  / (endX - endXRegion1)) * (1.0 - stretchX.w)) + stretchX.w) * uvSprite.z + uvSprite.x;
                                              }
                                          }


                                          // Y
                                          if (countRegionY > 0.0) {
                                              float stretchedRegionY = stretchY.y - stretchY.x + stretchY.w - stretchY.z;

                                              float notStretchedRegionY = 1.0 - stretchedRegionY;

                                              float overflowRegion0Y = (scales.y - notStretchedRegionY) / countRegionY;
                                              float overflowRegion1Y = countRegionY == 2.0 ? overflowRegion0Y : 0.0;

                                              float startYRegion0 = stretchY.x;
                                              float endYRegion0 = startYRegion0 + overflowRegion0Y;
                                              float startYRegion1 = endYRegion0 + (stretchY.z - stretchY.y);
                                              float endYRegion1 = startYRegion1 + overflowRegion1Y;
                                              float endY = scales.y;

                                              if (normalizedUV.y < startYRegion0) {
                                                  mappedUV.y = (normalizedUV.y / startYRegion0) * stretchY.x * uvSprite.w + uvSprite.y;
                                              } else if (normalizedUV.y >= startYRegion0 && normalizedUV.y < endYRegion0) {
                                                  mappedUV.y = ((((normalizedUV.y - startYRegion0)  / (endYRegion0 - startYRegion0)) * (stretchY.y - stretchY.y)) + stretchY.y) * uvSprite.w + uvSprite.y;
                                              } else if (normalizedUV.y >= endYRegion0 && normalizedUV.y < startYRegion1) {
                                                  mappedUV.y = ((((normalizedUV.y - startYRegion1)  / (endYRegion1 - startYRegion1)) * (1.0 - stretchY.w)) + stretchY.w) * uvSprite.w + uvSprite.y;
                                              } else if (normalizedUV.y >= startYRegion1 && normalizedUV.y < endYRegion1) {
                                                  mappedUV.y = ((((normalizedUV.y - startYRegion1)  / (endYRegion1 - startYRegion1)) * (stretchY.w - stretchY.w)) + stretchY.w) * uvSprite.w + uvSprite.y;
                                              } else {
                                                  mappedUV.y = ((((normalizedUV.y - endYRegion1)  / (endY - endYRegion1)) * (1.0 - stretchY.w)) + stretchY.w) * uvSprite.w + uvSprite.y;
                                              }
                                          }

                                          vec4 color = texture(textureSampler, mappedUV);
                                          float a = color.a * alpha;
                                          fragmentColor = vec4(color.rgb * a, a);
                                      }
    );
}


std::shared_ptr<ShaderProgramInterface> StretchShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }