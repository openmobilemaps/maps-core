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

const int TextInstancedShaderOpenGl::BYTE_SIZE_TEXT_STYLES = 4 * sizeof(GLfloat); // Ensure consistency with style buffer creation and shader code below. CAUTION! Padded to 16 bytes due to std140!
const int TextInstancedShaderOpenGl::MAX_NUM_TEXT_STYLES = 16384 / BYTE_SIZE_TEXT_STYLES; // guaranteed by GL ES 3.2 min 16384 bytes

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

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(programName);
    glUseProgram(program);

    int aspectRatioHandle = glGetUniformLocation(program, "uAspectRatio");
    glUniform1f(aspectRatioHandle, openGlContext->getAspectRatio());
}

std::string TextInstancedShaderOpenGl::getVertexShader() {
    return projectOntoUnitSphere ?
    OMMVersionedGlesShaderCode(320 es,
                               uniform mat4 uvpMatrix;
                               uniform mat4 umMatrix;
                               uniform vec4 uOriginOffset;
                               uniform vec4 uOrigin;
                               uniform float uAspectRatio;

                               in vec4 vPosition;
                               in vec2 texCoordinate;

                               in vec2 aPosition;
                               in vec3 aReferencePosition;
                               in vec4 aTexCoordinate;
                               in vec2 aScale;
                               in float aAlpha;
                               in float aRotation;
                               in uint aStyleIndex;

                               out vec2 v_texCoord;
                               out vec4 v_texCoordInstance;
                               out flat highp uint vStyleIndex;
                               out float v_alpha;

                               void main() {
                                   float angle = aRotation * 3.14159265 / 180.0;

                                   vec4 newVertex = umMatrix * vec4(aReferencePosition + uOriginOffset.xyz, 1.0);

                                   vec4 earthCenter = uvpMatrix * vec4(0.0 - uOrigin.x, 0.0 - uOrigin.y, 0.0 - uOrigin.z, 1.0);
                                   earthCenter = earthCenter / earthCenter.w;
                                   vec4 screenPosition = uvpMatrix * newVertex;
                                   screenPosition = screenPosition / screenPosition.w;

                                   // Apply non-uniform scaling first
                                   vec2 pScaled = vec2(vPosition.x * aScale.x, vPosition.y * aScale.y);

                                   // Apply rotation after scaling
                                   float sinAngle = sin(angle);
                                   float cosAngle = cos(angle);
                                   vec2 pRot = vec2(pScaled.x * cosAngle - pScaled.y * sinAngle,
                                                    (pScaled.x * sinAngle + pScaled.y * cosAngle) * uAspectRatio);

                                   gl_Position = vec4(screenPosition.xy + aPosition.xy + pRot, 0.0, 1.0);
                                   v_texCoordInstance = aTexCoordinate;
                                   v_texCoord = texCoordinate;
                                   vStyleIndex = aStyleIndex;
                                   v_alpha = aAlpha;
                                   if (screenPosition.z - earthCenter.z > 0.0) {
                                       v_alpha = 0.0;
                                   }
                               }
                           )
    : OMMVersionedGlesShaderCode(320 es,
                                  uniform mat4 uvpMatrix;
                                  uniform vec4 uOriginOffset;

                                  in vec4 vPosition;
                                  in vec2 texCoordinate;

                                  in vec2 aPosition;
                                  in vec4 aTexCoordinate;
                                  in vec2 aScale;
                                  in float aAlpha;
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
                                              vec4(aPosition.xy + uOriginOffset.xy, 1.0, 1.0)
                                      );

                                      mat4 matrix = uvpMatrix * model_matrix;

                                      gl_Position = matrix * vPosition;
                                      v_texCoordInstance = aTexCoordinate;
                                      v_texCoord = texCoordinate;
                                      vStyleIndex = aStyleIndex;
                                      v_alpha = aAlpha;
                                  }
    );
}

std::string TextInstancedShaderOpenGl::getFragmentShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      precision highp float;
                                              struct TextStyle {
                                                  float colorRGBA;
                                                  float haloColorRGBA;
                                                  float haloWidth;
                                                  float haloBlur;
                                              };
                                              layout(std140, binding = 0) uniform TextStyleCollection {
                                                  TextStyle styles[) + std::to_string(MAX_NUM_TEXT_STYLES) + OMMShaderCode(];
                                              } uTextStyles;

                                              uniform sampler2D textureSampler;
                                              uniform vec2 textureFactor;
                                              uniform float isHalo; // 0.0 = false, 1.0 = true

                                              in vec2 v_texCoord;
                                              in vec4 v_texCoordInstance;
                                              in flat highp uint vStyleIndex;
                                              in float v_alpha;

                                              out vec4 fragmentColor;

                                              void main() {
                                                  highp int styleIndex = int(vStyleIndex);

                                                  highp uint colorBits = 0u;
                                                  if (bool(isHalo)) {
                                                      colorBits = floatBitsToUint(uTextStyles.styles[styleIndex].haloColorRGBA);
                                                  } else {
                                                      colorBits = floatBitsToUint(uTextStyles.styles[styleIndex].colorRGBA);
                                                  }
                                                  vec4 color = vec4(
                                                          float(colorBits >> 24), // r
                                                          float((colorBits >> 16) & 0xFFu), // g
                                                          float((colorBits >> 8) & 0xFFu), // b
                                                          float(colorBits & 0xFFu) // a
                                                  ) / 255.0;
                                                  color.a *= v_alpha;

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
                                                      float haloWidth = uTextStyles.styles[styleIndex].haloWidth;
                                                      float halfHaloBlur = 0.5 * uTextStyles.styles[styleIndex].haloBlur;

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
