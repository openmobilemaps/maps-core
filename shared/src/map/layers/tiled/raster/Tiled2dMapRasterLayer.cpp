//
// Created by Christoph Maurhofer on 09.02.2021.
//

#include "Tiled2dMapRasterLayer.h"
#include "MapConfig.h"
#include "LambdaTask.h"
#include "RenderPass.h"

Tiled2dMapRasterLayer::Tiled2dMapRasterLayer(const std::shared_ptr<::MapInterface> &mapInterface,
                                             const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                             const std::shared_ptr<::TextureLoaderInterface> &textureLoader)
        : Tiled2dMapLayer(mapInterface, layerConfig), textureLoader(textureLoader) {
}

void Tiled2dMapRasterLayer::onAdded() {
    Tiled2dMapLayer::onAdded();
    colorShader = mapInterface->getShaderFactory()->createColorShader();
    colorShader->setColor(0, 1, 1, 0.5);

    rasterSource = std::make_shared<Tiled2dMapRasterSource>(mapInterface->getMapConfig(),
                                                            layerConfig,
                                                            mapInterface->getCoordinateConverterHelper(),
                                                            mapInterface->getScheduler(),
                                                            textureLoader,
                                                            shared_from_this());
    setSourceInterface(rasterSource);
}

void Tiled2dMapRasterLayer::onRemoved() {
    Tiled2dMapLayer::onRemoved();
    pause();
}

std::shared_ptr<::LayerInterface> Tiled2dMapRasterLayer::asLayerInterface() {
    return shared_from_this();
}

std::vector<std::shared_ptr<::RenderPassInterface>> Tiled2dMapRasterLayer::buildRenderPasses() {
    return renderPasses;
}

std::string Tiled2dMapRasterLayer::getIdentifier() {
    return "RasterTileLayer"; // TODO: Fix identifier (e.g. include layerConfig-Info)
}

void Tiled2dMapRasterLayer::pause() {
    rasterSource->pause();
    for (const auto &tileObject : tileObjectMap) {
        tileObject.second->getPolygonObject()->clear();
    }
}

void Tiled2dMapRasterLayer::resume() {
    rasterSource->resume();
    auto renderingContext = mapInterface->getRenderingContext();
    for (const auto &tileObject : tileObjectMap) {
        tileObject.second->getPolygonObject()->setup(renderingContext);
    }
}

void Tiled2dMapRasterLayer::onTilesUpdated() {
    // TODO: Check for current tiles in source and create corresponding layer objects (resp. remove old ones)
    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
            TaskConfig("Tiled2dMapSource_Update",
                       0,
                       TaskPriority::NORMAL,
                       ExecutionEnvironment::GRAPHICS),
            [=] {
                // TODO: Check if this task is still relevant/the most current one
                std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);

                auto currentTileInfos = rasterSource->getCurrentTiles();

                std::unordered_set<Tiled2dMapRasterTileInfo> tilesToAdd;
                for (const auto &rasterTileInfo: currentTileInfos) {
                    if (!tileObjectMap[rasterTileInfo]) tilesToAdd.insert(rasterTileInfo);
                }

                std::unordered_set<Tiled2dMapRasterTileInfo> tilesToRemove;
                for (const auto &tileEntry: tileObjectMap) {
                    if (currentTileInfos.count(tileEntry.first) == 0) tilesToRemove.insert(tileEntry.first);
                }

                auto renderingContext = mapInterface->getRenderingContext();
                auto graphicsFactory = mapInterface->getGraphicsObjectFactory();

                for (const auto &tile : tilesToAdd) {
                    auto tileObject = std::make_shared<Polygon2dLayerObject>(mapInterface->getCoordinateConverterHelper(),
                                                                             graphicsFactory->createPolygon(
                                                                                     colorShader->asShaderProgramInterface()),
                                                                             colorShader);
                    auto topLeft = tile.tileInfo.bounds.topLeft;
                    auto bottomRight = tile.tileInfo.bounds.bottomRight;
                    auto borderWidth = 0.1 * (bottomRight.x - topLeft.x);
                    tileObject->setPositions({Coord(topLeft.systemIdentifier, topLeft.x + borderWidth, topLeft.y + borderWidth, 0),
                                              Coord(topLeft.systemIdentifier, bottomRight.x - borderWidth, topLeft.y + borderWidth,
                                                    0),
                                              Coord(topLeft.systemIdentifier, bottomRight.x - borderWidth,bottomRight.y - borderWidth,
                                                    0),
                                              Coord(topLeft.systemIdentifier, topLeft.x + borderWidth, bottomRight.y - borderWidth,
                                                    0)});
                    tileObject->getPolygonObject()->setup(renderingContext);
                    tileObjectMap[tile] = tileObject;
                }

                for (const auto &tile : tilesToRemove) {
                    auto tileObject = tileObjectMap[tile];
                    tileObject->getPolygonObject()->clear();
                    tileObjectMap.erase(tile);
                }

                std::vector<std::shared_ptr<GraphicsObjectInterface>> renderObjects;
                for (const auto &objectEntry : tileObjectMap) {
                    // TODO: Build correct number of render passes by respecting configs received from each LayerObject
                    renderObjects.push_back(objectEntry.second->getRenderConfig()[0]->getGraphicsObject());
                }
                std::shared_ptr<RenderPass> renderPass = std::make_shared<RenderPass>(RenderPassConfig(0), renderObjects);
                renderPasses = {renderPass};
            }));
}

