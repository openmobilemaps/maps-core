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

#include "SphereProjectionShaderInterface.h"
#include "Coord.h"
#include "PolygonStyle.h"
#include "CoordinateConversionHelperInterface.h"
#include "LayerObjectInterface.h"
#include "Polygon3dInterface.h"
#include "RenderConfig.h"
#include "PolygonCoord.h"
#include "Vec3D.h"
#include "RectCoord.h"

class Polygon3dLayerObject : public LayerObjectInterface {
  public:
    Polygon3dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                      const std::shared_ptr<Polygon3dInterface> &polygon,
                              const std::shared_ptr<SphereProjectionShaderInterface> &shader);

    ~Polygon3dLayerObject(){};

    virtual void update() override;

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig() override;

    void setPolygons(const std::vector<PolygonCoord> &polygons);

    void setTilePolygons(const std::vector<PolygonCoord> &polygons, const RectCoord &bounds);

    std::shared_ptr<GraphicsObjectInterface> getGraphicsObject();

    std::shared_ptr<Polygon3dInterface> getPolygonObject();

    std::shared_ptr<ShaderProgramInterface> getShaderProgram();

  private:
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;

    std::shared_ptr<GraphicsObjectInterface> graphicsObject;
    std::shared_ptr<Polygon3dInterface> polygon;
    std::shared_ptr<SphereProjectionShaderInterface> shader;
    std::shared_ptr<RenderConfigInterface> renderConfig;
};
