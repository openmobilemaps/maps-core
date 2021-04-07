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
#include "ColorLineShaderInterface.h"
#include "ShaderProgramInterface.h"
#include <vector>

class ColorLineShaderOpenGl : public BaseShaderProgramOpenGl,
                              public ColorLineShaderInterface,
                              public ShaderProgramInterface,
                              public std::enable_shared_from_this<ShaderProgramInterface> {
  public:
    virtual std::shared_ptr<ShaderProgramInterface> asShaderProgramInterface() override;

    virtual std::string getProgramName() override;

    virtual void setupProgram(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void preRender(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void setStyle(const ::LineStyle & lineStyle) override;

    virtual void setHighlighted(bool highlighted) override;

protected:
    virtual std::string getVertexShader() override;

    virtual std::string getFragmentShader() override;

  private:
    LineStyle lineStyle = LineStyle(ColorStateList(Color(1.0, 0.0, 0.0, 1.0), Color(1.0, 0.0, 0.0, 1.0)), SizeType::SCREEN_PIXEL, 4.0);
    Color lineColor = lineStyle.color.normal;
    float miter;
    bool isHighlighted;
};

#endif // MAPSDK_COLORLINESHADEROPENGL_H
