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

#include "LineGroupShaderInterface.h"
#include "LineGroupInstancedShaderInterface.h"
#include "Coord.h"
#include "LineStyle.h"
#include "CoordinateConversionHelperInterface.h"
#include "LayerObjectInterface.h"
#include "LineGroup2dInterface.h"
#include "Line2dInstancedInterface.h"
#include "RenderConfig.h"
#include "Vec2D.h"

class LineGroup2dLayerObject : public LayerObjectInterface {
  public:
    LineGroup2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                      const std::shared_ptr<Line2dInstancedInterface> &line, const std::shared_ptr<LineGroupInstancedShaderInterface> &shader);

    ~LineGroup2dLayerObject(){};

    virtual void update() override;

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig() override;

    void setLines(const std::vector<std::tuple<std::vector<Coord>, int>> &lines);

    void setStyles(const std::vector<LineStyle> &styles);

    std::shared_ptr<GraphicsObjectInterface> getLineObject();

    std::shared_ptr<ShaderProgramInterface> getShaderProgram();

  private:
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
    std::shared_ptr<Line2dInstancedInterface> line;
    std::shared_ptr<LineGroupInstancedShaderInterface> shader;
    std::shared_ptr<RenderConfigInterface> renderConfig;
};
