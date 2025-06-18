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

#include "BaseShaderProgramOpenGl.h"
#include "RenderingContextInterface.h"
#include "ShaderProgramInterface.h"
#include "TextInstancedShaderInterface.h"
#include <vector>

class TextInstancedShaderOpenGl : public BaseShaderProgramOpenGl,
                         public TextInstancedShaderInterface,
                         public std::enable_shared_from_this<ShaderProgramInterface> {

  public:
    TextInstancedShaderOpenGl(bool projectOntoUnitSphere);

    virtual std::string getProgramName() override;

    virtual void setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void preRender(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual std::shared_ptr<ShaderProgramInterface> asShaderProgramInterface() override;

    static const int BYTE_SIZE_TEXT_STYLES;
    static const int MAX_NUM_TEXT_STYLES;

protected:
    const bool projectOntoUnitSphere;
    const std::string programName;

    virtual std::string getFragmentShader() override;

    virtual std::string getVertexShader() override;
};
