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
#include "PolygonCoord.h"
#include "RectCoord.h"
#include "RenderConfig.h"
#include "RenderConfigInterface.h"
#include "RenderObjectInterface.h"
#include "Vec2D.h"
#include <optional>
#include "RasterShaderInterface.h"
#include "AnimationInterface.h"
#include "RasterShaderStyle.h"
#include "TexturedPolygonInterface.h"

class Textured2dLayerObject : public LayerObjectInterface, public std::enable_shared_from_this<Textured2dLayerObject> {
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

    Textured2dLayerObject(std::shared_ptr<TexturedPolygonInterface> texturedPolygon,
                          const std::shared_ptr<AlphaShaderInterface> &shader,
                          const std::shared_ptr<MapInterface> &mapInterface,
                          bool is3d = false);

    Textured2dLayerObject(std::shared_ptr<TexturedPolygonInterface> texturedPolygon,
                          const std::shared_ptr<RasterShaderInterface> &rasterShader,
                          const std::shared_ptr<MapInterface> &mapInterface,
                          bool is3d = false);

    Textured2dLayerObject(std::shared_ptr<TexturedPolygonInterface> texturedPolygon,
                          const std::shared_ptr<MapInterface> &mapInterface,
                          bool is3d = false);

    virtual ~Textured2dLayerObject() override {}

    virtual void update() override;

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig() override;

    void setPosition(const ::Coord &coord, double width, double height);

    void setPositions(const ::QuadCoord &coords);

    void setRectCoord(const ::RectCoord &rectCoord);
    void setRectCoord(const ::RectCoord &rectCoord, double overlapFactor);
    void setPolygons(const std::vector<::PolygonCoord> &polygons, const ::RectCoord &textureBounds);
    void setPolygons(const std::vector<::PolygonCoord> &polygons, const ::RectCoord &textureBounds, double overlapFactor);

    void setAlpha(float alpha);
    
    void setStyle(const RasterShaderStyle &style);

    std::shared_ptr<Quad2dInterface> getQuadObject();

    void setSubdivisionFactor(int32_t factor);

    void setMinMagFilter(TextureFilterType filterType);

    void loadTexture(const std::shared_ptr<RenderingContextInterface> &context,
                     const std::shared_ptr<TextureHolderInterface> &textureHolder);

    void loadDualTexture(const std::shared_ptr<RenderingContextInterface> &context,
                         const std::shared_ptr<TextureHolderInterface> &textureHolder,
                         const std::shared_ptr<TextureHolderInterface> &elevationHolder);

    void loadTextures(const std::shared_ptr<RenderingContextInterface> &context,
                      const std::shared_ptr<TextureHolderInterface> &textureHolder,
                      const std::shared_ptr<TextureHolderInterface> &lookupHolder,
                      const std::shared_ptr<TextureHolderInterface> &elevationHolder);

    void removeTexture();

    std::shared_ptr<GraphicsObjectInterface> getGraphicsObject();
    
    std::shared_ptr<RenderObjectInterface> getRenderObject();

    std::shared_ptr<ShaderProgramInterface> getShader();

    void beginAlphaAnimation(double startAlpha, double targetAlpha, int64_t duration);
    
    void beginStyleAnimation(RasterShaderStyle start, RasterShaderStyle target, int64_t duration);

  protected:
    void setFrame(const ::Quad3dD &frame, const ::Vec3D & origin);

  private:
    std::shared_ptr<Quad2dInterface> quad;
    std::shared_ptr<TexturedPolygonInterface> texturedPolygon;
    std::shared_ptr<AlphaShaderInterface> shader;
    std::shared_ptr<GraphicsObjectInterface> graphicsObject;
    std::shared_ptr<RenderObjectInterface> renderObject;
    std::shared_ptr<RasterShaderInterface> rasterShader;

    std::shared_ptr<RenderConfig> renderConfig;

    const std::shared_ptr<MapInterface> mapInterface;
    const std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;

    std::shared_ptr<AnimationInterface> animation;

    bool is3d = false;
};
