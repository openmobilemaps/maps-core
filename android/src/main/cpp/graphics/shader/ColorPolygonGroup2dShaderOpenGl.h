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
#include "PolygonGroupShaderInterface.h"
#include "ShaderProgramInterface.h"
#include <vector>

class ColorPolygonGroup2dShaderOpenGl : public BaseShaderProgramOpenGl,
                                        public PolygonGroupShaderInterface,
                                        public ShaderProgramInterface,
                                        public std::enable_shared_from_this<ShaderProgramInterface> {
  public:
    virtual std::shared_ptr<ShaderProgramInterface> asShaderProgramInterface() override;

    virtual std::string getProgramName() override;

    virtual void setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void preRender(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void setStyles(const ::SharedBytes & styles) override;

  protected:
    virtual std::string getVertexShader() override;

    virtual std::string getFragmentShader() override;

  private:
    std::recursive_mutex styleMutex;
    std::vector<GLfloat> polygonStyles;
    GLint numStyles;

    const int sizeStyleValues = 5;
    const int sizeStyleValuesArray = sizeStyleValues * 32;
};
