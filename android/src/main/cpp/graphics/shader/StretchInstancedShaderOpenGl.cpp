/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "StretchInstancedShaderOpenGl.h"
#include "OpenGlContext.h"
#include "OpenGlHelper.h"

StretchInstancedShaderOpenGl::StretchInstancedShaderOpenGl(bool projectOntoUnitSphere)
        : projectOntoUnitSphere(projectOntoUnitSphere),
          programName(projectOntoUnitSphere ? "UBMAP_StretchInstancedUnitSphereShaderOpenGl" : "UBMAP_StretchInstancedShaderOpenGl") {}

std::string StretchInstancedShaderOpenGl::getProgramName() { return programName; }

void StretchInstancedShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    BaseShaderProgramOpenGl::preRender(context);
}

void StretchInstancedShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
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

std::string StretchInstancedShaderOpenGl::getVertexShader() {
    return projectOntoUnitSphere ?
           OMMVersionedGlesShaderCodeWithFrameUBO(320 es,
                                      uniform vec4 uOriginOffset;

                                      in vec4 vPosition;
                                      in vec2 texCoordinate;

                                      in vec3 aPosition;
                                      in vec4 aTexCoordinate;
                                      in vec2 aScale;
                                      in float aRotation;
                                      in float aAlpha;
                                      in vec2 aStretchScales;
                                      in vec4 aStretchX;
                                      in vec4 aStretchY;

                                      out vec2 v_texCoord;
                                      out vec4 v_texCoordInstance;
                                      out float v_alpha;
                                      out vec2 v_stretchScales;
                                      out vec4 v_stretchX;
                                      out vec4 v_stretchY;

                                      void main() {
                                          float angle = aRotation * 3.14159265 / 180.0;

                                          vec4 earthCenter = uFrameUniforms.vpMatrix * vec4(0.0, 0.0, 0.0, 1.0);
                                          earthCenter = earthCenter / earthCenter.w;
                                          vec4 screenPosition = uFrameUniforms.vpMatrix * (vec4(aPosition, 1.0) + uOriginOffset);
                                          screenPosition = screenPosition / screenPosition.w;
                                          float mask = float(aAlpha > 0.0) * float(screenPosition.z - earthCenter.z < 0.0);

                                          vec2 scalePosition = vPosition.xy * aScale;
                                          float sinAngle = sin(angle) * mask;
                                          float cosAngle = cos(angle) * mask;
                                          mat4 scaleRotateMatrix = mat4(cosAngle, -sinAngle, 0.0, 0.0,
                                                                        sinAngle, cosAngle, 0.0, 0.0,
                                                                        0.0, 0.0, 1.0, 0.0,
                                                                        scalePosition.x, scalePosition.y, 0.0, 1.0);

                                          gl_Position = mix(vec4(-10.0, -10.0, -10.0, -10.0),
                                                            scaleRotateMatrix * screenPosition,
                                                            mask);
                                          v_texCoordInstance = aTexCoordinate;
                                          v_texCoord = texCoordinate;
                                          v_alpha = aAlpha * mask;
                                          v_stretchScales = aStretchScales;
                                          v_stretchX = aStretchX;
                                          v_stretchY = aStretchY;
                                      }
                              )
    : OMMVersionedGlesShaderCodeWithFrameUBO(320 es,
                                      uniform vec4 uOriginOffset;

                                      in vec4 vPosition;
                                      in vec2 texCoordinate;

                                      in vec2 aPosition;
                                      in vec4 aTexCoordinate;
                                      in vec2 aScale;
                                      in float aRotation;
                                      in float aAlpha;
                                      in vec2 aStretchScales;
                                      in vec4 aStretchX;
                                      in vec4 aStretchY;

                                      out vec2 v_texCoord;
                                      out vec4 v_texCoordInstance;
                                      out float v_alpha;
                                      out vec2 v_stretchScales;
                                      out vec4 v_stretchX;
                                      out vec4 v_stretchY;

                                      void main() {
                                          float mask = float(aAlpha > 0.0);
                                          float angle = aRotation * 3.14159265 / 180.0;
                                          float sinAngle = sin(angle) * mask;
                                          float cosAngle = cos(angle) * mask;
                                          mat4 model_matrix = mat4(
                                                  vec4(cosAngle * aScale.x, -sinAngle * aScale.x, 0, 0),
                                                  vec4(sinAngle * aScale.y, cosAngle * aScale.y, 0, 0),
                                                  vec4(0, 0, 1, 0),
                                                  vec4(aPosition + uOriginOffset.xy, 0.0, 1.0)
                                          );

                                          mat4 matrix = uFrameUniforms.vpMatrix * model_matrix;

                                          gl_Position = mix(vec4(-10.0, -10.0, -10.0, -10.0), matrix * vPosition, mask);
                                          v_texCoordInstance = aTexCoordinate;
                                          v_texCoord = texCoordinate;
                                          v_alpha = aAlpha * mask;
                                          v_stretchScales = aStretchScales;
                                          v_stretchX = aStretchX;
                                          v_stretchY = aStretchY;
                                      }
    );
}


