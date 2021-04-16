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

#include "AlphaShaderInterface.h"
#include "BaseShaderProgramOpenGl.h"
#include "RenderingContextInterface.h"
#include "ShaderProgramInterface.h"

class AlphaShaderOpenGl : public BaseShaderProgramOpenGl,
                          public ShaderProgramInterface,
                          public AlphaShaderInterface,
                          public std::enable_shared_from_this<ShaderProgramInterface> {

  public:
    virtual std::string getProgramName() override;

    virtual void setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void preRender(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void updateAlpha(float value) override;

    virtual std::shared_ptr<ShaderProgramInterface> asShaderProgramInterface() override;

  protected:
    virtual std::string getFragmentShader() override;

  private:
    float alpha = 1;
};