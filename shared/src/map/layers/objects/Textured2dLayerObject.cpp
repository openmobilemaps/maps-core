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
#include "DoubleAnimation.h"
#include "RasterStyleAnimation.h"
#include "RenderObject.h"
#include <cmath>
#include <cassert>

Textured2dLayerObject::Textured2dLayerObject(std::shared_ptr<Quad2dInterface> quad,
                                             const std::shared_ptr<AlphaShaderInterface> &shader,
                                             const std::shared_ptr<MapInterface> &mapInterface,
                                             bool is3d)
        : quad(quad), shader(shader), rasterShader(nullptr), mapInterface(mapInterface),
          conversionHelper(mapInterface->getCoordinateConverterHelper()),
          renderConfig(std::make_shared<RenderConfig>(quad->asGraphicsObject(), 0)), graphicsObject(quad->asGraphicsObject()),
          renderObject(std::make_shared<RenderObject>(graphicsObject)),
          is3d(is3d) {}


Textured2dLayerObject::Textured2dLayerObject(std::shared_ptr<Quad2dInterface> quad,
                                             const std::shared_ptr<RasterShaderInterface> &rasterShader,
                                             const std::shared_ptr<MapInterface> &mapInterface,
                                             bool is3d)
        : quad(quad), shader(nullptr), rasterShader(rasterShader), mapInterface(mapInterface),
          conversionHelper(mapInterface->getCoordinateConverterHelper()),
          renderConfig(std::make_shared<RenderConfig>(quad->asGraphicsObject(), 0)), graphicsObject(quad->asGraphicsObject()),
          renderObject(std::make_shared<RenderObject>(graphicsObject)),
          is3d(is3d) {}


Textured2dLayerObject::Textured2dLayerObject(std::shared_ptr<Quad2dInterface> quad,
                                             const std::shared_ptr<MapInterface> &mapInterface,
                                             bool is3d)
        : quad(quad), shader(nullptr), rasterShader(nullptr), mapInterface(mapInterface),
          conversionHelper(mapInterface->getCoordinateConverterHelper()),
          renderConfig(std::make_shared<RenderConfig>(quad->asGraphicsObject(), 0)), graphicsObject(quad->asGraphicsObject()),
          renderObject(std::make_shared<RenderObject>(graphicsObject)),
          is3d(is3d) {}

void Textured2dLayerObject::setRectCoord(const ::RectCoord &rectCoord) {
    auto width = rectCoord.bottomRight.x - rectCoord.topLeft.x;
    auto height = rectCoord.bottomRight.y - rectCoord.topLeft.y;
    setPosition(rectCoord.topLeft, width, height);
}

void Textured2dLayerObject::setPosition(const ::Coord &coord, double width, double height) {
    setPositions(QuadCoord(coord, Coord(coord.systemIdentifier, coord.x + width, coord.y, coord.z),
                           Coord(coord.systemIdentifier, coord.x + width, coord.y + height, coord.z),
                           Coord(coord.systemIdentifier, coord.x, coord.y + height, coord.z)));
}

