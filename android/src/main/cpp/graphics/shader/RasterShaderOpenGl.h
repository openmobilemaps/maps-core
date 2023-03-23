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
#include "ShaderProgramInterface.h"
#include "RasterShaderInterface.h"
#include "RenderingContextInterface.h"
#include "RasterShaderStyle.h"
#include <vector>

class RasterShaderOpenGl : public BaseShaderProgramOpenGl,
                           public ShaderProgramInterface,
                           public RasterShaderInterface,
                           public std::enable_shared_from_this<ShaderProgramInterface> {
public:
    std::string getProgramName() override;

    void setupProgram(const std::shared_ptr<RenderingContextInterface> &context) override;

    void preRender(const std::shared_ptr<RenderingContextInterface> &context) override;

    std::shared_ptr<ShaderProgramInterface> asShaderProgramInterface() override;

    void setStyle(const RasterShaderStyle &style) override;


protected:
    std::string getFragmentShader() override;

private:
    std::vector<GLfloat> styleValues = {1.0, 1.0, 1.0, 0.0, 1.0};
};

