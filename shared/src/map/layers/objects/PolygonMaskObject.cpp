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

std::shared_ptr<PolygonMaskObjectInterface>
PolygonMaskObjectInterface::create(const std::shared_ptr<::GraphicsObjectFactoryInterface> &graphicsObjectFactory,
                                   const std::shared_ptr<::CoordinateConversionHelperInterface> &conversionHelper) {
    return std::make_shared<PolygonMaskObject>(graphicsObjectFactory, conversionHelper);
}

PolygonMaskObject::PolygonMaskObject(const std::shared_ptr<GraphicsObjectFactoryInterface> &graphicsObjectFactory,
                                     const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper)
    : conversionHelper(conversionHelper)
    , polygon(graphicsObjectFactory->createPolygonMask()) {}

void PolygonMaskObject::setPositions(const std::vector<Coord> &positions, const std::vector<std::vector<Coord>> &holes) {
    setPolygon({positions, holes});
}

void PolygonMaskObject::setPolygon(const ::PolygonCoord &polygon) { setPolygons({polygon}); }

void PolygonMaskObject::setPolygons(const std::vector<::PolygonCoord> &polygons) {
    std::vector<uint16_t> indices;
    std::vector<float> vertices;
    int32_t indexOffset = 0;

    for (auto const &polygon : polygons) {
        std::vector<std::vector<Vec2D>> renderCoords;
        std::vector<Vec2D> polygonCoords;
        for (const Coord &mapCoord : polygon.positions) {
            Coord renderCoord = conversionHelper->convertToRenderSystem(mapCoord);
            polygonCoords.push_back(Vec2D(renderCoord.x, renderCoord.y));
        }
        renderCoords.push_back(polygonCoords);

        for (const auto &hole : polygon.holes) {
            std::vector<::Vec2D> holeCoords;
            for (const Coord &coord : hole) {
                Coord renderCoord = conversionHelper->convertToRenderSystem(coord);
                holeCoords.push_back(Vec2D(renderCoord.x, renderCoord.y));
            }
            renderCoords.push_back(holeCoords);
        }
        std::vector<int32_t> curIndices = mapbox::earcut<int32_t>(renderCoords);

        for (auto const &index : curIndices) {
            indices.push_back(indexOffset + index);
        }

        for (auto const &list : renderCoords) {
            indexOffset += list.size();

            for(auto& i : list) {
                vertices.push_back(i.x);
                vertices.push_back(i.y);
                // fill for android z
                vertices.push_back(0.0);
#ifdef __APPLE__
                // are needed to fill metal vertex property (position, uv, normal)
                vertices.push_back(0.0);
                vertices.push_back(0.0);
                vertices.push_back(0.0);
#endif
            }
        }
    }

    auto attr = SharedBytes((int64_t)vertices.data(), (int32_t)vertices.size(), (int32_t)sizeof(float));
    auto ind = SharedBytes((int64_t)indices.data(), (int32_t)indices.size(), (int32_t)sizeof(uint16_t));

    polygon->setVertices(attr, ind);
}

std::shared_ptr<Polygon2dInterface> PolygonMaskObject::getPolygonObject() { return polygon; }
