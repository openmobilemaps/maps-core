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
#include "ColorShaderInterface.h"
#include "ShaderProgramInterface.h"
#include <vector>

class ColorShaderOpenGl : public BaseShaderProgramOpenGl,
                          public ColorShaderInterface,
                          public std::enable_shared_from_this<ShaderProgramInterface> {
  public:
    ColorShaderOpenGl(bool projectOntoUnitSphere);

    virtual std::shared_ptr<ShaderProgramInterface> asShaderProgramInterface() override;

    virtual std::string getProgramName() override;

    virtual void setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void preRender(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void setColor(float red, float green, float blue, float alpha) override;

  protected:
    virtual std::string getVertexShader() override;

    virtual std::string getFragmentShader() override;

  private:
    const bool projectOntoUnitSphere;
    const std::string programName;

    std::mutex dataMutex;
    std::vector<float> color = {0, 0, 0, 0};
};