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
#include "LineGroupShaderInterface.h"
#include "ShaderProgramInterface.h"
#include <vector>

class ColorLineGroup2dShaderOpenGl : public BaseShaderProgramOpenGl,
                                     public LineGroupShaderInterface,
                                     public ShaderProgramInterface,
                                     public std::enable_shared_from_this<ShaderProgramInterface> {
  public:
    virtual std::shared_ptr<ShaderProgramInterface> asShaderProgramInterface() override;

    virtual std::string getProgramName() override;

    virtual void setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void preRender(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void setStyles(const std::vector<::LineStyle> &lineStyles) override;

  protected:
    virtual std::string getVertexShader() override;

    virtual std::string getFragmentShader() override;

  private:
    std::recursive_mutex styleMutex;
    std::vector<GLfloat> lineStyles;
    std::vector<GLfloat> lineColors;
    std::vector<GLfloat> lineGapColors;
    std::vector<GLfloat> lineDashValues;
    GLint numStyles = 0;

    const int maxNumStyles = 48;
    const int sizeStyleValues = 3;
    const int sizeStyleValuesArray = sizeStyleValues * maxNumStyles;
    const int sizeColorValues = 4;
    const int sizeColorValuesArray = sizeColorValues * maxNumStyles;
    const int sizeGapColorValues = 4;
    const int sizeGapColorValuesArray = sizeGapColorValues * maxNumStyles;
    const int maxNumDashValues = 4;
    const int sizeDashValues = maxNumDashValues + 1;
    const int sizeDashValuesArray = sizeDashValues * maxNumStyles;
};
