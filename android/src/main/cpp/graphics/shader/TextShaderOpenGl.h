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
#include "TextShaderInterface.h"
#include <vector>

class TextShaderOpenGl : public BaseShaderProgramOpenGl,
                         public ShaderProgramInterface,
                         public TextShaderInterface,
                         public std::enable_shared_from_this<ShaderProgramInterface> {

  public:
    virtual std::string getProgramName() override;

    virtual void setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void preRender(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void setColor(const ::Color & color) override;

    virtual void setHaloColor(const ::Color & color) override;

    virtual void setScale(float scale) override;

    virtual void setReferencePoint(const ::Vec3D &point) override;

    virtual std::shared_ptr<ShaderProgramInterface> asShaderProgramInterface() override;

  protected:
    virtual std::string getFragmentShader() override;

    virtual std::string getVertexShader() override;

  private:
    std::vector<float> color = {0.0, 0.0, 0.0, 1.0};
    std::vector<float> haloColor = {0.0, 0.0, 0.0, 1.0};
    std::vector<float> referencePoint = {0.0, 0.0, 0.0};
    float scale = 0.0;
};