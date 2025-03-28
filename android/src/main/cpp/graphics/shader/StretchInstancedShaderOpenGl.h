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

#include "StretchInstancedShaderInterface.h"
#include "BaseShaderProgramOpenGl.h"
#include "RenderingContextInterface.h"
#include "ShaderProgramInterface.h"

class StretchInstancedShaderOpenGl : public BaseShaderProgramOpenGl,
                          public StretchInstancedShaderInterface,
                          public std::enable_shared_from_this<StretchInstancedShaderOpenGl> {

  public:
    StretchInstancedShaderOpenGl(bool projectOntoUnitSphere);

    virtual std::string getProgramName() override;

    virtual void setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void preRender(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual std::shared_ptr<ShaderProgramInterface> asShaderProgramInterface() override;

  protected:
    const bool projectOntoUnitSphere;
    const std::string programName;

    virtual std::string getFragmentShader() override;

    virtual std::string getVertexShader() override;
};