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
#include "RasterStyleAnimation.h"
#include "RenderObject.h"
#include <cmath>
#include <cassert>
#include "RectD.h"

Textured3dLayerObject::Textured3dLayerObject(std::shared_ptr<Quad2dInstancedInterface> quad,
                                             const std::shared_ptr<AlphaInstancedShaderInterface> &shader,
                                             const std::shared_ptr<MapInterface> &mapInterface,
                                             bool is3d)
        : quad(quad), shader(shader), mapInterface(mapInterface),
          conversionHelper(mapInterface->getCoordinateConverterHelper()),
          renderConfig(std::make_shared<RenderConfig>(quad->asGraphicsObject(), 0)), graphicsObject(quad->asGraphicsObject()),
          renderObject(std::make_shared<RenderObject>(graphicsObject)),
          is3d(is3d)
{
    int count = 1;
    quad->setInstanceCount(count);

    iconAlphas.resize(count, 0.0);
    iconRotations.resize(count, 0.0);
    iconScales.resize(count * 2, 0.0);
    iconPositions.resize(count * (is3d ? 3 : 2), 0.0);
    iconTextureCoordinates.resize(count * 4, 0.0);
    if (is3d) {
        iconOffsets.resize(count * 2, 0.0);
    }

    iconOffsets[0] = 0.0;
    iconOffsets[1] = 0.0;

    iconRotations[0] = 0.0;

    iconTextureCoordinates[0] = 0.0;
    iconTextureCoordinates[1] = 0.0;
    iconTextureCoordinates[2] = 1.0;
    iconTextureCoordinates[3] = 1.0;
}

//
//void Textured3dLayerObject::setRectCoord(const ::RectCoord &rectCoord) {
//    auto width = rectCoord.bottomRight.x - rectCoord.topLeft.x;
//    auto height = rectCoord.bottomRight.y - rectCoord.topLeft.y;
//    setPosition(rectCoord.topLeft, width, height);
//}

void Textured3dLayerObject::setPosition(const ::Coord &coord, double width, double height) {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    auto context = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    auto converter = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;

    if(!converter || !context) { return; }

    auto viewport = context->getViewportSize();
    auto renderCoord = converter->convertToRenderSystem(coord);

    iconScales[0] = 2.0 * width / double(viewport.x);
    iconScales[1] = 2.0 * height / double(viewport.y);

    if (is3d) {
        const double sinY = sin(renderCoord.y);
        const double cosY = cos(renderCoord.y);
        const double sinX = sin(renderCoord.x);
        const double cosX = cos(renderCoord.x);

        origin = Vec3D(sinY * cosX, cosY, -sinY * sinX);
    } else {
       // buffer[baseIndex]     = x_ - tileOrigin.x;
       // buffer[baseIndex + 1] = y_ - tileOrigin.y;
    }

    quad->setFrame(Quad2dD(Vec2D(-0.5, 0.5), Vec2D(0.5, 0.5), Vec2D(0.5, -0.5), Vec2D(-0.5, -0.5)), origin, true);
}

//
//    setPositions(QuadCoord(coord, Coord(coord.systemIdentifier, coord.x + width, coord.y, coord.z),
//                           Coord(coord.systemIdentifier, coord.x + width, coord.y + height, coord.z),
//                           Coord(coord.systemIdentifier, coord.x, coord.y + height, coord.z)));
//}

//void Textured3dLayerObject::setPositions(const ::QuadCoord &coords) {
//    QuadCoord renderCoords = conversionHelper->convertQuadToRenderSystem(coords);
//
//    const double cx = (renderCoords.bottomRight.x + renderCoords.topLeft.x) / 2.0;
//    const double cy = (renderCoords.bottomRight.y + renderCoords.topLeft.y) / 2.0;
//    const double cz = 0.0;
//
//    origin = Vec3D(cx, cy, cz);
//
//    if (is3d) {
//        origin.x = 1.0 * sin(cy) * cos(cx);
//        origin.y = 1.0 * cos(cy);
//        origin.z = -1.0 * sin(cy) * sin(cx);
//    }
//
//    quad->setFrame(Quad2dD(Vec2D(-0.5, 0.5), Vec2D(0.5, 0.5), Vec2D(0.5, -0.5), Vec2D(-0.5, -0.5)), tileOrigin, is3d);

//    auto transform = [&origin](const Coord coordinate) -> Vec3D {
//        const double x = coordinate.x;
//        const double y = coordinate.y;
//        const double z = coordinate.z;
//        return Vec3D(x, y, z);
//    };
//
//    setFrame(Quad3dD(transform(renderCoords.topLeft),
//                     transform(renderCoords.topRight),
//                     transform(renderCoords.bottomRight),
//                     transform(renderCoords.bottomLeft)), origin);
//}

void Textured3dLayerObject::setFrame(const ::Quad3dD &frame, const ::Vec3D & origin) {
    // TODO
    //quad->setFrame(frame, origin, is3d);
}

void Textured3dLayerObject::update() {
    int positionSize = is3d ? 3 : 2;

    quad->setTextureCoordinates(SharedBytes((int64_t)iconTextureCoordinates.data(), iconAlphas.size(), 4 * (int32_t)sizeof(float)));

    quad->setPositions(
            SharedBytes((int64_t) iconPositions.data(), (int32_t) iconAlphas.size(), iconPositions.size() * (int32_t) sizeof(float)));

    quad->setAlphas(
        SharedBytes((int64_t) iconAlphas.data(), (int32_t) iconAlphas.size(), (int32_t) sizeof(float)));

    quad->setScales(
        SharedBytes((int64_t) iconScales.data(), (int32_t) iconAlphas.size(), 2 * (int32_t) sizeof(float)));

    quad->setRotations(
        SharedBytes((int64_t) iconRotations.data(), (int32_t) iconAlphas.size(), 1 * (int32_t) sizeof(float)));

    if (is3d) {
        quad->setPositionOffset(
                SharedBytes((int64_t) iconOffsets.data(), (int32_t) iconAlphas.size(), 2 * (int32_t) sizeof(float)));
    }

    if (animation) {
        animation->update();
    }
}

std::vector<std::shared_ptr<RenderConfigInterface>> Textured3dLayerObject::getRenderConfig() { return {renderConfig}; }

void Textured3dLayerObject::setAlpha(float alpha) {
    // TODO

    iconAlphas[0] = 1.0;
    mapInterface->invalidate();
}

std::shared_ptr<Quad2dInstancedInterface> Textured3dLayerObject::getQuadObject() { return quad; }

std::shared_ptr<GraphicsObjectInterface> Textured3dLayerObject::getGraphicsObject() { return graphicsObject; }

std::shared_ptr<RenderObjectInterface> Textured3dLayerObject::getRenderObject() {
    return renderObject;
}

void Textured3dLayerObject::beginAlphaAnimation(double startAlpha, double targetAlpha, long long duration) {
    assert(shader != nullptr);
    animation = std::make_shared<DoubleAnimation>(
            duration, startAlpha, targetAlpha, InterpolatorFunction::EaseIn, [=](double alpha) { this->setAlpha(alpha); },
            [=] {
                this->setAlpha(targetAlpha);
                this->animation = nullptr;
            });
    animation->start();
    mapInterface->invalidate();
}

std::shared_ptr<ShaderProgramInterface> Textured3dLayerObject::getShader() {
    if (shader) {
        return shader->asShaderProgramInterface();
    }
    return nullptr;
}