void Textured2dLayerObject::setPositions(const ::QuadCoord &coords) {
    QuadCoord renderCoords = conversionHelper->convertQuadToRenderSystem(coords);

    const double cx = (renderCoords.bottomRight.x + renderCoords.topLeft.x) / 2.0;
    const double cy = (renderCoords.bottomRight.y + renderCoords.topLeft.y) / 2.0;
    const double cz = 0.0;

    auto origin = Vec3D(cx, cy, cz);

    if (is3d) {
        origin.x = 1.0 * sin(cy) * cos(cx);
        origin.y = 1.0 * cos(cy);
        origin.z = -1.0 * sin(cy) * sin(cx);
    }

    auto transform = [&origin](const Coord coordinate) -> Vec3D {
        const double x = coordinate.x;
        const double y = coordinate.y;
        const double z = coordinate.z;
        return Vec3D(x, y, z);
    };

    setFrame(Quad3dD(transform(renderCoords.topLeft),
                     transform(renderCoords.topRight),
                     transform(renderCoords.bottomRight),
                     transform(renderCoords.bottomLeft)), origin);

//    if (is3d) {
//        double rx = is3d ? 1.0 * sin(cy) * cos(cx) : cx;
//        double ry = is3d ? 1.0 * cos(cy) : cy;
//        double rz = is3d ? -1.0 * sin(cy) * sin(cx) : 0.0;
//
//        auto origin = Vec3D(rx, ry, rz);
//
//        auto transform = [&origin](const Coord coordinate) -> Vec3D {
//            double x = 1.0 * sin(coordinate.y) * cos(coordinate.x) - origin.x;
//            double y =  1.0 * cos(coordinate.y) - origin.y;
//            double z = -1.0 * sin(coordinate.y) * sin(coordinate.x) - origin.z;
//            return Vec3D(x, y, z);
//        };
//
//        setFrame(Quad3dD(transform(renderCoords.topLeft),
//                         transform(renderCoords.topRight),
//                         transform(renderCoords.bottomRight),
//                         transform(renderCoords.bottomLeft)), origin);
//    } else {
//        auto origin = Vec3D(cx, cy, 0.0);
//        auto transform = [&origin](const Coord coordinate) -> Vec3D {
//            double x = coordinate.x - origin.x;
//            double y = coordinate.y - origin.y;
//            double z = coordinate.z - origin.z;
//            return Vec3D(x, y, z);
//        };
//
//        setFrame(Quad3dD(transform(renderCoords.topLeft),
//                         transform(renderCoords.topRight),
//                         transform(renderCoords.bottomRight),
//                         transform(renderCoords.bottomLeft)), origin);
//    }
}

void Textured2dLayerObject::setFrame(const ::Quad3dD &frame, const ::Vec3D & origin) { quad->setFrame(frame, RectD(0, 0, 1, 1), origin, is3d); }

void Textured2dLayerObject::update() {
    if (animation) {
        animation->update();
    }
}

std::vector<std::shared_ptr<RenderConfigInterface>> Textured2dLayerObject::getRenderConfig() { return {renderConfig}; }

void Textured2dLayerObject::setAlpha(float alpha) {
    // setAlpha only works for AlphaShaders
    // use setStyle for RasterShader
    assert(shader != nullptr);
    if (shader) {
        shader->updateAlpha(alpha);
    }
    mapInterface->invalidate();
}

void Textured2dLayerObject::setStyle(const RasterShaderStyle &style) {
    // setStyle only works for RasterShaders
    // use setAlpha for AlphaShader
    assert(rasterShader != nullptr);
    if (rasterShader) {
        rasterShader->setStyle(style);
    }
    mapInterface->invalidate();
}

std::shared_ptr<Quad2dInterface> Textured2dLayerObject::getQuadObject() { return quad; }

std::shared_ptr<GraphicsObjectInterface> Textured2dLayerObject::getGraphicsObject() { return graphicsObject; }

std::shared_ptr<RenderObjectInterface> Textured2dLayerObject::getRenderObject() {
    return renderObject;
}

void Textured2dLayerObject::beginAlphaAnimation(double startAlpha, double targetAlpha, long long duration) {
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

void Textured2dLayerObject::beginStyleAnimation(RasterShaderStyle start, RasterShaderStyle target, long long duration) {
    assert(rasterShader != nullptr);
    animation = std::make_shared<RasterStyleAnimation>(
                                                  duration, start, target, InterpolatorFunction::EaseIn, [=](RasterShaderStyle style) { this->setStyle(style); },
                                                  [=] {
                                                      this->setStyle(target);
                                                      this->animation = nullptr;
                                                  });
    animation->start();
    mapInterface->invalidate();
}

std::shared_ptr<ShaderProgramInterface> Textured2dLayerObject::getShader() {
    if (rasterShader) {
        return rasterShader->asShaderProgramInterface();
    }
    if (shader) {
        return shader->asShaderProgramInterface();
    }
    return nullptr;
}
