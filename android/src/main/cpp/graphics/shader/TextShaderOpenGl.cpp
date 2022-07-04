/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "TextShaderOpenGl.h"
#include "OpenGlContext.h"
#include "OpenGlHelper.h"

std::string TextShaderOpenGl::getProgramName() { return "UBMAP_TextShaderOpenGl"; }

void TextShaderOpenGl::setColor(float red, float green, float blue, float alpha) {
    color = std::vector<float>{red, green, blue, alpha};
}

void TextShaderOpenGl::setScale(float scale) { this->scale = scale; }

void TextShaderOpenGl::setReferencePoint(const Vec3D &point) {
    this->referencePoint = {(float)point.x, (float)point.y, (float)point.z};
}

void TextShaderOpenGl::setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    std::string programName = getProgramName();
    // prepare shaders and OpenGL program
    int vertexShader = loadShader(GL_VERTEX_SHADER, getVertexShader());
    int fragmentShader = loadShader(GL_FRAGMENT_SHADER, getFragmentShader());

    int program = glCreateProgram();       // create empty OpenGL Program
    glAttachShader(program, vertexShader); // add the vertex shader to program
    OpenGlHelper::checkGlError("glAttachShader Vertex  Text");
    glDeleteShader(vertexShader);
    glAttachShader(program, fragmentShader); // add the fragment shader to program
    OpenGlHelper::checkGlError("glAttachShader Fragment Text");
    glDeleteShader(fragmentShader);
    glLinkProgram(program); // create OpenGL program executables

    checkGlProgramLinking(program);
    OpenGlHelper::checkGlError("glLinkProgram Text");

    openGlContext->storeProgram(programName, program);
}

void TextShaderOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(getProgramName());

    int mColorHandle = glGetUniformLocation(program, "vColor");
    glUniform4fv(mColorHandle, 1, &color[0]);

    int mRefPointHandle = glGetUniformLocation(program, "vReferencePoint");
    glUniform3fv(mRefPointHandle, 1, &referencePoint[0]);

    int mScaleHandle = glGetUniformLocation(program, "vScale");
    glUniform1f(mScaleHandle, scale);
}

std::string TextShaderOpenGl::getVertexShader() {
    return UBRendererShaderCode(uniform mat4 uMVPMatrix; uniform vec3 vReferencePoint; uniform float vScale;
                                attribute vec2 vPosition; attribute vec2 texCoordinate; varying vec2 v_texcoord;

                                void main() {
                                    vec2 pos = (uMVPMatrix * vec4(vPosition.xy, 0.0, 1.0)).xy;
                                    vec2 ref = (uMVPMatrix * vec4(vReferencePoint.xy, 0.0, 1.0)).xy;
                                    pos = ref.xy + (pos.xy - ref.xy) * vScale;
                                    gl_Position = vec4(pos.xy, 0.0, 1.0);
                                    v_texcoord = texCoordinate;
                                });
}

std::string TextShaderOpenGl::getFragmentShader() {
    return UBRendererShaderCode(precision highp float; uniform sampler2D u_texture; uniform vec4 vColor; varying vec2 v_texcoord;

                                void main() {
                                    float delta = 0.1;
                                    vec4 dist = texture2D(u_texture, v_texcoord);
                                    float alpha = smoothstep(0.5 - delta, 0.5 + delta, dist.x) * vColor.a;
                                    gl_FragColor = vColor;
                                    gl_FragColor.a = 1.0;
                                    gl_FragColor *= alpha;
                                });
}

std::shared_ptr<ShaderProgramInterface> TextShaderOpenGl::asShaderProgramInterface() { return shared_from_this(); }
