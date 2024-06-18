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
#include "PolygonCoord.h"
#include "Coord.h"
#include "CoordinateConversionHelperInterface.h"
#include "LayerObjectInterface.h"
#include "Polygon2dInterface.h"
#include "RenderConfig.h"
#include "Vec2D.h"

class Polygon2dLayerObject : public LayerObjectInterface {
  public:
    Polygon2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                         const std::shared_ptr<Polygon2dInterface> &polygon, const std::shared_ptr<ColorShaderInterface> &shader,
                         bool is3d);

    virtual ~Polygon2dLayerObject() override {}

    virtual void update() override{};

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig() override;

    void setPolygons(const std::vector<PolygonCoord> &polygons);

    void setPolygon(const PolygonCoord &polygon);

    void setPositions(const std::vector<Coord> &positions, const std::vector<std::vector<Coord>> &holes);

    std::shared_ptr<GraphicsObjectInterface> getPolygonObject();

    std::shared_ptr<ShaderProgramInterface> getShaderProgram();

    void setColor(const Color &color);

  private:
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
    std::shared_ptr<ColorShaderInterface> shader;

    std::shared_ptr<Polygon2dInterface> polygon;
    std::shared_ptr<GraphicsObjectInterface> graphicsObject;
    std::shared_ptr<RenderConfig> renderConfig;

    const static int32_t SUBDIVISION_FACTOR_3D_DEFAULT = 4;

    const bool is3d;
};
