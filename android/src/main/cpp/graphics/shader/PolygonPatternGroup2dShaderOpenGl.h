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
#include "PolygonPatternGroupShaderInterface.h"
#include "ShaderProgramInterface.h"
#include <vector>

class PolygonPatternGroup2dShaderOpenGl : public BaseShaderProgramOpenGl,
                         public PolygonPatternGroupShaderInterface,
                         public std::enable_shared_from_this<ShaderProgramInterface> {

  public:
    PolygonPatternGroup2dShaderOpenGl(bool fadeInPattern, bool projectOntoUnitSphere);

    virtual std::string getProgramName() override;

    virtual void setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void preRender(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual std::shared_ptr<ShaderProgramInterface> asShaderProgramInterface() override;

  protected:
    const bool projectOntoUnitSphere;
    const bool fadeInPattern;
    const std::string programName;

    virtual std::string getFragmentShader() override;

    virtual std::string getVertexShader() override;
};
