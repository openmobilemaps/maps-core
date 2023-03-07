/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Polygon3dLayerObject.h"
#include "RenderVerticesDescription.h"
#include "Logger.h"
#include "earcut.hpp"
#include "EarcutVec2D.h"
#include <cstdint>

Polygon3dLayerObject::Polygon3dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                                     const std::shared_ptr<Polygon3dInterface> &polygon,
                                                     const std::shared_ptr<SphereProjectionShaderInterface> &shader)
    : conversionHelper(conversionHelper)
    , polygon(polygon)
    , graphicsObject(polygon->asGraphicsObject())
    , shader(shader)
    , renderConfig(std::make_shared<RenderConfig>(polygon->asGraphicsObject(), 0)) {}

void Polygon3dLayerObject::update() {}

std::vector<std::shared_ptr<RenderConfigInterface>> Polygon3dLayerObject::getRenderConfig() { return {renderConfig}; }

void Polygon3dLayerObject::setPolygons(const std::vector<PolygonCoord> &polygons) {
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

                // are needed to fill metal vertex property (position, uv, normal)
                vertices.push_back(0.0);
                vertices.push_back(0.0);

                vertices.push_back(0.0);
            }
        }
    }

    auto attr = SharedBytes((int64_t)vertices.data(), (int32_t)vertices.size(), (int32_t)sizeof(float));
    auto ind = SharedBytes((int64_t)indices.data(), (int32_t)indices.size(), (int32_t)sizeof(uint16_t));
    polygon->setVertices(attr, ind);
}

void Polygon3dLayerObject::setTilePolygons(const std::vector<PolygonCoord> &polygons, const RectCoord &bounds) {
    std::vector<uint16_t> indices;
    std::vector<float> vertices;
    int32_t indexOffset = 0;

    auto topLeft = conversionHelper->convertToRenderSystem(bounds.topLeft);
    auto bottomRight = conversionHelper->convertToRenderSystem(bounds.bottomRight);

    auto minX = std::min(topLeft.x, bottomRight.x);
    auto maxX = std::max(topLeft.x, bottomRight.x);

    auto minY = std::min(topLeft.y, bottomRight.y);
    auto maxY = std::max(topLeft.y, bottomRight.y);

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

                // are needed to fill metal vertex property (position, uv, normal)
                vertices.push_back((i.x - minX) / (maxX - minX));
                vertices.push_back((i.y - minY) / (maxY - minY));

                vertices.push_back(0.0);
                vertices.push_back(0.0);
            }
        }
    }

    auto attr = SharedBytes((int64_t)vertices.data(), (int32_t)vertices.size(), (int32_t)sizeof(float));
    auto ind = SharedBytes((int64_t)indices.data(), (int32_t)indices.size(), (int32_t)sizeof(uint16_t));
    polygon->setVertices(attr, ind);
}


std::shared_ptr<Polygon3dInterface> Polygon3dLayerObject::getPolygonObject() { return polygon; }

std::shared_ptr<GraphicsObjectInterface> Polygon3dLayerObject::getGraphicsObject() { return graphicsObject; }

std::shared_ptr<ShaderProgramInterface> Polygon3dLayerObject::getShaderProgram() { return shader->asShaderProgramInterface(); }
