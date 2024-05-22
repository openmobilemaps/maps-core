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
#include "PolygonHelper.h"
#include <unordered_map>

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

    std::vector<Vec2F> vecVertices;

    for (auto const &polygon : polygons) {
        std::vector<std::vector<Vec2F>> renderCoords;
        std::vector<Vec2F> polygonCoords;
        for (const Coord &mapCoord : polygon.positions) {
            Coord renderCoord = conversionHelper->convertToRenderSystem(mapCoord);
            polygonCoords.push_back(Vec2F(renderCoord.x, renderCoord.y));
        }
        renderCoords.push_back(polygonCoords);

        for (const auto &hole : polygon.holes) {
            std::vector<::Vec2F> holeCoords;
            for (const Coord &coord : hole) {
                Coord renderCoord = conversionHelper->convertToRenderSystem(coord);
                holeCoords.push_back(Vec2F(renderCoord.x, renderCoord.y));
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
                vecVertices.push_back(i);
            }
        }
    }

    PolygonHelper::subdivision(vecVertices, indices, 0.0001, 3);

    for (const auto& v : vecVertices) {
        vertices.push_back(v.x);
        vertices.push_back(v.y);
        vertices.push_back(1.0f);
    #ifdef __APPLE__
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);
    #endif
    }

    auto attr = SharedBytes((int64_t)vertices.data(), (int32_t)vertices.size(), (int32_t)sizeof(float));
    auto ind = SharedBytes((int64_t)indices.data(), (int32_t)indices.size(), (int32_t)sizeof(uint16_t));

    polygon->setVertices(attr, ind);
}

std::shared_ptr<Polygon2dInterface> PolygonMaskObject::getPolygonObject() { return polygon; }
