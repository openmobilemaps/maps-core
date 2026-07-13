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
#include "ColorCircleShaderInterface.h"
#include "ShaderProgramInterface.h"
#include <array>

class ColorCircleShaderOpenGl : public BaseShaderProgramOpenGl,
                                public ColorCircleShaderInterface,
                                public std::enable_shared_from_this<ShaderProgramInterface> {
  public:
    ColorCircleShaderOpenGl(bool projectOntoUnitSphere);

    virtual std::shared_ptr<ShaderProgramInterface> asShaderProgramInterface() override;

    virtual std::string getProgramName() override;

    virtual void setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void preRender(const std::shared_ptr<::RenderingContextInterface> &context, bool isScreenSpaceCoords) override;

    virtual void setColor(float red, float green, float blue, float alpha) override;

    virtual void setCircleStyle(float fillRed, float fillGreen, float fillBlue, float fillAlpha,
                                float strokeRed, float strokeGreen, float strokeBlue, float strokeAlpha,
                                float innerRadius) override;

  protected:
    std::string getVertexShader() override;

    virtual std::string getFragmentShader() override;

  private:
    const std::string programName;
    const bool projectOntoUnitSphere;

    std::mutex dataMutex;
    std::array<float, 4> fillColor = {0.0, 0.0, 0.0, 0.0};
    std::array<float, 4> strokeColor = {0.0, 0.0, 0.0, 0.0};
    float innerRadius = 1.0f;
};
