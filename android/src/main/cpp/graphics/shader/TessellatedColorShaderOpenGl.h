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

#include "ColorShaderOpenGl.h"
#include "RenderingContextInterface.h"
#include "Tiled2dMapVectorLayerConstants.h"

class TessellatedColorShaderOpenGl : public ColorShaderOpenGl {
public:
    TessellatedColorShaderOpenGl(bool projectOntoUnitSphere);

    void setupProgram(const std::shared_ptr<RenderingContextInterface> &context) override;

protected:
    std::string getVertexShader() override;

    virtual std::string getControlShader();

    virtual std::string getEvaluationShader();

#if HARDWARE_TESSELLATION_WIREFRAME
    virtual std::string getGeometryShader();
#endif
};

