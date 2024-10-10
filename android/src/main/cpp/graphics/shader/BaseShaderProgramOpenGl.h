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

#define OMMShaderCode(...) std::string(#__VA_ARGS__)
#define OMMVersionedGlesShaderCode(version, ...) std::string("#version " #version "\n" #__VA_ARGS__)

#include "Logger.h"
#include "ShaderProgramInterface.h"
#include "opengl_wrapper.h"
#include "BlendMode.h"

class BaseShaderProgramOpenGl: public ShaderProgramInterface {
  protected:
    virtual std::string getVertexShader();

    virtual std::string getFragmentShader();

public:
    static int loadShader(int type, std::string shaderCode);

    static void checkGlProgramLinking(GLuint program);

    virtual void preRender(const std::shared_ptr<::RenderingContextInterface> &context) override;

protected:

    virtual void setBlendMode(BlendMode blendMode) override;

    BlendMode blendMode = BlendMode::NORMAL;
};