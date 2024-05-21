/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Polygon2dLayerObject.h"
#include "EarcutVec2D.h"
#include "PolygonHelper.h"

Polygon2dLayerObject::Polygon2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                           const std::shared_ptr<Polygon2dInterface> &polygon,
                                           const std::shared_ptr<ColorShaderInterface> &shader)
    : conversionHelper(conversionHelper)
    , shader(shader)
    , polygon(polygon)
    , graphicsObject(polygon->asGraphicsObject())
    , renderConfig(std::make_shared<RenderConfig>(graphicsObject, 0))
{}

std::vector<std::shared_ptr<RenderConfigInterface>> Polygon2dLayerObject::getRenderConfig() { return {renderConfig}; }

void Polygon2dLayerObject::setPositions(const std::vector<Coord> &positions, const std::vector<std::vector<Coord>> &holes) {
    setPolygon({positions, holes});
}

void Polygon2dLayerObject::setPolygon(const PolygonCoord &polygon) { setPolygons({polygon}); }

void Polygon2dLayerObject::setPolygons(const std::vector<PolygonCoord> &polygons) {
    std::vector<uint16_t> indices;
    std::vector<float> vertices;
    int32_t indexOffset = 0;

    std::vector<Vec2D> vecVertices;

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
                vecVertices.push_back(i);
            }
        }
    }

    PolygonHelper::subdivision(vecVertices, indices, 0.1, 4);

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

void Polygon2dLayerObject::setColor(const Color &color) { shader->setColor(color.r, color.g, color.b, color.a); }

std::shared_ptr<GraphicsObjectInterface> Polygon2dLayerObject::getPolygonObject() { return graphicsObject; }

std::shared_ptr<ShaderProgramInterface> Polygon2dLayerObject::getShaderProgram() { return shader->asShaderProgramInterface(); }
