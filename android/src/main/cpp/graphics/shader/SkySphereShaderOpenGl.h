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
#include "SkySphereShaderInterface.h"

class SkySphereShaderOpenGl
        : public BaseShaderProgramOpenGl,
          public SkySphereShaderInterface,
          public std::enable_shared_from_this<SkySphereShaderOpenGl> {
public:
    std::string getProgramName() override;

    void setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) override;

    std::shared_ptr<ShaderProgramInterface> asShaderProgramInterface() override;

    void setCameraProperties(const std::vector<float> &inverseVP, const Vec3D &cameraPosition) override;

    void preRender(const std::shared_ptr<::RenderingContextInterface> &context, bool isScreenSpaceCoords) override;

    std::string getVertexShader() override;

protected:
    std::string getFragmentShader() override;

private:
    const static std::string programName;

    std::mutex dataMutex;
    std::vector<GLfloat> inverseVPMatrix = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
    Vec3D cameraPosition = {0.0, 0.0, 0.0};
    GLint inverseVPMatrixHandle;
    GLint cameraPositionHandle;
};
