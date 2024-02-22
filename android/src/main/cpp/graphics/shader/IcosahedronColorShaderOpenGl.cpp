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
                                      uniform mat4 uMVPMatrix;
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
                                          gl_Position = uMVPMatrix * adjPos;
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
                                    fragmentColor = vColor;
                                    fragmentColor.a = gridValue;
                                    fragmentColor *= vColor.a;
                                });
}

std::shared_ptr<ShaderProgramInterface> IcosahedronColorShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }