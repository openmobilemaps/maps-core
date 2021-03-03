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
#include "Vec2D.h"
#include <optional>

#include "AnimationInterface.h"

class Textured2dLayerObject : public LayerObjectInterface {
  public:
    Textured2dLayerObject(std::shared_ptr<Quad2dInterface> quad, std::shared_ptr<AlphaShaderInterface> shader,
                          const std::shared_ptr<MapInterface> &mapInterface);

    virtual ~Textured2dLayerObject() override {}

    virtual void update() override;

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig() override;

    void setPosition(const ::Coord &coord, double width, double height);

    void setPositions(const ::QuadCoord &coords);

    void setRectCoord(const ::RectCoord &rectCoord);

    void setAlpha(float alpha);

    std::shared_ptr<Quad2dInterface> getQuadObject();

    void beginAlphaAnimation(double startAlpha, double targetAlpha, long long duration);

  protected:
    void setFrame(const ::Quad2dD &frame);

  private:
    std::shared_ptr<Quad2dInterface> quad;
    std::shared_ptr<AlphaShaderInterface> shader;

    std::shared_ptr<RenderConfig> renderConfig;

    const std::shared_ptr<MapInterface> mapInterface;
    const std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;

    std::shared_ptr<AnimationInterface> animation;
};