std::string StretchInstancedShaderOpenGl::getFragmentShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      precision mediump float;
                                      uniform sampler2D textureSampler;

                                      uniform vec2 textureFactor;

                                      in vec2 v_texCoord;
                                      in vec4 v_texCoordInstance;
                                      in float v_alpha;
                                      in vec2 v_stretchScales; // scale x, y
                                      in vec4 v_stretchX; // x0 begin, x0 end, x1 begin, x1 end
                                      in vec4 v_stretchY;

                                      out vec4 fragmentColor;

                                      void main() {
                                          if (v_alpha == 0.0) {
                                              discard;
                                          }

                                          vec4 adjTexCoordIns = vec4(v_texCoordInstance.x, v_texCoordInstance.y + v_texCoordInstance.w, v_texCoordInstance.z, -v_texCoordInstance.w) * textureFactor.xyxy;
                                          vec2 texCoordNorm = v_texCoord;

                                          // X
                                          if (v_stretchX.x != v_stretchX.y) {
                                              float sumStretchedX = (v_stretchX.y - v_stretchX.x) + (v_stretchX.w - v_stretchX.z);
                                              float scaleStretchX = (sumStretchedX * v_stretchScales.x) / (1.0 - (v_stretchScales.x - sumStretchedX * v_stretchScales.x));

                                              float totalOffset = min(texCoordNorm.x, v_stretchX.x) // offsetPre0Unstretched
                                                                        + (clamp(texCoordNorm.x, v_stretchX.x, v_stretchX.y) - v_stretchX.x) / scaleStretchX // offset0Stretched
                                                                        + (clamp(texCoordNorm.x, v_stretchX.y, v_stretchX.z) - v_stretchX.y) // offsetPre1Unstretched
                                                                        + (clamp(texCoordNorm.x, v_stretchX.z, v_stretchX.w) - v_stretchX.z) / scaleStretchX // offset1Stretched
                                                                        + (clamp(texCoordNorm.x, v_stretchX.w, 1.0) - v_stretchX.w); // offsetPost1Unstretched
                                              texCoordNorm.x = totalOffset * v_stretchScales.x;
                                          }

                                          // Y
                                          if (v_stretchY.x != v_stretchY.y) {
                                              float sumStretchedY = (v_stretchY.y - v_stretchY.x) + (v_stretchY.w - v_stretchY.z);
                                              float scaleStretchY = (sumStretchedY * v_stretchScales.y) / (1.0 - (v_stretchScales.y - sumStretchedY * v_stretchScales.y));

                                              float totalOffset = min(texCoordNorm.y, v_stretchY.x) // offsetPre0Unstretched
                                                                        + (clamp(texCoordNorm.y, v_stretchY.x, v_stretchY.y) - v_stretchY.x) / scaleStretchY // offset0Stretched
                                                                        + (clamp(texCoordNorm.y, v_stretchY.y, v_stretchY.z) - v_stretchY.y) // offsetPre1Unstretched
                                                                        + (clamp(texCoordNorm.y, v_stretchY.z, v_stretchY.w) - v_stretchY.z) / scaleStretchY // offset1Stretched
                                                                        + (clamp(texCoordNorm.y, v_stretchY.w, 1.0) - v_stretchY.w); // offsetPost1Unstretched
                                              texCoordNorm.y = totalOffset * v_stretchScales.y;
                                          }

                                          // remap final normalized uv to sprite atlas coordinates
                                          texCoordNorm = adjTexCoordIns.xy + adjTexCoordIns.zw * texCoordNorm;
                                          vec4 color = texture(textureSampler, texCoordNorm);
                                          float a = color.a * v_alpha;
                                          fragmentColor = vec4(color.rgb * a, a);
                                      }
    );
}

std::shared_ptr<ShaderProgramInterface> StretchInstancedShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }