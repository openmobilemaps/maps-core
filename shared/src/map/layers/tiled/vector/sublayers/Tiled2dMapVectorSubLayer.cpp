/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include <logger/Logger.h>
#include <map>
#include "MapInterface.h"
#include "Tiled2dMapVectorSubLayer.h"
#include "VectorTilePolygonHandler.h"
#include "RenderObject.h"
#include "RenderPass.h"
#include "LambdaTask.h"

Tiled2dMapVectorSubLayer::Tiled2dMapVectorSubLayer(const Color &fillColor) : fillColor(fillColor) {}

void Tiled2dMapVectorSubLayer::update() {

}

std::vector<std::shared_ptr<::RenderPassInterface>> Tiled2dMapVectorSubLayer::buildRenderPasses() {
    return renderPasses;
}

void Tiled2dMapVectorSubLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface) {
    this->mapInterface = mapInterface;
}

void Tiled2dMapVectorSubLayer::onRemoved() {
    this->mapInterface = nullptr;
}

void Tiled2dMapVectorSubLayer::pause() {
    std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
    for (const auto &tileGroup : tilePolygonMap) {
        for (const auto &polygon : tileGroup.second) {
            polygon->getPolygonObject()->clear();
        }
    }
    for (const auto &tileGroup : tileMaskMap) {
        if (tileGroup.second) tileGroup.second->getQuadObject()->asGraphicsObject()->clear();
    }
}

void Tiled2dMapVectorSubLayer::resume() {
    std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
    const auto &context = mapInterface->getRenderingContext();
    for (const auto &tileGroup : tilePolygonMap) {
        for (const auto &polygon : tileGroup.second) {
            polygon->getPolygonObject()->setup(context);
        }
    }
    for (const auto &tileGroup : tileMaskMap) {
        if (tileGroup.second) tileGroup.second->getQuadObject()->asGraphicsObject()->setup(context);
    }
}

void Tiled2dMapVectorSubLayer::hide() {
    // TODO
}

void Tiled2dMapVectorSubLayer::show() {
    // TODO
}

void
Tiled2dMapVectorSubLayer::updateTileData(const Tiled2dMapVectorTileInfo &tileInfo, vtzero::layer &data) {
    if (!mapInterface) return;

    std::string layerName = std::string(data.name());
    uint32_t extent = data.extent();
    LogDebug <<= "    layer: " + layerName + " - v" + std::to_string(data.version()) + " - extent: " +
                 std::to_string(extent);
    std::string defIdPrefix =
            std::to_string(tileInfo.tileInfo.x) + "/" + std::to_string(tileInfo.tileInfo.y) + "_" + layerName + "_";
    if (!data.empty()) {
        LogDebug <<= "      with " + std::to_string(data.num_features()) + " features";
        int featureNum = 0;
        std::vector<const PolygonInfo> newPolygons;
        while (const auto &feature = data.next_feature()) {
            if (feature.geometry_type() == vtzero::GeomType::POLYGON) {
                auto polygonHandler = VectorTilePolygonHandler(tileInfo.tileInfo.bounds, extent);
                try {
                    vtzero::decode_polygon_geometry(feature.geometry(), polygonHandler);
                } catch (vtzero::geometry_exception &geometryException) {
                    continue;
                }
                std::string id = feature.has_id() ? std::to_string(feature.id()) : (defIdPrefix + std::to_string(featureNum));
                auto polygonCoordinates = polygonHandler.getCoordinates();
                auto polygonHoles = polygonHandler.getHoles();
                for (int i = 0; i < polygonCoordinates.size(); i++) {
                    const auto polygonInfo = PolygonInfo(id, polygonCoordinates[i], polygonHoles[i], false, fillColor, fillColor);
                    newPolygons.push_back(polygonInfo);
                    featureNum++;
                }
            }
        }
        addPolygons(tileInfo, newPolygons);
    }
}

void Tiled2dMapVectorSubLayer::clearTileData(const Tiled2dMapVectorTileInfo &tileInfo) {
    {
        std::lock_guard<std::recursive_mutex> lock(updateMutex);
        const auto &polygons = tilePolygonMap[tileInfo];
        for (const auto &polygonObject : polygons) {
            polygonObject->getPolygonObject()->clear();
        }
        tilePolygonMap.erase(tileInfo);
        const auto &maskObject = tileMaskMap[tileInfo];
        if (maskObject)
            tileMaskMap.erase(tileInfo);
    }
}

