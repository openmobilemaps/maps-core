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
                                   const std::shared_ptr<::CoordinateConversionHelperInterface> &conversionHelper,
                                   bool is3D) {
    return std::make_shared<PolygonMaskObject>(graphicsObjectFactory, conversionHelper, is3D);
}

PolygonMaskObject::PolygonMaskObject(const std::shared_ptr<GraphicsObjectFactoryInterface> &graphicsObjectFactory,
                                     const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                     bool is3D)
    : conversionHelper(conversionHelper)
    , polygon(graphicsObjectFactory->createPolygonMaskTessellated(is3D))
    , is3D(is3D) {}

void PolygonMaskObject::setPositions(const std::vector<Coord> &positions,
                                     const Vec3D & origin,
                                     const std::vector<std::vector<Coord>> &holes) {
    setPolygon({positions, holes}, origin, std::nullopt);
}

void PolygonMaskObject::setPolygon(const ::PolygonCoord &polygon,
                                   const Vec3D & origin, std::optional<float> subdivisionFactor) {
    setPolygons({polygon}, origin, subdivisionFactor);
}

void PolygonMaskObject::setPolygons(const std::vector<::PolygonCoord> &polygons,
                                    const Vec3D & origin,
                                    std::optional<float> subdivisionFactor) {
    std::vector<uint16_t> indices;
    std::vector<float> vertices;
    int32_t indexOffset = 0;

    std::vector<Vec2D> vecVertices;
    mapbox::detail::Earcut<int32_t> earcutter;

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
        earcutter(renderCoords);
        std::vector<int32_t> curIndices = std::move(earcutter.indices);

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
    
    for (const auto& v : vecVertices) {
        double rx = origin.x;
        double ry = origin.y;
        double rz = origin.z;

        double x = is3D ? (1.0 * sin(v.y) * cos(v.x) - rx) : v.x - rx ;
        double y = is3D ? (1.0 * cos(v.y) - ry) : v.y - ry;
        double z = is3D ? (-1.0 * sin(v.y) * sin(v.x) - rz) : 0.0;

        // Position
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
    #ifdef __APPLE__
        vertices.push_back(0.0f);
    #endif
        // Frame Coord
        vertices.push_back(v.x);
        vertices.push_back(v.y);
    }

    auto attr = SharedBytes((int64_t)vertices.data(), (int32_t)vertices.size(), (int32_t)sizeof(float));
    auto ind = SharedBytes((int64_t)indices.data(), (int32_t)indices.size(), (int32_t)sizeof(uint16_t));
    
    polygon->setSubdivisionFactor((int32_t)(subdivisionFactor.value_or(1.0f)));
    polygon->setVertices(attr, ind, origin, is3D);
}

std::shared_ptr<Polygon2dInterface> PolygonMaskObject::getPolygonObject() { return polygon; }
