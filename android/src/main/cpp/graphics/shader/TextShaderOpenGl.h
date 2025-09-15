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
                         public TextShaderInterface,
                         public std::enable_shared_from_this<ShaderProgramInterface> {

  public:
    virtual std::string getProgramName() override;

    virtual void setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void preRender(const std::shared_ptr<::RenderingContextInterface> &context, bool isScreenSpaceCoords) override;

    virtual void setColor(const ::Color & color) override;

    virtual void setHaloColor(const ::Color & color, float width, float blur) override;

    virtual void setOpacity(float opacity) override;

    virtual std::shared_ptr<ShaderProgramInterface> asShaderProgramInterface() override;

    virtual bool usesModelMatrix() override { return false; };

protected:
    virtual std::string getFragmentShader() override;

    virtual std::string getVertexShader() override;

  private:
    const static std::string programName;

    std::mutex dataMutex;
    std::vector<float> color = {0.0, 0.0, 0.0, 0.0};
    std::vector<float> haloColor = {0.0, 0.0, 0.0, 0.0};
    float opacity = 0.0;
    float haloWidth = 0.0f;
    float haloBlur = 0.0f;
};
