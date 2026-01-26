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

const std::string StretchShaderOpenGl::programName = "UBMAP_StretchShaderOpenGl";

std::string StretchShaderOpenGl::getProgramName() { return programName; }

void StretchShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context, bool isScreenSpaceCoords) {
    BaseShaderProgramOpenGl::preRender(context, isScreenSpaceCoords);
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int alphaLocation = glGetUniformLocation(openGlContext->getProgram(programName), "alpha");
    glUniform1f(alphaLocation, alpha);
    int uvSpriteLocation = glGetUniformLocation(openGlContext->getProgram(programName), "uvSprite"); // we receive the texture/uv-values upside down, the Quad already compensates for that
    glUniform4f(uvSpriteLocation, (float) stretchShaderInfo.uv.x, (float) stretchShaderInfo.uv.y, (float) stretchShaderInfo.uv.width, (float) stretchShaderInfo.uv.height);
    int scalesLocation = glGetUniformLocation(openGlContext->getProgram(programName), "scales");
    glUniform2f(scalesLocation, stretchShaderInfo.scaleX, stretchShaderInfo.scaleY);
    int stretchXLocation = glGetUniformLocation(openGlContext->getProgram(programName), "stretchX");
    glUniform4f(stretchXLocation, stretchShaderInfo.stretchX0Begin, stretchShaderInfo.stretchX0End, stretchShaderInfo.stretchX1Begin, stretchShaderInfo.stretchX1End);
    int stretchYLocation = glGetUniformLocation(openGlContext->getProgram(programName), "stretchY");
    glUniform4f(stretchYLocation, stretchShaderInfo.stretchY0Begin, stretchShaderInfo.stretchY0End, stretchShaderInfo.stretchY1Begin, stretchShaderInfo.stretchY1End);
}

void StretchShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
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

void StretchShaderOpenGl::updateAlpha(float value) { alpha = value; }

void StretchShaderOpenGl::updateStretchInfo(const StretchShaderInfo &info) {
    stretchShaderInfo = info;
}

std::string StretchShaderOpenGl::getFragmentShader() {
    return OMMVersionedGlesShaderCode(320 es, 300 es,
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
                                          // All computed in normalized uv space of this single sprite
                                          vec2 texCoordNorm = (v_texcoord - uvSprite.xy) / uvSprite.zw;
                                          vec2 texCoordNorm2 = (v_texcoord - uvSprite.xy) / uvSprite.zw;

                                          // X
                                          if (stretchX.x != stretchX.y) {
                                              float sumStretchedX = (stretchX.y - stretchX.x) + (stretchX.w - stretchX.z);
                                              float scaleStretchX = (sumStretchedX * scales.x) / (1.0 - (scales.x - sumStretchedX * scales.x));

                                              float totalOffset = min(texCoordNorm.x, stretchX.x) // offsetPre0Unstretched
                                                      + (clamp(texCoordNorm.x, stretchX.x, stretchX.y) - stretchX.x) / scaleStretchX // offset0Stretched
                                                      + (clamp(texCoordNorm.x, stretchX.y, stretchX.z) - stretchX.y) // offsetPre1Unstretched
                                                      + (clamp(texCoordNorm.x, stretchX.z, stretchX.w) - stretchX.z) / scaleStretchX // offset1Stretched
                                                      + (clamp(texCoordNorm.x, stretchX.w, 1.0) - stretchX.w); // offsetPost1Unstretched
                                              texCoordNorm.x = totalOffset * scales.x;
                                          }

                                          // Y
                                          if (stretchY.x != stretchY.y) {
                                              float sumStretchedY = (stretchY.y - stretchY.x) + (stretchY.w - stretchY.z);
                                              float scaleStretchY = (sumStretchedY * scales.y) / (1.0 - (scales.y - sumStretchedY * scales.y));

                                              float totalOffset = min(texCoordNorm.y, stretchY.x) // offsetPre0Unstretched
                                                      + (clamp(texCoordNorm.y, stretchY.x, stretchY.y) - stretchY.x) / scaleStretchY // offset0Stretched
                                                      + (clamp(texCoordNorm.y, stretchY.y, stretchY.z) - stretchY.y) // offsetPre1Unstretched
                                                      + (clamp(texCoordNorm.y, stretchY.z, stretchY.w) - stretchY.z) / scaleStretchY // offset1Stretched
                                                      + (clamp(texCoordNorm.y, stretchY.w, 1.0) - stretchY.w); // offsetPost1Unstretched
                                              texCoordNorm.y = totalOffset * scales.y;
                                          }

                                          // remap final normalized uv to sprite atlas coordinates
                                          texCoordNorm = texCoordNorm * uvSprite.zw + uvSprite.xy;

                                          vec4 color = texture(textureSampler, texCoordNorm);
                                          float a = color.a * alpha;
                                          fragmentColor = vec4(color.rgb * a, a);
                                      }
    );
}


std::shared_ptr<ShaderProgramInterface> StretchShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }
