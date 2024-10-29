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

#include "AlphaShaderInterface.h"
#include "Coord.h"
#include "CoordinateConversionHelperInterface.h"
#include "LayerObjectInterface.h"
#include "MapInterface.h"
#include "Quad2dInterface.h"
#include "QuadCoord.h"
#include "RectCoord.h"
#include "RenderConfig.h"
#include "RenderConfigInterface.h"
#include "RenderObjectInterface.h"
#include "Vec2D.h"
#include <optional>
#include "RasterShaderInterface.h"
#include "AnimationInterface.h"
#include "RasterShaderStyle.h"

class Textured2dLayerObject : public LayerObjectInterface {
  public:
    Textured2dLayerObject(std::shared_ptr<Quad2dInterface> quad,
                          const std::shared_ptr<AlphaShaderInterface> &shader,
                          const std::shared_ptr<MapInterface> &mapInterface,
                          bool is3d = false);
    
    Textured2dLayerObject(std::shared_ptr<Quad2dInterface> quad, 
                          const std::shared_ptr<RasterShaderInterface> &rasterShader,
                          const std::shared_ptr<MapInterface> &mapInterface,
                          bool is3d = false);
    
    Textured2dLayerObject(std::shared_ptr<Quad2dInterface> quad, 
                          const std::shared_ptr<MapInterface> &mapInterface,
                          bool is3d = false);

    virtual ~Textured2dLayerObject() override {}

    virtual void update() override;

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig() override;

    void setPosition(const ::Coord &coord, double width, double height);

    void setPositions(const ::QuadCoord &coords);

    void setRectCoord(const ::RectCoord &rectCoord);

    void setAlpha(float alpha);
    
    void setStyle(const RasterShaderStyle &style);

    std::shared_ptr<Quad2dInterface> getQuadObject();

    std::shared_ptr<GraphicsObjectInterface> getGraphicsObject();
    
    std::shared_ptr<RenderObjectInterface> getRenderObject();

    std::shared_ptr<ShaderProgramInterface> getShader();

    void beginAlphaAnimation(double startAlpha, double targetAlpha, long long duration);
    
    void beginStyleAnimation(RasterShaderStyle start, RasterShaderStyle target, long long duration);

  protected:
    void setFrame(const ::Quad3dD &frame, const ::Vec3D & origin);

  private:
    std::shared_ptr<Quad2dInterface> quad;
    std::shared_ptr<AlphaShaderInterface> shader;
    std::shared_ptr<GraphicsObjectInterface> graphicsObject;
    std::shared_ptr<RenderObjectInterface> renderObject;
    std::shared_ptr<RasterShaderInterface> rasterShader;

    std::shared_ptr<RenderConfig> renderConfig;

    const std::shared_ptr<MapInterface> mapInterface;
    const std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;

    std::shared_ptr<AnimationInterface> animation;

    float alpha = 1.0;
    bool is3d = false;
};
