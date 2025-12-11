/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "SphereEffectShaderOpenGl.h"
#include "OpenGlContext.h"
#include "OpenGlHelper.h"
#include <cstring>

const std::string SphereEffectShaderOpenGl::programName = "UBMAP_SphereEffectShaderOpenGl";

std::string SphereEffectShaderOpenGl::getProgramName() { return programName; }

void SphereEffectShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
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

void SphereEffectShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context, bool isScreenSpaceCoords) {
    BaseShaderProgramOpenGl::preRender(context, isScreenSpaceCoords);
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(programName);

    int mCoefficientsHandle = glGetUniformLocation(program, "vCoefficients");
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        glUniformMatrix4fv(mCoefficientsHandle, 1, false, (GLfloat *)&coefficients[0]);
    }
}

void SphereEffectShaderOpenGl::setEllipse(const SharedBytes &coefficients) {
    std::lock_guard<std::mutex> lock(dataMutex);

    if(coefficients.elementCount > 0) {
        std::memcpy(this->coefficients.data(), (void *)coefficients.address,
                    coefficients.elementCount * coefficients.bytesPerElement);
    }
}

// TO_CHANGE
std::string SphereEffectShaderOpenGl::getVertexShader() {
    return OMMVersionedGlesShaderCode(300 es,
                                              uniform mat4 umMatrix;
                                              in vec4 vPosition;
                                              in vec2 texCoordinate;
                                              out vec2 v_texcoord;

                                              void main() {
                                                  gl_Position = umMatrix * vPosition;
                                                  v_texcoord = texCoordinate;
                                              }
    );
}

std::string SphereEffectShaderOpenGl::getFragmentShader() {
    return OMMVersionedGlesShaderCode(300 es,
                                      precision highp float;
                                              uniform mat4 vCoefficients;
                                              in vec2 v_texcoord;
                                              out vec4 fragmentColor;

                                              void main() {
                                                  float y1 = v_texcoord.x;
                                                  float y2 = -v_texcoord.y;

                                                  // Extract elements of an inverse projection matrix for better readability
                                                  float a11 = vCoefficients[0][0];
                                                  float a12 = vCoefficients[1][0];
                                                  float a13 = vCoefficients[2][0];
                                                  float a14 = vCoefficients[3][0];
                                                  float a21 = vCoefficients[0][1];
                                                  float a22 = vCoefficients[1][1];
                                                  float a23 = vCoefficients[2][1];
                                                  float a24 = vCoefficients[3][1];
                                                  float a31 = vCoefficients[0][2];
                                                  float a32 = vCoefficients[1][2];
                                                  float a33 = vCoefficients[2][2];
                                                  float a34 = vCoefficients[3][2];
                                                  float a41 = vCoefficients[0][3];
                                                  float a42 = vCoefficients[1][3];
                                                  float a43 = vCoefficients[2][3];
                                                  float a44 = vCoefficients[3][3];

                                                  float k1 = y1 * a11 + y2 * a12 + a14;
                                                  float k2 = y1 * a21 + y2 * a22 + a24;
                                                  float k3 = y1 * a31 + y2 * a32 + a34;
                                                  float k4 = y1 * a41 + y2 * a42 + a44;

                                                  float l1 = a13 * a13 + a23 * a23 + a33 * a33;
                                                  float l2 = 2.0 * k1 * a13 + 2.0 * k2 * a23 + 2.0 * k3 * a33;
                                                  float l3 = k1 * k1+k2 * k2+k3 * k3;

                                                  float divident = l2 * l2 - 4.0 * l1 * l3;
                                                  float divisor = 4.0 * (l2 - (l3 / k4) * a43 - (l1 / a43) * k4);
                                                  float Rsq = ((divident / k4) / a43) / divisor;
                                                  float R = sqrt(Rsq);

                                                  float L = clamp((R - 1.0) * 5.0, -1.0, 1.0);

                                                  vec4 white = vec4(1.0,1.0,1.0,1.0);
                                                  vec4 blueOut = vec4(44.0/255.0,166.0/255.0,1.0,1.0);
                                                  vec4 blueClear = vec4(0.0,148.0/255.0,1.0,1.0);
                                                  float opacity = 0.7;

                                                  if (L > 0.0) {
                                                      float t = clamp(L * 2.4, 0.0, 1.0);

                                                      vec4 c, c2;
                                                      float alpha, alpha2;

                                                      if(t < 0.5) {
                                                          t = t / 0.5;
                                                          c = white;
                                                          alpha = 1.0;
                                                          c2 = blueOut;
                                                          alpha2 = 0.5;
                                                      } else {
                                                          t = (t - 0.5) / 0.5;
                                                          c = blueOut;
                                                          c2 = blueClear;
                                                          alpha = 0.5;
                                                          alpha2 = 0.0;
                                                      }

                                                      fragmentColor = ((1.0 - t) * alpha + t * alpha2) * ((1.0 - t) * c + t * c2) * opacity;
                                                  } else {
                                                      fragmentColor = vec4(1.0, 1.0, 1.0, 1.0) * (1.0 + L * 10.0);
                                                  }
//                                                  fragmentColor =  white * opacity;
                                              });
}

std::shared_ptr<ShaderProgramInterface> SphereEffectShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }
