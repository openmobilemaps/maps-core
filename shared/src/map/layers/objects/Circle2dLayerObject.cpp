/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Circle2dLayerObject.h"
#include "ColorCircleShaderInterface.h"
#include "ColorShaderInterface.h"
#include "Quad2dInterface.h"
#include <cmath>

Circle2dLayerObject::Circle2dLayerObject(const std::shared_ptr<MapInterface> &mapInterface)
        : is3d(mapInterface->is3d()),
          conversionHelper(mapInterface->getCoordinateConverterHelper()),
          shader(mapInterface->is3d() ? mapInterface->getShaderFactory()->createUnitSphereColorCircleShader()
                                      : mapInterface->getShaderFactory()->createColorCircleShader()),
          quad(mapInterface->getGraphicsObjectFactory()->createQuad(shader->asShaderProgramInterface())),
          graphicsObject(quad->asGraphicsObject()), renderConfig(std::make_shared<RenderConfig>(graphicsObject, 0)) {}

std::vector<std::shared_ptr<RenderConfigInterface>> Circle2dLayerObject::getRenderConfig() { return {renderConfig}; }

void Circle2dLayerObject::setColor(Color color) { shader->setColor(color.r, color.g, color.b, color.a); }

void Circle2dLayerObject::setPosition(Coord position, double radius) {
    Coord renderPos = conversionHelper->convertToRenderSystem(position);
    auto origin = Vec3D(renderPos.x, renderPos.y, renderPos.z);
    if (is3d) {
        origin.x = 1.0 * sin(renderPos.y) * cos(renderPos.x);
        origin.y = 1.0 * cos(renderPos.y);
        origin.z = -1.0 * sin(renderPos.y) * sin(renderPos.x);
    }

    quad->setFrame(Quad3dD(Vec3D(renderPos.x - radius, renderPos.y + radius, 0.0),
                           Vec3D(renderPos.x + radius, renderPos.y + radius, 0.0),
                           Vec3D(renderPos.x + radius, renderPos.y - radius, 0.0),
                           Vec3D(renderPos.x - radius, renderPos.y - radius, 0.0)),
                   RectD(0, 0, 1, 1), origin, is3d);
}

std::shared_ptr<Quad2dInterface> Circle2dLayerObject::getQuadObject() { return quad; }

std::shared_ptr<GraphicsObjectInterface> Circle2dLayerObject::getGraphicsObject() { return graphicsObject; }

