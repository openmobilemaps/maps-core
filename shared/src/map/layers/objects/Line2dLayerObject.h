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

#include "ColorLineShaderInterface.h"
#include "Coord.h"
#include "CoordinateConversionHelperInterface.h"
#include "LayerObjectInterface.h"
#include "Line2dInterface.h"
#include "RenderConfig.h"
#include "Vec2D.h"

class Line2dLayerObject : public LayerObjectInterface {
  public:
    Line2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                      const std::shared_ptr<Line2dInterface> &line, const std::shared_ptr<ColorLineShaderInterface> &shader);

    ~Line2dLayerObject(){};

    virtual void update() override;

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig() override;

    void setPositions(std::vector<Coord> positions);

    std::shared_ptr<GraphicsObjectInterface> getLineObject();

    std::shared_ptr<LineShaderProgramInterface> getShaderProgram();

  private:
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
    std::shared_ptr<Line2dInterface> line;
    std::shared_ptr<ColorLineShaderInterface> shader;
    std::vector<std::shared_ptr<RenderConfigInterface>> renderConfig;
};
