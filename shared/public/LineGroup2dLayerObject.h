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
#include "Coord.h"
#include "LineStyle.h"
#include "CoordinateConversionHelperInterface.h"
#include "LayerObjectInterface.h"
#include "LineGroup2dInterface.h"
#include "RenderConfig.h"
#include "Vec2D.h"
#include "Vec3D.h"

class LineGroup2dLayerObject : public LayerObjectInterface {
  public:
    LineGroup2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                      const std::shared_ptr<LineGroup2dInterface> &line, const std::shared_ptr<LineGroupShaderInterface> &shader, bool is3d);

    ~LineGroup2dLayerObject(){};

    virtual void update() override;

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig() override;

    void setLines(const std::vector<std::tuple<std::vector<Vec2D>, int>> &lines, const int32_t systemIdentifier, const Vec3D & origin, LineCapType capType = LineCapType::BUTT, LineJoinType joinType = LineJoinType::MITER);

    void setLines(const std::vector<std::tuple<std::vector<Coord>, int>> &lines, const Vec3D & origin, LineCapType capType = LineCapType::BUTT, LineJoinType joinType = LineJoinType::MITER);

    void setStyles(const std::vector<LineStyle> &styles);

    void setScalingFactor(float factor);

    std::shared_ptr<GraphicsObjectInterface> getLineObject();

    std::shared_ptr<ShaderProgramInterface> getShaderProgram();

  private:
    void buildLines(const std::vector<std::tuple<std::vector<Vec3D>, int>> &renderCoords, const Vec3D & origin, LineCapType capType, LineJoinType joinType);
    void pushLineVertex(const Vec3D &p, const Vec3D &extrude, const float extrudeScale, const float side, const float prefixTotalLineLength, const int lineStyleIndex, const bool addTriangle, const bool reverse, uint32_t &vertexCount, int32_t &prePreIndex, int32_t &preIndex, std::vector<float> &lineAttributes, std::vector<uint32_t> &lineIndices);
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
    std::shared_ptr<LineGroup2dInterface> line;
    std::shared_ptr<LineGroupShaderInterface> shader;
    std::shared_ptr<RenderConfigInterface> renderConfig;
    bool is3d;
};
