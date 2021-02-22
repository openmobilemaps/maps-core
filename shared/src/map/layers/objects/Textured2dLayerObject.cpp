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

Textured2dLayerObject::Textured2dLayerObject(std::shared_ptr<Rectangle2dInterface> rectangle,
                                             std::shared_ptr<AlphaShaderInterface> shader,
                                             const std::shared_ptr<MapInterface> &mapInterface)
    : rectangle(rectangle)
    , shader(shader)
    , mapInterface(mapInterface)
    , conversionHelper(mapInterface->getCoordinateConverterHelper())
    , renderConfig(std::make_shared<RenderConfig>(rectangle->asGraphicsObject(), 0)) {}

void Textured2dLayerObject::setFrame(const ::RectD &frame) { rectangle->setFrame(frame, RectD(0, 0, 1, 1)); }

void Textured2dLayerObject::setPosition(const ::Coord &coord, double width, double height) {
    auto renderCoord = conversionHelper->convertToRenderSystem(coord);
    setFrame(RectD(renderCoord.x, renderCoord.y, width, height));
}

void Textured2dLayerObject::setRectCoord(const ::RectCoord &rectCoord) {
    auto renderCoord = conversionHelper->convertRectToRenderSystem(rectCoord);
    auto width = renderCoord.bottomRight.x - renderCoord.topLeft.x;
    auto height = renderCoord.bottomRight.y - renderCoord.topLeft.y;
    setFrame(RectD(renderCoord.topLeft.x, renderCoord.topLeft.y, width, height));
}

void Textured2dLayerObject::update() {
    applyAnimationState();
}

std::vector<std::shared_ptr<RenderConfigInterface>> Textured2dLayerObject::getRenderConfig() { return {renderConfig}; }

void Textured2dLayerObject::setAlpha(float alpha) { shader->updateAlpha(alpha); }

std::shared_ptr<Rectangle2dInterface> Textured2dLayerObject::getRectangleObject() { return rectangle; }

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
