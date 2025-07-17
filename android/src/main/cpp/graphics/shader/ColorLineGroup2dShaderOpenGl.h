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
#include <mutex>
#include <vector>

class ColorLineGroup2dShaderOpenGl : public BaseShaderProgramOpenGl,
                                     public LineGroupShaderInterface,
                                     public std::enable_shared_from_this<ShaderProgramInterface> {
public:
    ColorLineGroup2dShaderOpenGl(bool projectOntoUnitSphere, bool simple);

    virtual std::shared_ptr<ShaderProgramInterface> asShaderProgramInterface() override;

    virtual std::string getProgramName() override;

    virtual void setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void preRender(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void setStyles(const ::SharedBytes & styles) override;

    virtual void setDashingScaleFactor(float factor) override;

    virtual void setupGlObjects(const std::shared_ptr<::OpenGlContext> &context) override;

    virtual void clearGlObjects() override;

    virtual bool isRenderable() override;

    virtual bool usesModelMatrix() override { return false; };

protected:
    virtual std::string getVertexShader() override;

    virtual std::string getFragmentShader() override;

    std::string getSimpleLineFragmentShader();
    std::string getLineFragmentShader();

private:
    static std::string getLineStylesUBODefinition(bool isSimpleLine);

    const bool projectOntoUnitSphere;
    const bool isSimpleLine;
    const std::string programName;

    GLint dashingScaleFactorHandle = -1;

    std::recursive_mutex styleMutex;
    GLuint lineStyleBuffer = 0;
    std::vector<GLfloat> lineValues;
    bool stylesUpdated = false;
    GLint numStyles = 0;

    float dashingScaleFactor = 1.0;

    static const GLuint STYLE_UBO_BINDING = 1;
    static const int MAX_NUM_STYLES = 32;
    //const int sizeStyleValues = 3;
    //const int sizeColorValues = 4;
    //const int sizeGapColorValues = 4;
    //const int maxNumDashValues = 4;
   // const int sizeDashValues = maxNumDashValues + 3;
    const int sizeFullLineValues = 24; //sizeStyleValues + sizeColorValues + sizeGapColorValues + sizeDashValues + 1 => 23, with padding (to have styles aligned to 4 floats - std140);
    const int sizeSimpleLineValues = 8; // ensure size multiple of 4 floats - std140

    const int sizeLineValuesArray;
    const int sizeLineValues;
};
