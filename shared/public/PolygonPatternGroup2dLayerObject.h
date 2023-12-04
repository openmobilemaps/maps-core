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

#include "PolygonPatternGroupShaderInterface.h"
#include "Coord.h"
#include "PolygonStyle.h"
#include "CoordinateConversionHelperInterface.h"
#include "LayerObjectInterface.h"
#include "PolygonPatternGroup2dInterface.h"
#include "RenderConfig.h"
#include "Vec2D.h"
#include "Vec2F.h"

class PolygonPatternGroup2dLayerObject : public LayerObjectInterface {
  public:
    PolygonPatternGroup2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                      const std::shared_ptr<PolygonPatternGroup2dInterface> &polygon, const std::shared_ptr<PolygonPatternGroupShaderInterface> &shader);

    ~PolygonPatternGroup2dLayerObject(){};

    virtual void update() override;

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig() override;

    void setVertices(const std::vector<std::tuple<std::vector<::Coord>, int>> & vertices, const std::vector<uint16_t> & indices);

    void setVertices(const std::vector<float> &verticesBuffer, const std::vector<uint16_t> & indices);

    void setOpacities(const std::vector<float> &opacities);

    void setTextureCoordinates(const std::vector<float> &coordinates);

    void setScalingFactor(float factor);

    void setScalingFactor(Vec2F factor);

    void loadTexture(const /*not-null*/ std::shared_ptr<::RenderingContextInterface> & context, const /*not-null*/ std::shared_ptr<TextureHolderInterface> & textureHolder);

    std::shared_ptr<GraphicsObjectInterface> getPolygonObject();

    std::shared_ptr<ShaderProgramInterface> getShaderProgram();

  private:
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
    std::shared_ptr<PolygonPatternGroup2dInterface> polygon;
    std::shared_ptr<PolygonPatternGroupShaderInterface> shader;
    std::shared_ptr<RenderConfigInterface> renderConfig;
};
