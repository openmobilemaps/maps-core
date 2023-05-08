/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "TexturedPolygon3dLayerObject.h"
#include "DateHelper.h"
#include "DoubleAnimation.h"
#include <cmath>
#include "RenderObject.h"
#include "CoordinateSystemIdentifiers.h"
#include "Vec2D.h"
#include "EarcutVec2D.h"

TexturedPolygon3dLayerObject::TexturedPolygon3dLayerObject(std::shared_ptr<Polygon3dInterface> polygon, std::shared_ptr<SphereProjectionShaderInterface> shader,
                                             const std::shared_ptr<MapInterface> &mapInterface)
: polygon(polygon), shader(shader), mapInterface(mapInterface), conversionHelper(mapInterface->getCoordinateConverterHelper()),
renderConfig(std::make_shared<RenderConfig>(polygon->asGraphicsObject(), 0)), graphicsObject(polygon->asGraphicsObject()),
renderObject(std::make_shared<RenderObject>(graphicsObject))
{
}

void TexturedPolygon3dLayerObject::setPositions(const std::vector<Coord> &positions, const std::vector<std::vector<Coord>> &holes, const ::RectCoord &bounds) {
    setPolygon({positions, holes}, bounds);
}

void TexturedPolygon3dLayerObject::setPolygon(const ::PolygonCoord &polygon, const ::RectCoord &bounds) { setPolygons({polygon}, bounds); }

void TexturedPolygon3dLayerObject::setPolygons(const std::vector<::PolygonCoord> &polygons, const ::RectCoord &bounds) {
    std::vector<uint16_t> indices;
    std::vector<float> vertices;
    int32_t indexOffset = 0;

    if (polygons.size() == 0 || polygons[0].positions.size() == 0) {
        return;
    }
    double minX = bounds.topLeft.x;
    double minY = bounds.topLeft.y;
    double maxX = bounds.bottomRight.x;
    double maxY = bounds.bottomRight.y;
    if (minX > maxX) {
        minX = bounds.bottomRight.x;
        maxX = bounds.topLeft.x;
    }
    if (minY > maxY) {
        minY = bounds.bottomRight.y;
        maxY = bounds.topLeft.y;
    }

    for (auto const &polygon : polygons) {
        std::vector<std::vector<Vec2D>> renderCoords;
        std::vector<std::vector<Vec2D>> renderUV;
        std::vector<Vec2D> polygonCoords;
        std::vector<Vec2D> polygonUV;
        for (const Coord &mapCoord : polygon.positions) {
            Coord renderCoord = conversionHelper->convert(CoordinateSystemIdentifiers::EPSG4326(), mapCoord);
            polygonCoords.push_back(Vec2D(renderCoord.x, renderCoord.y));
            polygonUV.push_back(Vec2D(mapCoord.x, mapCoord.y));
        }
        renderCoords.push_back(polygonCoords);
        renderUV.push_back(polygonUV);

        for (const auto &hole : polygon.holes) {
            std::vector<::Vec2D> holeCoords;
            std::vector<Vec2D> holeUV;
            for (const Coord &coord : hole) {
                Coord renderCoord = conversionHelper->convert(CoordinateSystemIdentifiers::EPSG4326(), coord);
                holeCoords.push_back(Vec2D(renderCoord.x, renderCoord.y));
                holeUV.push_back(Vec2D(coord.x, coord.y));
            }
            renderCoords.push_back(holeCoords);
            renderUV.push_back(holeUV);
        }
        std::vector<int32_t> curIndices = mapbox::earcut<int32_t>(renderUV);

        for (auto const &index : curIndices) {
            indices.push_back(indexOffset + index);
        }

        int renderCoordsIndex = 0;
        for (auto const &list : renderCoords) {
            indexOffset += list.size();

            int listIndex = 0;
            for(auto& i : list) {
                vertices.push_back(i.x);
                vertices.push_back(i.y);
                // fill for android z
//                vertices.push_back(0.0);
                // are needed to fill metal vertex property (position, uv, normal)
                auto const &uv = renderUV[renderCoordsIndex][listIndex];
                double u = (uv.x-minX)/(maxX-minX);
                double v = (uv.y-minY)/(maxY-minY);
                vertices.push_back(u);
                vertices.push_back(1.0 - v);

                // normal
                vertices.push_back(0.0);
                vertices.push_back(0.0);


                listIndex++;
            }
            renderCoordsIndex++;
        }
    }

    auto attr = SharedBytes((int64_t)vertices.data(), (int32_t)vertices.size(), (int32_t)sizeof(float));
    auto ind = SharedBytes((int64_t)indices.data(), (int32_t)indices.size(), (int32_t)sizeof(uint16_t));

    polygon->setVertices(attr, ind);
}

void TexturedPolygon3dLayerObject::update() {
    if (animation) {
        animation->update();
    }
}

std::vector<std::shared_ptr<RenderConfigInterface>> TexturedPolygon3dLayerObject::getRenderConfig() { return {renderConfig}; }

void TexturedPolygon3dLayerObject::setAlpha(float alpha) {
    if (shader) {
        shader->updateAlpha(alpha);
    }
    mapInterface->invalidate();
}

void TexturedPolygon3dLayerObject::setTileInfo(const int32_t x, const int32_t y, const int32_t z, const int32_t offset) {
    polygon->setTileInfo(x, y, z, offset);
}

std::shared_ptr<Polygon3dInterface> TexturedPolygon3dLayerObject::getPolygonObject() { return polygon; }

std::shared_ptr<GraphicsObjectInterface> TexturedPolygon3dLayerObject::getGraphicsObject() { return graphicsObject; }

std::shared_ptr<RenderObjectInterface> TexturedPolygon3dLayerObject::getRenderObject() {
    return renderObject;
}

void TexturedPolygon3dLayerObject::beginAlphaAnimation(double startAlpha, double targetAlpha, long long duration) {
    animation = std::make_shared<DoubleAnimation>(
                                                  duration, startAlpha, targetAlpha, InterpolatorFunction::EaseIn, [=](double alpha) { this->setAlpha(alpha); },
                                                  [=] {
                                                      this->setAlpha(targetAlpha);
                                                      this->animation = nullptr;
                                                  });
    animation->start();
    mapInterface->invalidate();
}
