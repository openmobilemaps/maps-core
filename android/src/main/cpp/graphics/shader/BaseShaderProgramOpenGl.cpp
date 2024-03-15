/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "BaseShaderProgramOpenGl.h"
#include <vector>

int BaseShaderProgramOpenGl::loadShader(int type, std::string shaderCode) {
    // create a vertex shader type (GL_VERTEX_SHADER)
    // or a fragment shader type (GL_FRAGMENT_SHADER)
    int shader = glCreateShader(type);

    // add the source code to the shader and compile it
    const char *code = shaderCode.c_str();
    int code_length = int(shaderCode.size());
    glShaderSource(shader, 1, &code, &code_length);
    glCompileShader(shader);

    GLint isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        std::vector<GLchar> errorLog(maxLength);
        glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

        LogError << "Shader " << shader << " failed:\n";

        for (auto a : errorLog) {
            LogError << a;
        }

        LogError <<= ".";
    }
    return shader;
}

void BaseShaderProgramOpenGl::checkGlProgramLinking(GLuint program) {
    GLint isLinked = 0;

    glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE) {
        GLint maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        std::vector<GLchar> infoLog(maxLength);
        glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

        LogError << "OpenGL Program Linking failed:";

        for (auto a : infoLog) {
            LogError << a;
        }

        LogError <<= ".";
    }
}


std::string BaseShaderProgramOpenGl::getVertexShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      uniform mat4 uvpMatrix;
                                      uniform mat4 umMatrix;
                                      in vec4 vPosition;
                                      in vec2 texCoordinate;
                                      out vec2 v_texcoord;

                                      void main() {
                                          gl_Position = uvpMatrix * umMatrix * vPosition;
                                          v_texcoord = texCoordinate;
                                      }
    );
}

std::string BaseShaderProgramOpenGl::getUnitSphereVertexShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      uniform mat4 uvpMatrix;
                                      uniform mat4 umMatrix;
                                      in vec4 vPosition;
                                      in vec2 texCoordinate;
                                      out vec2 v_texcoord;

                                      void main() {
                                          gl_Position = umMatrix * vPosition;
                                          gl_Position = gl_Position / gl_Position.w;
                                          gl_Position = uvpMatrix * vec4(vPosition.z * sin(vPosition.y) * cos(vPosition.x),
                                                                          vPosition.z * cos(vPosition.y),
                                                                          -vPosition.z * sin(vPosition.y) * sin(vPosition.x),
                                                                          1.0);
                                          v_texcoord = texCoordinate;
                                      }
    );
}

std::string BaseShaderProgramOpenGl::getFragmentShader() {
    return OMMVersionedGlesShaderCode(320 es,
                                      precision mediump float;
                                      uniform sampler2D textureSampler;
                                      in vec2 v_texcoord;
                                      out vec4 fragmentColor;

                                      void main() {
                                          fragmentColor = texture(textureSampler, v_texcoord);
                                      }
    );
}

void BaseShaderProgramOpenGl::setBlendMode(BlendMode blendMode) {
    this->blendMode = blendMode;
}

void BaseShaderProgramOpenGl::preRender(const std::shared_ptr<::RenderingContextInterface> &context) {
    glEnable(GL_BLEND);
    switch (blendMode) {
        case BlendMode::NORMAL: {
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            break;
        }
        case BlendMode::MULTIPLY: {
            glBlendFuncSeparate(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
            break;
        }
    }
}