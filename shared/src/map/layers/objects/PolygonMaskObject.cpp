/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "PolygonMaskObject.h"
#include "EarcutVec2D.h"

PolygonMaskObject::PolygonMaskObject(const std::shared_ptr<GraphicsObjectFactoryInterface> &graphicsObjectFactory,
                                     const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper)
        : conversionHelper(conversionHelper), polygon(graphicsObjectFactory->createPolygonMask()) {}

void
PolygonMaskObject::setPositions(const std::vector<Coord> &positions, const std::vector<std::vector<Coord>> &holes) {
    std::vector<std::vector<Vec2D>> renderCoords;
    std::vector<Vec2D> polygonCoords;
    for (const Coord &mapCoord : positions) {
        Coord renderCoord = conversionHelper->convertToRenderSystem(mapCoord);
        polygonCoords.push_back(Vec2D(renderCoord.x, renderCoord.y));
    }
    renderCoords.push_back(polygonCoords);

    for (const auto &hole : holes) {
        std::vector<::Vec2D> holeCoords;
        for (const Coord &coord : hole) {
            Coord renderCoord = conversionHelper->convertToRenderSystem(coord);
            holeCoords.push_back(Vec2D(renderCoord.x, renderCoord.y));
        }
        renderCoords.push_back(holeCoords);
    }
    std::vector<int32_t> indices = mapbox::earcut<int32_t>(renderCoords);

    std::vector<Vec2D> flatCoords;
    for (auto const &list: renderCoords) {
        flatCoords.insert(flatCoords.end(), list.begin(), list.end());
    }

    polygon->setVertices(flatCoords, indices);
}

std::shared_ptr<Polygon2dInterface> PolygonMaskObject::getPolygonObject() {
    return polygon;
}
