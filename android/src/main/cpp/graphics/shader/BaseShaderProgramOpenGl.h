/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#ifndef MAPSCORE_BASESHADERPROGRAMOPENGL_H
#define MAPSCORE_BASESHADERPROGRAMOPENGL_H

#define UBRendererShaderCode(...) #__VA_ARGS__

#include "../logger/Logger.h"
#include "ShaderProgramInterface.h"
#include "opengl_wrapper.h"

class BaseShaderProgramOpenGl {
  protected:
    int loadShader(int type, std::string shaderCode);

    void checkGlProgramLinking(GLuint program);

    virtual std::string getVertexShader();

    virtual std::string getFragmentShader();
};

#endif // MAPSCORE_BASESHADERPROGRAMOPENGL_H
