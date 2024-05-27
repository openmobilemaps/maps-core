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

QuadMaskObject::QuadMaskObject(const std::shared_ptr<GraphicsObjectFactoryInterface> &graphicsObjectFactory,
                               const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                               bool is3D)
    : conversionHelper(conversionHelper)
    , quad(graphicsObjectFactory->createQuadMask(is3D)) {}

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
    quad->setFrame(Quad3dD(Vec3D(renderCoords.topLeft.x, renderCoords.topLeft.y, renderCoords.topLeft.z),
                           Vec3D(renderCoords.topRight.x, renderCoords.topRight.y, renderCoords.topRight.z),
                           Vec3D(renderCoords.bottomRight.x, renderCoords.bottomRight.y, renderCoords.bottomRight.z),
                           Vec3D(renderCoords.bottomLeft.x, renderCoords.bottomLeft.y, renderCoords.bottomLeft.z)),
                   RectD(0, 0, 1, 1));
}

std::shared_ptr<Quad2dInterface> QuadMaskObject::getQuadObject() { return quad; }

std::shared_ptr<MaskingObjectInterface> QuadMaskObject::getMaskObject() { return quad->asMaskingObject(); }
