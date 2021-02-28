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

#include "ColorShaderInterface.h"
#include "Coord.h"
#include "CoordinateConversionHelperInterface.h"
#include "LayerObjectInterface.h"
#include "Polygon2dInterface.h"
#include "RenderConfig.h"
#include "Vec2D.h"

class Polygon2dLayerObject : public LayerObjectInterface {
  public:
    Polygon2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                         const std::shared_ptr<Polygon2dInterface> &polygon, const std::shared_ptr<ColorShaderInterface> &shader);

    virtual ~Polygon2dLayerObject() override {}

    virtual void update() override{};

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig() override;

    void setPositions(const std::vector<Coord> &positions, const std::vector<std::vector<Coord>> &holes, bool isConvex);

    std::shared_ptr<GraphicsObjectInterface> getPolygonObject();

    std::shared_ptr<ShaderProgramInterface> getShaderProgram();

    void setColor(const Color &color);

  private:
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
    std::shared_ptr<Polygon2dInterface> polygon;
    std::shared_ptr<ColorShaderInterface> shader;
    std::shared_ptr<RenderConfig> renderConfig;
};
