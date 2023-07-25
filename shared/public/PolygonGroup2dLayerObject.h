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

#include "PolygonGroupShaderInterface.h"
#include "Coord.h"
#include "PolygonStyle.h"
#include "CoordinateConversionHelperInterface.h"
#include "LayerObjectInterface.h"
#include "PolygonGroup2dInterface.h"
#include "RenderConfig.h"
#include "Vec2D.h"
#include "Vec2F.h"

class PolygonGroup2dLayerObject : public LayerObjectInterface {
  public:
    PolygonGroup2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                      const std::shared_ptr<PolygonGroup2dInterface> &polygon, const std::shared_ptr<PolygonGroupShaderInterface> &shader);

    ~PolygonGroup2dLayerObject(){};

    virtual void update() override;

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig() override;

    void setVertices(const std::vector<std::tuple<std::vector<::Coord>, int>> & vertices, const std::vector<uint16_t> & indices);

    void setVertices(const std::vector<float> &verticesBuffer, const std::vector<uint16_t> & indices);

    void setStyles(const std::vector<PolygonStyle> &styles);

    std::shared_ptr<GraphicsObjectInterface> getPolygonObject();

    std::shared_ptr<ShaderProgramInterface> getShaderProgram();

  private:
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
    std::shared_ptr<PolygonGroup2dInterface> polygon;
    std::shared_ptr<PolygonGroupShaderInterface> shader;
    std::shared_ptr<RenderConfigInterface> renderConfig;
};
