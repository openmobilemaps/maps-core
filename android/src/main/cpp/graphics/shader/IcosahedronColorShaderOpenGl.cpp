/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "IcosahedronColorShaderOpenGl.h"
#include "OpenGlContext.h"
#include "OpenGlHelper.h"

const std::string IcosahedronColorShaderOpenGl::programName = "UBMAP_IcosahedronColorShaderOpenGl";

std::string IcosahedronColorShaderOpenGl::getProgramName() { return programName; }

void IcosahedronColorShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
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

    blendMode = BlendMode::MULTIPLY;
}

void IcosahedronColorShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    BaseShaderProgramOpenGl::preRender(context);
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(programName);

    int mColorHandle = glGetUniformLocation(program, "vColor");
    {
        glUniform4fv(mColorHandle, 1, &color[0]);
        std::lock_guard<std::mutex> lock(dataMutex);
    }
}

void IcosahedronColorShaderOpenGl::setColor(float red, float green, float blue, float alpha) {
    std::lock_guard<std::mutex> lock(dataMutex);
    color[0] = red;
    color[1] = green;
    color[2] = blue;
    color[3] = alpha;
}

std::string IcosahedronColorShaderOpenGl::getVertexShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      precision highp float;
                                      uniform mat4 uvpMatrix;
                                      in vec3 vLatLon;
                                      in float vValue;
                                      out float gridValue;

                                      void main() {
                                          float phi = (vLatLon.y - 180.0) * 3.1415926536 / 180.0;
                                          float th = (vLatLon.x - 90.0) * 3.1415926536 / 180.0;

                                          vec4 adjPos = vec4(1.0 * sin(th) * cos(phi),
                                                             1.0 * cos(th),
                                                             -1.0 * sin(th) * sin(phi),
                                                             1.0);
                                          gridValue = vValue;
                                          gl_Position = uvpMatrix * adjPos;
                                      }
    );
}


std::string IcosahedronColorShaderOpenGl::getFragmentShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                precision mediump float;
                                uniform vec4 vColor;
                                in float gridValue;
                                out vec4 fragmentColor;

                                void main() {
                                    /*float temperature = gridValue;
                                    // Normalize temperature to a 0.0 - 1.0 range based on expected min/max values
                                    // Adjust minTemp and maxTemp based on your application's expected temperature range
                                    float minTemp = 253.15; // -20°C, adjust as needed
                                    float maxTemp = 323.15; // 50°C, adjust as needed
                                    float normalizedTemp = (temperature - minTemp) / (maxTemp - minTemp);

                                    // Clamp value between 0.0 and 1.0 to ensure it stays within the gradient bounds
                                    normalizedTemp = clamp(normalizedTemp, 0.0, 1.0);

                                    // Define colors for cold (blue), medium (green), and hot (red)
                                    vec4 coldColor = vec4(0.0, 0.0, 1.0, 1.0); // Blue
                                    vec4 mediumColor = vec4(0.0, 1.0, 0.0, 1.0); // Green
                                    vec4 hotColor = vec4(1.0, 0.0, 0.0, 1.0); // Red

                                    // Interpolate between colors based on the normalized temperature
                                    vec4 colorRes;
                                    if (normalizedTemp < 0.5)
                                    {
                                        // Transition from cold to medium
                                        float t = normalizedTemp * 2.0; // Scale to 0.0 - 1.0 range
                                        colorRes = mix(coldColor, mediumColor, t);
                                    }
                                    else
                                    {
                                        // Transition from medium to hot
                                        float t = (normalizedTemp - 0.5) * 2.0; // Scale to 0.0 - 1.0 range
                                        colorRes = mix(mediumColor, hotColor, t);
                                    }

                                    // Return the interpolated color with the original alpha value
                                    fragmentColor = vec4(colorRes.rgb * 0.5, 0.5);*/
                                    float precipFloat = clamp(gridValue * 0.75/* * 3600.0 / 100.0*/, 0.0, 1.0);
                                    //precipFloat = precipFloat * precipFloat;
                                    if (gridValue < 0.01) {
                                        discard;
                                    }
                                    float alpha = 0.5;
                                    fragmentColor = mix(vec4(vec3(0.149, 0.0, 0.941/*0.29, 0.56, 0.27*/) * alpha, alpha), vec4(vec3(0.51, 1.0, 0.36/*0.99, 1.0, 0.36*/) * alpha, alpha), precipFloat);
                                });
}

std::shared_ptr<ShaderProgramInterface> IcosahedronColorShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }