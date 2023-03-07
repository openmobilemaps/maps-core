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
#include "CoordinateConversionHelperInterface.h"
#include "LayerObjectInterface.h"
#include "MapInterface.h"
#include "Quad3dInterface.h"
#include "QuadCoord.h"
#include "RectCoord.h"
#include "RenderConfig.h"
#include "RenderConfigInterface.h"
#include "RenderObjectInterface.h"
#include <optional>

#include "AnimationInterface.h"

class Textured3dLayerObject : public LayerObjectInterface {
public:
    Textured3dLayerObject(std::shared_ptr<Quad3dInterface> quad, std::shared_ptr<SphereProjectionShaderInterface> shader,
                          const std::shared_ptr<MapInterface> &mapInterface);

    virtual ~Textured3dLayerObject() override {}

    virtual void update() override;

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig() override;

    void setPosition(const ::Coord &coord, double width, double height);

    void setPositions(const ::QuadCoord &coords);

    void setRectCoord(const ::RectCoord &rectCoord);

    void setAlpha(float alpha);

    std::shared_ptr<Quad3dInterface> getQuadObject();

    std::shared_ptr<GraphicsObjectInterface> getGraphicsObject();

    std::shared_ptr<RenderObjectInterface> getRenderObject();

    void beginAlphaAnimation(double startAlpha, double targetAlpha, long long duration);

protected:
    void setFrame(const ::Quad2dD &frame);

private:
    std::shared_ptr<Quad3dInterface> quad;
    std::shared_ptr<SphereProjectionShaderInterface> shader;
    std::shared_ptr<GraphicsObjectInterface> graphicsObject;
    std::shared_ptr<RenderObjectInterface> renderObject;

    std::shared_ptr<RenderConfig> renderConfig;

    const std::shared_ptr<MapInterface> mapInterface;
    const std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;

    std::shared_ptr<AnimationInterface> animation;
};
