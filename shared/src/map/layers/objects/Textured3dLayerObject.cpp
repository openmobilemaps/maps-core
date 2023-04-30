/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Textured3dLayerObject.h"
#include "DateHelper.h"
#include "DoubleAnimation.h"
#include <cmath>
#include "RenderObject.h"
#include "CoordinateSystemIdentifiers.h"

Textured3dLayerObject::Textured3dLayerObject(std::shared_ptr<Quad3dInterface> quad, std::shared_ptr<SphereProjectionShaderInterface> shader,
                                             const std::shared_ptr<MapInterface> &mapInterface)
: quad(quad), shader(shader), mapInterface(mapInterface), conversionHelper(mapInterface->getCoordinateConverterHelper()),
renderConfig(std::make_shared<RenderConfig>(quad->asGraphicsObject(), 0)), graphicsObject(quad->asGraphicsObject()),
renderObject(std::make_shared<RenderObject>(graphicsObject))
{
}

void Textured3dLayerObject::setTileInfo(const int32_t x, const int32_t y, const int32_t z, const int32_t offset) {
    quad->setTileInfo(x, y, z, offset);
}

void Textured3dLayerObject::setRectCoord(const ::RectCoord &rectCoord) {
//    auto width = rectCoord.bottomRight.x - rectCoord.topLeft.x;
//    auto height = rectCoord.topLeft.y - rectCoord.bottomRight.y;
//    setPosition(rectCoord.topLeft, width, height);

    setPositions(QuadCoord(rectCoord.topLeft,
                           Coord(rectCoord.topLeft.systemIdentifier, rectCoord.bottomRight.x, rectCoord.topLeft.y,  rectCoord.topLeft.z),
                           rectCoord.bottomRight,
                           Coord(rectCoord.topLeft.systemIdentifier, rectCoord.topLeft.x, rectCoord.bottomRight.y,  rectCoord.topLeft.z)));
}

void Textured3dLayerObject::setPosition(const ::Coord &coord, double width, double height) {
    setPositions(QuadCoord(coord, Coord(coord.systemIdentifier, coord.x + width, coord.y, coord.z),
                           Coord(coord.systemIdentifier, coord.x + width, coord.y + height, coord.z),
                           Coord(coord.systemIdentifier, coord.x, coord.y + height, coord.z)));
}

void Textured3dLayerObject::setPositions(const ::QuadCoord &coords) {
    QuadCoord renderCoords = conversionHelper->convertQuad(CoordinateSystemIdentifiers::EPSG4326(), coords);
    setFrame(Quad2dD(
     Vec2D(renderCoords.topLeft.x, renderCoords.topLeft.y),
     Vec2D(renderCoords.topRight.x, renderCoords.topRight.y),
     Vec2D(renderCoords.bottomRight.x, renderCoords.bottomRight.y),
     Vec2D(renderCoords.bottomLeft.x, renderCoords.bottomLeft.y)));
}

void Textured3dLayerObject::setFrame(const ::Quad2dD &frame) { quad->setFrame(frame, RectD(0, 0, 1, 1)); }

void Textured3dLayerObject::update() {
    if (animation) {
        animation->update();
    }
}

std::vector<std::shared_ptr<RenderConfigInterface>> Textured3dLayerObject::getRenderConfig() { return {renderConfig}; }

void Textured3dLayerObject::setAlpha(float alpha) {
    if (shader) {
        shader->updateAlpha(alpha);
    }
    mapInterface->invalidate();
}

std::shared_ptr<Quad3dInterface> Textured3dLayerObject::getQuadObject() { return quad; }

std::shared_ptr<GraphicsObjectInterface> Textured3dLayerObject::getGraphicsObject() { return graphicsObject; }

std::shared_ptr<RenderObjectInterface> Textured3dLayerObject::getRenderObject() {
    return renderObject;
}

void Textured3dLayerObject::beginAlphaAnimation(double startAlpha, double targetAlpha, long long duration) {
    animation = std::make_shared<DoubleAnimation>(
        duration, startAlpha, targetAlpha, InterpolatorFunction::EaseIn, [=](double alpha) { this->setAlpha(alpha); },
        [=] {
            this->setAlpha(targetAlpha);
            this->animation = nullptr;
        });
    animation->start();
    mapInterface->invalidate();
}
