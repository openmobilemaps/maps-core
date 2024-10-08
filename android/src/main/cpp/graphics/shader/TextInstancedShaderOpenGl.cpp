/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "TextInstancedShaderOpenGl.h"
#include "OpenGlContext.h"
#include "OpenGlHelper.h"


TextInstancedShaderOpenGl::TextInstancedShaderOpenGl(bool projectOntoUnitSphere)
        : projectOntoUnitSphere(projectOntoUnitSphere),
          programName(projectOntoUnitSphere ? "UBMAP_TextInstancedUnitSphereShaderOpenGl" : "UBMAP_TextInstancedShaderOpenGl") {}

std::string TextInstancedShaderOpenGl::getProgramName() { return programName; }

void TextInstancedShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
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

void TextInstancedShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    BaseShaderProgramOpenGl::preRender(context);
}

std::string TextInstancedShaderOpenGl::getVertexShader() {
    return projectOntoUnitSphere ?
    OMMVersionedGlesShaderCode(320 es,
                               uniform mat4 uvpMatrix;
                               uniform mat4 umMatrix;

                                       in vec4 vPosition;
                                       in vec2 texCoordinate;

                                       in vec2 aPosition;
                                       in vec2 aReferencePosition;
                                       in vec4 aTexCoordinate;
                                       in vec2 aScale;
                                       in float aRotation;
                                       in uint aStyleIndex;

                                       out vec2 v_texCoord;
                                       out vec4 v_texCoordInstance;
                                       out flat highp uint vStyleIndex;
                                       out float v_alpha;

                                       void main() {
                                           float angle = aRotation * 3.14159265 / 180.0;

                                           vec4 newVertex = umMatrix * vec4(aReferencePosition, 1.0, 1.0);

                                           vec4 earthCenter = uvpMatrix * vec4(0.0, 0.0, 0.0, 1.0);
                                           earthCenter = earthCenter / earthCenter.w;
                                           vec4 screenPosition = uvpMatrix * vec4(newVertex.z * sin(newVertex.y) * cos(newVertex.x),
                                                                                  newVertex.z * cos(newVertex.y),
                                                                                  -newVertex.z * sin(newVertex.y) * sin(newVertex.x),
                                                                                  1.0);
                                           screenPosition = screenPosition / screenPosition.w;

                                           vec2 size = (vPosition.xy) * aScale;

                                           mat4 screenMatrix = mat4(
                                                   cos(angle), -sin(angle), 0.0, 0.0,
                                                   sin(angle), cos(angle), 0.0, 0.0,
                                                   0.0, 0.0, 0.0, 0.0,
                                                   size.x + aPosition.x, size.y + aPosition.y, 0.0, 1.0
                                           );

                                           gl_Position = screenMatrix * screenPosition;
                                           v_texCoordInstance = aTexCoordinate;
                                           v_texCoord = texCoordinate;
                                           vStyleIndex = aStyleIndex;
                                           v_alpha = 1.0;
                                           if (screenPosition.z - earthCenter.z > 0.0) {
                                               v_alpha = 0.0;
                                           }
                                       }
                               )
    : OMMVersionedGlesShaderCode(320 es,
                                      uniform mat4 uvpMatrix;

                                      in vec4 vPosition;
                                      in vec2 texCoordinate;

                                      in vec2 aPosition;
                                      in vec4 aTexCoordinate;
                                      in vec2 aScale;
                                      in float aRotation;
                                      in uint aStyleIndex;

                                      out vec2 v_texCoord;
                                      out vec4 v_texCoordInstance;
                                      out flat highp uint vStyleIndex;
                                      out float v_alpha;

                                      void main() {
                                          float angle = aRotation * 3.14159265 / 180.0;

                                          mat4 model_matrix = mat4(
                                                  vec4(cos(angle) * aScale.x, -sin(angle) * aScale.x, 0.0, 0.0),
                                                  vec4(sin(angle) * aScale.y, cos(angle) * aScale.y, 0.0, 0.0),
                                                  vec4(0.0, 0.0, 1.0, 0.0),
                                                  vec4(aPosition.x, aPosition.y, 1.0, 1.0)
                                          );

                                          mat4 matrix = uvpMatrix * model_matrix;

                                          gl_Position = matrix * vPosition;
                                          v_texCoordInstance = aTexCoordinate;
                                          v_texCoord = texCoordinate;
                                          vStyleIndex = aStyleIndex;
                                          v_alpha = 1.0;
                                      }
    );
}

std::string TextInstancedShaderOpenGl::getFragmentShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      precision highp float;
                                              layout(std430, binding = 0) buffer textInstancedStyleBuffer {
                                                  float styles[]; // vec4 color; vec4 haloColor; float haloWidth; float haloBlur;
                                              };

                                              uniform sampler2D textureSampler;
                                              uniform vec2 textureFactor;
                                              uniform float isHalo; // 0.0 = false, 1.0 = true

                                              in vec2 v_texCoord;
                                              in vec4 v_texCoordInstance;
                                              in flat highp uint vStyleIndex;
                                              in float v_alpha;

                                              out vec4 fragmentColor;

                                              void main() {
                                                  highp int styleOffset = int(vStyleIndex) * 10;

                                                  highp int colorOffset = int(isHalo) * 4 + styleOffset; // fill/halo color switch
                                                  vec4 color = vec4(styles[colorOffset + 0], styles[colorOffset + 1],
                                                               styles[colorOffset + 2], styles[colorOffset + 3]);

                                                  if (color.a == 0.0) {
                                                      discard;
                                                  }

                                                  ) + (projectOntoUnitSphere ? OMMShaderCode(
                                                          vec2 uv = (v_texCoordInstance.xy + v_texCoordInstance.zw * vec2(v_texCoord.x, v_texCoord.y)) * textureFactor;
                                                  ) : OMMShaderCode(
                                                          vec2 uv = (v_texCoordInstance.xy + v_texCoordInstance.zw * vec2(v_texCoord.x, (1.0 - v_texCoord.y))) * textureFactor;
                                                  )) + OMMShaderCode(

                                                  vec4 dist = texture(textureSampler, uv);

                                                  float median = max(min(dist.r, dist.g), min(max(dist.r, dist.g), dist.b)) / dist.a;
                                                  float w = fwidth(median);

                                                  float fillStart = 0.5 - w;
                                                  float fillEnd = 0.5 + w;

                                                  float innerFallOff = smoothstep(fillStart, fillEnd, median);

                                                  float edgeAlpha = 0.0;

                                                  if(bool(isHalo)) {
                                                      float haloWidth = styles[styleOffset + 8];
                                                      float halfHaloBlur = 0.5 * styles[styleOffset + 9];

                                                      if (haloWidth == 0.0 && halfHaloBlur == 0.0) {
                                                          discard;
                                                      }

                                                      float start = max(0.0, fillStart - (haloWidth + halfHaloBlur));
                                                      float end = max(0.0, fillStart - max(0.0, haloWidth - halfHaloBlur));

                                                      float sideSwitch = step(median, end);
                                                      float outerFallOff = smoothstep(start, end, median);

                                                      // Combination of blurred outer falloff and inverse inner fill falloff
                                                      edgeAlpha = (sideSwitch * outerFallOff + (1.0 - sideSwitch) * (1.0 - innerFallOff)) * color.a;
                                                  } else {
                                                      edgeAlpha = innerFallOff * color.a;
                                                  }

                                                  fragmentColor = color;
                                                  fragmentColor.a = 1.0;
                                                  fragmentColor *= edgeAlpha;
                                              }
    );
}
std::shared_ptr<ShaderProgramInterface> TextInstancedShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }
