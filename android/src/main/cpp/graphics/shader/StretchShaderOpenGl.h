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

#include "StretchShaderInterface.h"
#include "BaseShaderProgramOpenGl.h"
#include "RenderingContextInterface.h"
#include "ShaderProgramInterface.h"
#include "StretchShaderInfo.h"
#include <vector>

class StretchShaderOpenGl : public BaseShaderProgramOpenGl,
                          public ShaderProgramInterface,
                          public StretchShaderInterface,
                          public std::enable_shared_from_this<ShaderProgramInterface> {

  public:
    virtual std::string getProgramName() override;

    virtual void setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void preRender(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual std::shared_ptr<ShaderProgramInterface> asShaderProgramInterface() override;

    void updateAlpha(float value) override;

    void updateStretchInfo(const StretchShaderInfo &info) override;

protected:
    virtual std::string getFragmentShader() override;

  private:
    float alpha = 1;
    StretchShaderInfo stretchShaderInfo = StretchShaderInfo(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, RectD(0.0, 0.0, 1.0, 1.0));
};