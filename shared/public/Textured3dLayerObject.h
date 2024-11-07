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

#include "AlphaInstancedShaderInterface.h"
#include "Coord.h"
#include "CoordinateConversionHelperInterface.h"
#include "LayerObjectInterface.h"
#include "MapInterface.h"
#include "Quad2dInstancedInterface.h"
#include "QuadCoord.h"
#include "RectCoord.h"
#include "RenderConfig.h"
#include "RenderConfigInterface.h"
#include "RenderObjectInterface.h"
#include "Vec2D.h"
#include <optional>
#include "AnimationInterface.h"
#include "Quad3dD.h"

class Textured3dLayerObject : public LayerObjectInterface, public std::enable_shared_from_this<Textured3dLayerObject> {
  public:
    Textured3dLayerObject(std::shared_ptr<Quad2dInstancedInterface> quad,
                          const std::shared_ptr<AlphaInstancedShaderInterface> &shader,
                          const std::shared_ptr<MapInterface> &mapInterface,
                          bool is3d = false);

    virtual ~Textured3dLayerObject() override {}

    virtual void update() override;

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig() override;

    void setPosition(const ::Coord &coord, double width, double height);

  //  void setPositions(const ::QuadCoord &coords);

//    void setRectCoord(const ::RectCoord &rectCoord);

    void setAlpha(float alpha);

    std::shared_ptr<Quad2dInstancedInterface> getQuadObject();

    std::shared_ptr<GraphicsObjectInterface> getGraphicsObject();

    std::shared_ptr<RenderObjectInterface> getRenderObject();

    std::shared_ptr<ShaderProgramInterface> getShader();

    void beginAlphaAnimation(double startAlpha, double targetAlpha, long long duration);

  protected:
    void setFrame(const ::Quad3dD &frame, const ::Vec3D & origin);

  private:
    std::shared_ptr<Quad2dInstancedInterface> quad;
    std::shared_ptr<AlphaInstancedShaderInterface> shader;
    std::shared_ptr<GraphicsObjectInterface> graphicsObject;
    std::shared_ptr<RenderObjectInterface> renderObject;

    std::vector<float> iconPositions;
    std::vector<float> iconScales;
    std::vector<float> iconRotations;
    std::vector<float> iconAlphas;
    std::vector<float> iconOffsets;
    std::vector<float> iconTextureCoordinates;
    std::shared_ptr<TextureHolderInterface> texture;

    std::shared_ptr<RenderConfig> renderConfig;

    const std::shared_ptr<MapInterface> mapInterface;
    const std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;

    std::shared_ptr<AnimationInterface> animation;

    float alpha = 1.0;
    bool is3d = false;

    Vec3D origin = Vec3D(0.0, 0.0, 0.0);
};