void
Tiled2dMapVectorSubLayer::addPolygons(const Tiled2dMapVectorTileInfo &tileInfo, const std::vector<const PolygonInfo> &polygons) {
    if (polygons.empty()) return;

    const auto &objectFactory = mapInterface->getGraphicsObjectFactory();
    const auto &shaderFactory = mapInterface->getShaderFactory();

    std::vector<std::shared_ptr<GraphicsObjectInterface>> newGraphicObjects;

    {
        std::lock_guard<std::recursive_mutex> lock(updateMutex);
        for (const auto &polygon : polygons) {
            auto shader = shaderFactory->createColorShader();
            auto polygonGraphicsObject = objectFactory->createPolygon(shader->asShaderProgramInterface());

            auto polygonObject =
                    std::make_shared<Polygon2dLayerObject>(mapInterface->getCoordinateConverterHelper(), polygonGraphicsObject,
                                                           shader);

            polygonObject->setPositions(polygon.coordinates, polygon.holes, polygon.isConvex);
            polygonObject->setColor(polygon.color);

            newGraphicObjects.push_back(polygonGraphicsObject->asGraphicsObject());
            tilePolygonMap[tileInfo].push_back(polygonObject);
        }
        const auto &graphicsObjectFactory = mapInterface->getGraphicsObjectFactory();
        const auto &conversionHelper = mapInterface->getCoordinateConverterHelper();
        auto maskingObject = std::make_shared<QuadMaskObject>(graphicsObjectFactory, conversionHelper, tileInfo.tileInfo.bounds);

        /* Polygon-Mask Test
        auto maskingObject = std::make_shared<PolygonMaskObject>(graphicsObjectFactory, conversionHelper);
        const auto &bounds = tileInfo.tileInfo.bounds;
        double width = (bounds.bottomRight.x - bounds.topLeft.x);
        double height = (bounds.bottomRight.y - bounds.topLeft.y);
        std::vector<::Coord> positions = {bounds.topLeft,
                                          Coord(bounds.topLeft.systemIdentifier, bounds.topLeft.x + width, bounds.topLeft.y,
                                                bounds.topLeft.z),
                                          bounds.bottomRight,
                                          Coord(bounds.topLeft.systemIdentifier, bounds.topLeft.x, bounds.topLeft.y + height,
                                                bounds.topLeft.z), bounds.topLeft};
        std::vector<std::vector<::Coord>> holes = {{
                                                           Coord(bounds.topLeft.systemIdentifier, bounds.topLeft.x + 0.05 * width,
                                                                 bounds.topLeft.y, bounds.topLeft.z),
                                                           Coord(bounds.topLeft.systemIdentifier, bounds.bottomRight.x,
                                                                 bounds.topLeft.y + 0.95 * height, bounds.topLeft.z),
                                                           Coord(bounds.topLeft.systemIdentifier, bounds.topLeft.x + 0.95 * width,
                                                                 bounds.bottomRight.y, bounds.topLeft.z),
                                                           Coord(bounds.topLeft.systemIdentifier, bounds.topLeft.x,
                                                                 bounds.topLeft.y + 0.05 * height, bounds.topLeft.z),
                                                           Coord(bounds.topLeft.systemIdentifier, bounds.topLeft.x + 0.05 * width,
                                                                 bounds.topLeft.y, bounds.topLeft.z)
                                                   },
                                                   {
                                                           Coord(bounds.topLeft.systemIdentifier, bounds.topLeft.x + 0.95 * width,
                                                                 bounds.topLeft.y, bounds.topLeft.z),
                                                           Coord(bounds.topLeft.systemIdentifier, bounds.bottomRight.x,
                                                                 bounds.topLeft.y + 0.05 * height, bounds.topLeft.z),
                                                           Coord(bounds.topLeft.systemIdentifier, bounds.topLeft.x + 0.05 * width,
                                                                 bounds.bottomRight.y, bounds.topLeft.z),
                                                           Coord(bounds.topLeft.systemIdentifier, bounds.topLeft.x,
                                                                 bounds.topLeft.y + 0.95 * height, bounds.topLeft.z),
                                                           Coord(bounds.topLeft.systemIdentifier, bounds.topLeft.x + 0.95 * width,
                                                                 bounds.topLeft.y, bounds.topLeft.z)
                                                   }};
        maskingObject->setPositions(positions, holes, true); */

        newGraphicObjects.push_back(maskingObject->getQuadObject()->asGraphicsObject());
        tileMaskMap[tileInfo] = maskingObject;
    }
    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
            TaskConfig("PolygonLayer_setup_" + polygons[0].identifier, 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
            [=] {
                for (const auto &graphicsObject : newGraphicObjects) {
                    graphicsObject->setup(mapInterface->getRenderingContext());
                }
            }));
}

void Tiled2dMapVectorSubLayer::preGenerateRenderPasses() {
    std::lock_guard<std::recursive_mutex> lock(updateMutex);
    std::vector<std::shared_ptr<RenderPassInterface>> newRenderPasses;
    for (auto const &tilePolygonTuple : tilePolygonMap) {
        std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> renderPassObjectMap;
        for (auto const &polygonObject : tilePolygonTuple.second) {
            for (auto config : polygonObject->getRenderConfig()) {
                renderPassObjectMap[config->getRenderIndex()].push_back(
                        std::make_shared<RenderObject>(config->getGraphicsObject()));
            }
        }
        const auto &tileMask = tileMaskMap[tilePolygonTuple.first];
        for (const auto &passEntry : renderPassObjectMap) {
            std::shared_ptr<RenderPass> renderPass = std::make_shared<RenderPass>(RenderPassConfig(passEntry.first),
                                                                                  passEntry.second,
                                                                                  (tileMask
                                                                                   ? tileMask->getQuadObject()->asMaskingObject()
                                                                                   : nullptr));
            newRenderPasses.push_back(renderPass);
        }
    }
    renderPasses = newRenderPasses;
}
