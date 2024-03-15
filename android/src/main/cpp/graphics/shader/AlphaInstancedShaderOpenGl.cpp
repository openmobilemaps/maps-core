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

const std::string AlphaInstancedShaderOpenGl::programName = "UBMAP_AlphaInstancedShaderOpenGl";

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
    return OMMVersionedGlesShaderCode(320 es,
                                      uniform mat4 uvpMatrix;

                                      in vec3 vPosition;
                                      in vec2 vTexCoordinate;

                                      in vec2 aPosition;
                                      in float aRotation;
                                      in vec4 aTexCoordinate;
                                      in vec2 aScale;
                                      in float aAlpha;

                                      out vec2 v_texCoord;
                                      out vec4 v_texcoordInstance;
                                      out float v_alpha;

                                      void main() {
                                          float angle = aRotation * 3.14159265 / 180.0;
                                          mat4 model_matrix = mat4(
                                                  vec4(cos(angle) * aScale.x, -sin(angle) * aScale.x, 0.0, 0.0),
                                                  vec4(sin(angle) * aScale.y, cos(angle) * aScale.y, 0.0, 0.0),
                                                  vec4(0.0, 0.0, 1.0, 0.0),
                                                  vec4(aPosition.x, aPosition.y, 1.0, 1)
                                          );

                                          mat4 matrix = uvpMatrix * model_matrix;

                                          gl_Position = matrix * vec4(vPosition, 1.0);
                                          v_texcoordInstance = aTexCoordinate;
                                          v_texCoord = vTexCoordinate;
                                          v_alpha = aAlpha;
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
                                          vec2 uv = (v_texcoordInstance.xy + v_texcoordInstance.zw * vec2(v_texCoord.x, (1.0 - v_texCoord.y))) * textureFactor;
                                          vec4 c = texture(textureSampler, uv);
                                          float alpha = c.a * v_alpha;
                                          fragmentColor = vec4(c.rgb * v_alpha, alpha);
                                      }
    );
}

std::shared_ptr<ShaderProgramInterface> AlphaInstancedShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }