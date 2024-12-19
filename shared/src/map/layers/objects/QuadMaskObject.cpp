/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "QuadMaskObject.h"
#include "Vec2D.h"
#include <cmath>

QuadMaskObject::QuadMaskObject(const std::shared_ptr<GraphicsObjectFactoryInterface> &graphicsObjectFactory,
                               const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                               bool is3D)
    : conversionHelper(conversionHelper)
    , quad(graphicsObjectFactory->createQuadMask(is3D))
    , is3D(is3D) {}

QuadMaskObject::QuadMaskObject(const std::shared_ptr<GraphicsObjectFactoryInterface> &graphicsObjectFactory,
                               const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                               const RectCoord &rectCoord,
                               bool is3D)
    : conversionHelper(conversionHelper)
    , quad(graphicsObjectFactory->createQuadMask(is3D)) {
    setRectCoord(rectCoord);
}

void QuadMaskObject::setRectCoord(const RectCoord &rectCoord) {
    auto width = rectCoord.bottomRight.x - rectCoord.topLeft.x;
    auto height = rectCoord.bottomRight.y - rectCoord.topLeft.y;
    setPosition(rectCoord.topLeft, width, height);
}

void QuadMaskObject::setPosition(const ::Coord &coord, double width, double height) {
    setPositions(QuadCoord(coord, Coord(coord.systemIdentifier, coord.x + width, coord.y, coord.z),
                           Coord(coord.systemIdentifier, coord.x + width, coord.y + height, coord.z),
                           Coord(coord.systemIdentifier, coord.x, coord.y + height, coord.z)));
}

void QuadMaskObject::setPositions(const QuadCoord &coords) {
    QuadCoord renderCoords = conversionHelper->convertQuadToRenderSystem(coords);

    double cx = (renderCoords.bottomRight.x + renderCoords.topLeft.x) / 2.0;
    double cy = (renderCoords.bottomRight.y + renderCoords.topLeft.y) / 2.0;
    double cz = 0.0;

    auto origin = Vec3D(cx, cy, cz);

    if (is3D) {
        origin.x = 1.0 * sin(cy) * cos(cx);
        origin.y = 1.0 * cos(cy);
        origin.z = -1.0 * sin(cy) * sin(cx);
    }
    
    auto transform = [&origin](const Coord coordinate) -> Vec3D {
        double x = coordinate.x;
        double y = coordinate.y;
        double z = coordinate.z;
        return Vec3D(x, y, z);
    };

    quad->setFrame(Quad3dD(transform(renderCoords.topLeft),
                     transform(renderCoords.topRight),
                     transform(renderCoords.bottomRight),
                           transform(renderCoords.bottomLeft)), RectD(0, 0, 1, 1), origin, is3D);
//    if (is3D) {
//        double rx = is3D ? 1.0 * sin(cy) * cos(cx) : cx;
//        double ry = is3D ? 1.0 * cos(cy) : cy;
//        double rz = is3D ? -1.0 * sin(cy) * sin(cx) : 0.0;
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
//        quad->setFrame(Quad3dD(transform(renderCoords.topLeft),
//                         transform(renderCoords.topRight),
//                         transform(renderCoords.bottomRight),
//                         transform(renderCoords.bottomLeft)), RectD(0, 0, 1, 1), origin);
//    } else {
//        auto origin = Vec3D(cx, cy, 0.0);
//        auto transform = [&origin](const Coord coordinate) -> Vec3D {
//            double x = coordinate.x - origin.x;
//            double y = coordinate.y - origin.y;
//            double z = coordinate.z - origin.z;
//            return Vec3D(x, y, z);
//        };
//
//        quad->setFrame(Quad3dD(transform(renderCoords.topLeft),
//                         transform(renderCoords.topRight),
//                         transform(renderCoords.bottomRight),
//                         transform(renderCoords.bottomLeft)), RectD(0, 0, 1, 1), origin);
//    }
}

std::shared_ptr<Quad2dInterface> QuadMaskObject::getQuadObject() { return quad; }

std::shared_ptr<MaskingObjectInterface> QuadMaskObject::getMaskObject() { return quad->asMaskingObject(); }
