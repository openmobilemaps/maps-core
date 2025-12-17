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
#include "BoundingBox.h"
#include "CoordinateSystemIdentifiers.h"
#include "CoordinatesUtil.h"
#include "TrigonometryLUT.h"
#include <cmath>
#include "Tiled2dMapVectorLayerConstants.h"

Polygon2dLayerObject::Polygon2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                           const std::shared_ptr<Polygon2dInterface> &polygon,
                                           const std::shared_ptr<ColorShaderInterface> &shader,
                                           bool is3D)
        : conversionHelper(conversionHelper),
          shader(shader),
          polygon(polygon),
          graphicsObject(polygon->asGraphicsObject()),
          renderConfig(std::make_shared<RenderConfig>(graphicsObject, 0)),
          is3D(is3D) {}

std::vector<std::shared_ptr<RenderConfigInterface>> Polygon2dLayerObject::getRenderConfig() { return {renderConfig}; }

void Polygon2dLayerObject::setPositions(const std::vector<Coord> &positions, const std::vector<std::vector<Coord>> &holes) {
    setPolygon({positions, holes});
}

void Polygon2dLayerObject::setPolygon(const PolygonCoord &polygon) { setPolygons({polygon}); }

void Polygon2dLayerObject::setPolygons(const std::vector<PolygonCoord> &polygons) {
    std::vector<uint16_t> indices;
    std::vector<float> vertices;
    int32_t indexOffset = 0;
    double avgX = 0.0f, avgY = 0.0f;
    size_t totalPoints = 0;

    std::vector<Vec2D> vecVertices;
    mapbox::detail::Earcut<int32_t> earcutter;

    BoundingBox bbox = BoundingBox(CoordinateSystemIdentifiers::RENDERSYSTEM());
    for (auto const &polygon : polygons) {
        std::vector<std::vector<Vec2D>> renderCoords;
        std::vector<Vec2D> polygonCoords;
        for (const Coord &mapCoord : polygon.positions) {
            Coord renderCoord = conversionHelper->convertToRenderSystem(mapCoord);
            polygonCoords.push_back(Vec2D(renderCoord.x, renderCoord.y));
            bbox.addPoint(renderCoord);
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
                // Accumulate for averaging
                avgX += i.x;
                avgY += i.y;
                totalPoints += 1;

                vecVertices.push_back(Vec2D(i.x, i.y));
            }
        }
    }

    // Calculate the average (origin)
    if (totalPoints > 0) {
        avgX /= totalPoints;
        avgY /= totalPoints;
    }

    double rx = is3D ? 1.0 * sin(avgY) * cos(avgX) : avgX;
    double ry = is3D ? 1.0 * cos(avgY) : avgY;
    double rz = is3D ? -1.0 * sin(avgY) * sin(avgX) : 0.0;

#ifndef TESSELLATION_ACTIVATED
    if (is3D) {
        auto bboxSize = bbox.getMax() - bbox.getMin();
        double threshold = std::max(std::max(bboxSize.x, bboxSize.y), bboxSize.z) / std::pow(2, SUBDIVISION_FACTOR_3D_DEFAULT);
        PolygonHelper::subdivision(vecVertices, indices, threshold);
    }
#endif
    
    for (const auto& v : vecVertices) {
        
        // Position
        if(is3D) {
            double sinX, sinY, cosX, cosY;
            lut::sincos(v.x, sinX, cosX);
            lut::sincos(v.y, sinY, cosY);

            vertices.push_back(1.0 * sinY * cosX - rx);
            vertices.push_back(1.0 * cosY - ry);
            vertices.push_back(-1.0 * sinY * sinX - rz);
        } else {
            vertices.push_back(v.x - rx);
            vertices.push_back(v.y - ry);
            vertices.push_back(0.0);
        }
    #ifdef __APPLE__
        vertices.push_back(0.0f);
    #endif
    
    #ifdef TESSELLATION_ACTIVATED
        // Frame Coord
        vertices.push_back(v.x);
        vertices.push_back(v.y);
    #endif
    }

    auto attr = SharedBytes((int64_t)vertices.data(), (int32_t)vertices.size(), (int32_t)sizeof(float));
    auto ind = SharedBytes((int64_t)indices.data(), (int32_t)indices.size(), (int32_t)sizeof(uint16_t));
    polygon->setVertices(attr, ind, Vec3D(rx, ry, rz), is3D);
    
#ifdef TESSELLATION_ACTIVATED
    int32_t subdivisionFactor = is3D ? std::pow(2, SUBDIVISION_FACTOR_3D_DEFAULT) : 0;
    polygon->setSubdivisionFactor(subdivisionFactor);
#endif
}

void Polygon2dLayerObject::setColor(const Color &color) {
    shader->setColor(color.r, color.g, color.b, color.a);
}

std::shared_ptr<GraphicsObjectInterface> Polygon2dLayerObject::getPolygonObject() { return graphicsObject; }

std::shared_ptr<ShaderProgramInterface> Polygon2dLayerObject::getShaderProgram() { return shader->asShaderProgramInterface(); }
