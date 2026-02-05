/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "Logger.h"
#include "opengl_wrapper.h"

class OpenGlHelper {
  public:
    static void checkGlError(const std::string &glOperation) {
        int error;
        while ((error = glGetError()) != GL_NO_ERROR) {
            LogError << "GL ERROR: " << glOperation << " " <<= errorString(error);
        }
    }

    static std::string errorString(GLenum errorCode) {
        switch (errorCode) {
        case GL_INVALID_ENUM:
            return "INVALID_ENUM";
        case GL_INVALID_VALUE:
            return "INVALID_VALUE";
        case GL_INVALID_OPERATION:
            return "INVALID_OPERATION";
        case GL_STACK_OVERFLOW:
            return "STACK_OVERFLOW";
        case GL_STACK_UNDERFLOW:
            return "STACK_UNDERFLOW";
        case GL_OUT_OF_MEMORY:
            return "OUT_OF_MEMORY";
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return "INVALID_FRAMEBUFFER_OPERATION";
        default:
            return "UNKNOWN_ERROR(" + std::to_string(errorCode) + ")";
        }
    }

    static void generateMipmap(uint32_t texturePointer);
};
