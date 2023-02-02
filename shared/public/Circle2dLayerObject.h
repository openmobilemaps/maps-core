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

#include "MapInterface.h"
#include "LayerObjectInterface.h"
#include "Color.h"
#include "Coord.h"
#include "RenderConfig.h"

class Circle2dLayerObject : public LayerObjectInterface {
public:
    Circle2dLayerObject(const std::shared_ptr<MapInterface> &mapInterface);

    virtual void update() override {};

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig() override;

    virtual void setColor(Color color);

    virtual void setPosition(Coord position, double radius);

    virtual std::shared_ptr<Quad2dInterface> getQuadObject();

    virtual std::shared_ptr<GraphicsObjectInterface> getGraphicsObject();

private:
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
    std::shared_ptr<ColorCircleShaderInterface> shader;
    std::shared_ptr<Quad2dInterface> quad;
    std::shared_ptr<GraphicsObjectInterface> graphicsObject;
    std::shared_ptr<RenderConfig> renderConfig;

    double radius = 0;
};
