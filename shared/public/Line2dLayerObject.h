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

#include "Coord.h"
#include "LineStyle.h"
#include "CoordinateConversionHelperInterface.h"
#include "LayerObjectInterface.h"
#include "RenderConfig.h"
#include "Vec2D.h"
#include "LineGroupShaderInterface.h"
#include "LineGroup2dLayerObject.h"
#include "ShaderLineStyle.h"

class Line2dLayerObject : public LayerObjectInterface {
  public:
    Line2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                      const std::shared_ptr<LineGroup2dInterface> &line, const std::shared_ptr<LineGroupShaderInterface> &shader);

    ~Line2dLayerObject(){};

    virtual void update() override;

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig() override;

    void setPositions(const std::vector<Coord> &positions);

    void setStyle(const LineStyle &style);

    void setHighlighted(bool highlighted);

    std::shared_ptr<GraphicsObjectInterface> getLineObject();

    std::shared_ptr<ShaderProgramInterface> getShaderProgram();

private:
    void setStyle(const LineStyle &style, bool highlighted);

  private:
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
    std::shared_ptr<LineGroup2dInterface> line;
    std::shared_ptr<LineGroupShaderInterface> shader;
    std::vector<std::shared_ptr<RenderConfigInterface>> renderConfig;

    LineStyle style;
    bool highlighted;
};
