/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#ifndef MAPSDK_COLORLINESHADEROPENGL_H
#define MAPSDK_COLORLINESHADEROPENGL_H

#include "BaseShaderProgramOpenGl.h"
#include "ColorLineShaderInterface.h"
#include "LineShaderProgramInterface.h"
#include <vector>

class ColorLineShaderOpenGl : public BaseShaderProgramOpenGl,
                              public LineShaderProgramInterface,
                              public ColorLineShaderInterface,
                              public std::enable_shared_from_this<LineShaderProgramInterface> {
  public:
    virtual std::shared_ptr<LineShaderProgramInterface> asLineShaderProgramInterface() override;

    virtual std::string getRectProgramName() override;

    virtual void setupRectProgram(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void preRenderRect(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual std::string getPointProgramName() override;

    virtual void setupPointProgram(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void preRenderPoint(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual std::string getRectVertexShader();

    virtual std::string getRectFragmentShader();

    virtual std::string getPointVertexShader();

    virtual std::string getPointFragmentShader();

    virtual void setColor(float red, float green, float blue, float alpha) override;

    virtual void setMiter(float miter) override;

  private:
    std::vector<float> lineColor;
    float miter;
};

#endif // MAPSDK_COLORLINESHADEROPENGL_H
