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
                                     public std::enable_shared_from_this<ShaderProgramInterface> {
  public:
    virtual std::shared_ptr<ShaderProgramInterface> asShaderProgramInterface() override;

    virtual std::string getProgramName() override;

    virtual void setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void preRender(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void setStyles(const ::SharedBytes & styles) override;

    void setDashingScaleFactor(float factor) override;

protected:
    virtual std::string getVertexShader() override;

    virtual std::string getFragmentShader() override;

  private:
    std::recursive_mutex styleMutex;
    std::vector<GLfloat> lineValues;
    GLint numStyles = 0;

    float dashingScaleFactor = 1.0;

    const int maxNumStyles = 32;
    //const int sizeStyleValues = 3;
    //const int sizeColorValues = 4;
    //const int sizeGapColorValues = 4;
    //const int maxNumDashValues = 4;
   // const int sizeDashValues = maxNumDashValues + 1;
    const int sizeLineValues = 19;//sizeStyleValues + sizeColorValues + sizeGapColorValues + sizeDashValues + 1;
    const int sizeLineValuesArray = sizeLineValues * maxNumStyles;
};
