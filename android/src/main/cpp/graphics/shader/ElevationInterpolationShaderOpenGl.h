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
#include "ElevationInterpolationShaderInterface.h"

class ElevationInterpolationShaderOpenGl : public BaseShaderProgramOpenGl,
                                           public ElevationInterpolationShaderInterface,
                                           public std::enable_shared_from_this<ShaderProgramInterface> {
public:
    ElevationInterpolationShaderOpenGl();

    std::shared_ptr<ShaderProgramInterface> asShaderProgramInterface() override;

    std::string getProgramName() override;

    void setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) override;

protected:
    virtual std::string getFragmentShader() override;
private:
    const std::string programName;

};
