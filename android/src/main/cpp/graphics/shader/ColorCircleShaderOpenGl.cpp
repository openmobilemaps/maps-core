/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "ColorCircleShaderOpenGl.h"
#include "OpenGlContext.h"
#include "OpenGlHelper.h"
#include <algorithm>

ColorCircleShaderOpenGl::ColorCircleShaderOpenGl(bool projectOntoUnitSphere)
        : programName(projectOntoUnitSphere ? "UBMAP_ColorCircleShaderUnitSphereOpenGl" : "UBMAP_ColorCircleShaderOpenGl")
        , projectOntoUnitSphere(projectOntoUnitSphere) {}

std::string ColorCircleShaderOpenGl::getProgramName() { return programName; }

void ColorCircleShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
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

    openGlContext->storeProgram(programName, program);
}

void ColorCircleShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context, bool isScreenSpaceCoords) {
    BaseShaderProgramOpenGl::preRender(context, isScreenSpaceCoords);
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(programName);

    int fillColorHandle = glGetUniformLocation(program, "fillColor");
    int strokeColorHandle = glGetUniformLocation(program, "strokeColor");
    int innerRadiusHandle = glGetUniformLocation(program, "innerRadius");
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        glUniform4fv(fillColorHandle, 1, &fillColor[0]);
        glUniform4fv(strokeColorHandle, 1, &strokeColor[0]);
        glUniform1f(innerRadiusHandle, innerRadius);
    }

    if (projectOntoUnitSphere) {
        auto viewportSize = context->getViewportSize();
        int viewportSizeHandle = glGetUniformLocation(program, "viewportSize");
        glUniform2f(viewportSizeHandle, std::max(1, viewportSize.x), std::max(1, viewportSize.y));
    }
}

void ColorCircleShaderOpenGl::setColor(float red, float green, float blue, float alpha) {
    std::lock_guard<std::mutex> lock(dataMutex);
    fillColor[0] = red;
    fillColor[1] = green;
    fillColor[2] = blue;
    fillColor[3] = alpha;
}

void ColorCircleShaderOpenGl::setCircleStyle(float fillRed, float fillGreen, float fillBlue, float fillAlpha,
                                             float strokeRed, float strokeGreen, float strokeBlue, float strokeAlpha,
                                             float innerRadius) {
    std::lock_guard<std::mutex> lock(dataMutex);
    fillColor[0] = fillRed;
    fillColor[1] = fillGreen;
    fillColor[2] = fillBlue;
    fillColor[3] = fillAlpha;
    strokeColor[0] = strokeRed;
    strokeColor[1] = strokeGreen;
    strokeColor[2] = strokeBlue;
    strokeColor[3] = strokeAlpha;
    this->innerRadius = std::max(0.0f, std::min(1.0f, innerRadius));
}

std::string ColorCircleShaderOpenGl::getVertexShader() {
    if (projectOntoUnitSphere) {
        return OMMVersionedGlesShaderCodeWithFrameUBO(320 es, 300 es,
                                      uniform vec4 uOriginOffset;
                                      uniform vec2 viewportSize;
                                      in vec3 vPosition;
                                      in vec2 texCoordinate;
                                      out vec2 v_texcoord;

                                      void main() {
                                          vec4 earthCenter = uFrameUniforms.vpMatrix * vec4(-uFrameUniforms.origin.xyz, 1.0);
                                          earthCenter = earthCenter / earthCenter.w;

                                          vec4 screenPosition = uFrameUniforms.vpMatrix * vec4(uOriginOffset.xyz, 1.0);
                                          screenPosition = screenPosition / screenPosition.w;

                                          float mask = float(screenPosition.z - earthCenter.z < 0.0);
                                          vec2 safeViewportSize = max(viewportSize, vec2(1.0));
                                          vec2 screenSize = vPosition.xy * (2.0 / safeViewportSize);
                                          gl_Position = mix(vec4(-10.0, -10.0, -10.0, -10.0),
                                                            vec4(screenPosition.xy + screenSize, screenPosition.z, 1.0),
                                                            mask);
                                          v_texcoord = texCoordinate;
                                      }
        );
    }

    return OMMVersionedGlesShaderCodeWithFrameUBO(320 es, 300 es,
                                      uniform mat4 umMatrix;
                                      uniform vec4 uOriginOffset;
                                      in vec3 vPosition;
                                      in vec2 texCoordinate;
                                      out vec2 v_texcoord;

                                      void main() {
                                          mat4 modelToWorldCameraCentric = umMatrix;
                                          modelToWorldCameraCentric[3] += uOriginOffset;

                                          vec4 scaledPosition = vec4(vPosition, 1.0);
                                          scaledPosition.xy *= uFrameUniforms.frameSpecs.x;

                                          gl_Position = uFrameUniforms.vpMatrix * (modelToWorldCameraCentric * scaledPosition);
                                          v_texcoord = texCoordinate;
                                      }
    );
}

std::string ColorCircleShaderOpenGl::getFragmentShader() {
    return OMMVersionedGlesShaderCode(320 es, 300 es,
                                      precision mediump float;
                                      uniform vec4 fillColor;
                                      uniform vec4 strokeColor;
                                      uniform float innerRadius;
                                      in vec2 v_texcoord;
                                      out vec4 fragmentColor;

                                      void main() {
                                          highp vec2 circleCenter = vec2(0.5, 0.5);
                                          highp float dist = distance(v_texcoord, circleCenter);

                                          highp float edgeWidth = fwidth(dist);
                                          highp float innerRadiusTexCoord = innerRadius * 0.5;
                                          highp float outerCoverage = 1.0 - smoothstep(0.5 - edgeWidth, 0.5, dist);
                                          highp float fillCoverage = innerRadiusTexCoord > 0.0
                                              ? 1.0 - smoothstep(innerRadiusTexCoord - edgeWidth, innerRadiusTexCoord, dist)
                                              : 0.0;
                                          highp float strokeCoverage = max(outerCoverage - fillCoverage, 0.0);
                                          highp float alpha = fillColor.a * fillCoverage + strokeColor.a * strokeCoverage;

                                          if (alpha <= 0.0) {
                                              discard;
                                          }

                                          fragmentColor = vec4(
                                              fillColor.rgb * fillColor.a * fillCoverage + strokeColor.rgb * strokeColor.a * strokeCoverage,
                                              alpha);
                                      });
}

std::shared_ptr<ShaderProgramInterface> ColorCircleShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }
