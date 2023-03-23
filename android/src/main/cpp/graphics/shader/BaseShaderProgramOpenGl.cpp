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
    // create a vertex shader type (GLES20.GL_VERTEX_SHADER)
    // or a fragment shader type (GLES20.GL_FRAGMENT_SHADER)
    int shader = glCreateShader(type);

    // LogInfo << "Compiling Shader Code: " <<= shaderCode;

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
    return UBRendererShaderCode(uniform mat4 uMVPMatrix; attribute vec4 vPosition; attribute vec2 texCoordinate;
                                varying vec2 v_texcoord;

                                void main() {
                                    gl_Position = uMVPMatrix * vPosition;
                                    v_texcoord = texCoordinate;
                                });
}

std::string BaseShaderProgramOpenGl::getFragmentShader() {
    return UBRendererShaderCode(precision mediump float; uniform sampler2D texture; varying vec2 v_texcoord;

                                void main() { gl_FragColor = texture2D(texture, v_texcoord); });
}
