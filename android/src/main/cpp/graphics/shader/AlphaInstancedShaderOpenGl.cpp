/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "AlphaInstancedShaderOpenGl.h"
#include "OpenGlContext.h"
#include "OpenGlHelper.h"

AlphaInstancedShaderOpenGl::AlphaInstancedShaderOpenGl(bool projectOntoUnitSphere)
        : projectOntoUnitSphere(projectOntoUnitSphere),
          programName(projectOntoUnitSphere ? "UBMAP_AlphaInstancedUnitSphereShaderOpenGl" : "UBMAP_AlphaInstancedShaderOpenGl") {}

std::string AlphaInstancedShaderOpenGl::getProgramName() { return programName; }

void AlphaInstancedShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    BaseShaderProgramOpenGl::preRender(context);
}

void AlphaInstancedShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
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

std::string AlphaInstancedShaderOpenGl::getVertexShader() {
    return projectOntoUnitSphere ?
           OMMVersionedGlesShaderCode(320 es,
                                      uniform mat4 uvpMatrix;
                                      uniform vec4 uOriginOffset;
                                      uniform vec4 uOrigin;

                                      in vec3 vPosition;
                                      in vec2 vTexCoordinate;

                                      in vec3 aPosition;
                                      in float aRotation;
                                      in vec4 aTexCoordinate;
                                      in vec2 aScale;
                                      in float aAlpha;
                                      in vec2 aOffset;

                                      out vec2 v_texCoord;
                                      out vec4 v_texcoordInstance;
                                      out float v_alpha;

                                      void main() {
                                          float angle = aRotation * 3.14159265 / 180.0;

                                          vec4 earthCenter = uvpMatrix * vec4(0.0 - uOrigin.x, 0.0 - uOrigin.y, 0.0 - uOrigin.z, 1.0);
                                          earthCenter = earthCenter / earthCenter.w;
                                          vec4 screenPosition = uvpMatrix * (vec4(aPosition, 1.0) + uOriginOffset);
                                          screenPosition = screenPosition / screenPosition.w;
                                          float mask = float(aAlpha > 0.0) * float(screenPosition.z - earthCenter.z < 0.0);

                                          vec2 scaleOffset = vPosition.xy * aScale + aOffset;
                                          float sinAngle = sin(angle) * mask;
                                          float cosAngle = cos(angle) * mask;
                                          mat4 scaleRotateMatrix = mat4(cosAngle, -sinAngle, 0.0, 0.0,
                                                                        sinAngle, cosAngle, 0.0, 0.0,
                                                                        0.0, 0.0, 1.0, 0.0,
                                                                        scaleOffset.x, scaleOffset.y, 0.0, 1.0);

                                          gl_Position = mix(vec4(-10.0, -10.0, -10.0, -10.0),
                                                            scaleRotateMatrix * screenPosition,
                                                            mask);
                                          v_texcoordInstance = aTexCoordinate;
                                          v_texCoord = vTexCoordinate;
                                          v_alpha = aAlpha * mask;
                                      }
                                      )
    : OMMVersionedGlesShaderCode(320 es,
                                      uniform mat4 uvpMatrix;
                                      uniform vec4 uOriginOffset;

                                      in vec3 vPosition;
                                      in vec2 vTexCoordinate;

                                      in vec2 aPosition;
                                      in float aRotation;
                                      in vec4 aTexCoordinate;
                                      in vec2 aScale;
                                      in float aAlpha;
                                      in vec2 aOffset;

                                      out vec2 v_texCoord;
                                      out vec4 v_texcoordInstance;
                                      out float v_alpha;

                                      void main() {
                                          float mask = float(aAlpha > 0.0);
                                          float angle = aRotation * 3.14159265 / 180.0;
                                          float sinAngle = sin(angle) * mask;
                                          float cosAngle = cos(angle) * mask;
                                          mat4 model_matrix = mat4(
                                                  vec4(cosAngle * aScale.x, -sinAngle * aScale.x, 0.0, 0.0),
                                                  vec4(sinAngle * aScale.y, cosAngle * aScale.y, 0.0, 0.0),
                                                  vec4(0.0, 0.0, 1.0, 0.0),
                                                  vec4(aPosition + uOriginOffset.xy + aOffset, 0.0, 1.0)
                                          );

                                          mat4 matrix = uvpMatrix * model_matrix;

                                          gl_Position = mix(vec4(-10.0, -10.0, -10.0, -10.0), matrix * vec4(vPosition, 1.0), mask);
                                          v_texcoordInstance = aTexCoordinate;
                                          v_texCoord = vTexCoordinate;
                                          v_alpha = aAlpha * mask;
                                      }
    );
}


std::string AlphaInstancedShaderOpenGl::getFragmentShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      precision highp float;
                                      uniform sampler2D textureSampler;

                                      uniform vec2 textureFactor;

                                      in vec2 v_texCoord;
                                      in vec4 v_texcoordInstance;
                                      in float v_alpha;

                                      out vec4 fragmentColor;

                                      void main() {
                                          if (v_alpha == 0.0) {
                                              discard;
                                          }
                                          ) + (projectOntoUnitSphere ? OMMShaderCode(
                                              vec2 uv = (v_texcoordInstance.xy + v_texcoordInstance.zw * vec2(v_texCoord.x, v_texCoord.y)) * textureFactor;
                                          ) : OMMShaderCode(
                                              vec2 uv = (v_texcoordInstance.xy + v_texcoordInstance.zw * vec2(v_texCoord.x, (1.0 - v_texCoord.y))) * textureFactor;
                                          )) + OMMShaderCode(
                                          vec4 c = texture(textureSampler, uv);
                                          float alpha = c.a * v_alpha;
                                          fragmentColor = vec4(c.rgb * v_alpha, alpha);
                                      }
    );
}

std::shared_ptr<ShaderProgramInterface> AlphaInstancedShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }