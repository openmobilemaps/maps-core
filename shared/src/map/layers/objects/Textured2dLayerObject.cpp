/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Textured2dLayerObject.h"
#include "DateHelper.h"
#include <cmath>

Textured2dLayerObject::Textured2dLayerObject(std::shared_ptr<Quad2dInterface> quad,
                                             std::shared_ptr<AlphaShaderInterface> shader,
                                             const std::shared_ptr<MapInterface> &mapInterface)
    : quad(quad)
    , shader(shader)
    , mapInterface(mapInterface)
    , conversionHelper(mapInterface->getCoordinateConverterHelper())
    , renderConfig(std::make_shared<RenderConfig>(quad->asGraphicsObject(), 0)) {}

void Textured2dLayerObject::setRectCoord(const ::RectCoord &rectCoord) {
    auto width = rectCoord.bottomRight.x - rectCoord.topLeft.x;
    auto height = rectCoord.bottomRight.y - rectCoord.topLeft.y;
    setPosition(rectCoord.topLeft, width, height);
}

void Textured2dLayerObject::setPosition(const ::Coord &coord, double width, double height) {
    setPositions(QuadCoord(coord,
                           Coord(coord.systemIdentifier, coord.x + width, coord.y, coord.z),
                           Coord(coord.systemIdentifier, coord.x + width, coord.y + height, coord.z),
                           Coord(coord.systemIdentifier, coord.x, coord.y + height, coord.z)));
}

void Textured2dLayerObject::setPositions(const ::QuadCoord &coords) {
    QuadCoord renderCoords = conversionHelper->convertQuadToRenderSystem(coords);
    setFrame(Quad2dD(Vec2D(renderCoords.topLeft.x, renderCoords.topLeft.y),
                     Vec2D(renderCoords.topRight.x, renderCoords.topRight.y),
                     Vec2D(renderCoords.bottomRight.x, renderCoords.bottomRight.y),
                     Vec2D(renderCoords.bottomLeft.x, renderCoords.bottomLeft.y)));
}

void Textured2dLayerObject::setFrame(const ::Quad2dD &frame) {
    quad->setFrame(frame, RectD(0, 0, 1, 1));
}

void Textured2dLayerObject::update() {
    applyAnimationState();
}

std::vector<std::shared_ptr<RenderConfigInterface>> Textured2dLayerObject::getRenderConfig() { return {renderConfig}; }

void Textured2dLayerObject::setAlpha(float alpha) { shader->updateAlpha(alpha); }

std::shared_ptr<Quad2dInterface> Textured2dLayerObject::getQuadObject() { return quad; }

void Textured2dLayerObject::beginAlphaAnimation(double startAlpha, double targetAlpha, long long duration) {
    alphaAnimation = { startAlpha, targetAlpha, DateHelper::currentTimeMillis(), duration };
}

void Textured2dLayerObject::applyAnimationState() {
    if (!alphaAnimation) return;

    long long currentTime = DateHelper::currentTimeMillis();
    double progress = (double)(currentTime - alphaAnimation->startTime) / alphaAnimation->duration;

    if (progress >= 1) {
        setAlpha(alphaAnimation->targetAlpha);
        this->alphaAnimation = std::nullopt;
    } else {
        auto newAlpha = alphaAnimation->startAlpha + (alphaAnimation->targetAlpha - alphaAnimation->startAlpha) * std::pow(progress, 2);
        setAlpha(newAlpha);
    }
    mapInterface->invalidate();
}
