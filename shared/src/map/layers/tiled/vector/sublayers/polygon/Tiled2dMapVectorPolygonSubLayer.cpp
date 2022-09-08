/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorPolygonSubLayer.h"
#include <map>
#include "MapInterface.h"
#include "RenderObject.h"
#include "RenderPass.h"
#include "LambdaTask.h"
#include "VectorTileGeometryHandler.h"
#include "LineGroupShaderInterface.h"
#include "earcut.hpp"
#include <algorithm>
#include "MapCamera2dInterface.h"
#include "Tiled2dMapVectorRasterSubLayerConfig.h"

namespace mapbox {
namespace util {

template <>
struct nth<0, ::Coord> {
    inline static auto get(const ::Coord &t) {
        return t.x;
    };
};
template <>
struct nth<1, ::Coord> {
    inline static auto get(const ::Coord &t) {
        return t.y;
    };
};

} // namespace util
} // namespace mapbox


Tiled2dMapVectorPolygonSubLayer::Tiled2dMapVectorPolygonSubLayer(const std::shared_ptr<PolygonVectorLayerDescription> &description)
        : description(description), usedKeys(description->getUsedKeys()) {}

void Tiled2dMapVectorPolygonSubLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface) {
    Tiled2dMapVectorSubLayer::onAdded(mapInterface);
    shader = mapInterface->getShaderFactory()->createPolygonGroupShader();
}

void Tiled2dMapVectorPolygonSubLayer::onRemoved() {
    Tiled2dMapVectorSubLayer::onRemoved();
}

void Tiled2dMapVectorPolygonSubLayer::pause() {
    Tiled2dMapVectorSubLayer::pause();
    std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(tilesInSetupMutex, polygonMutex);
    for (const auto &tileGroup : tilePolygonMap) {
        tilesInSetup.insert(tileGroup.first);
        for (auto const &polygon: tileGroup.second) {
            if (polygon->getPolygonObject()->isReady()) polygon->getPolygonObject()->clear();
        }
    }
}

void Tiled2dMapVectorPolygonSubLayer::resume() {
    Tiled2dMapVectorSubLayer::resume();
    std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(tilesInSetupMutex, polygonMutex);
    const auto &context = mapInterface->getRenderingContext();
    for (const auto &tileGroup : tilePolygonMap) {
        for (auto const &polygon: tileGroup.second) {
            if (!polygon->getPolygonObject()->isReady()) polygon->getPolygonObject()->setup(context);
        }
        tilesInSetup.erase(tileGroup.first);
        if (auto delegate = readyDelegate.lock()) {
            delegate->tileIsReady(tileGroup.first);
        }
    }
}

void Tiled2dMapVectorPolygonSubLayer::hide() {
    Tiled2dMapVectorSubLayer::hide();
}

void Tiled2dMapVectorPolygonSubLayer::show() {
    Tiled2dMapVectorSubLayer::show();
}

void
Tiled2dMapVectorPolygonSubLayer::updateTileData(const Tiled2dMapTileInfo &tileInfo, const std::shared_ptr<MaskingObjectInterface> &tileMask,
                                                const std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>> &layerFeatures) {
    Tiled2dMapVectorSubLayer::updateTileData(tileInfo, tileMask, layerFeatures);
    if (!mapInterface) {
        return;
    }

    std::string layerName = description->sourceId;

    std::string defIdPrefix =
            std::to_string(tileInfo.x) + "/" + std::to_string(tileInfo.y) + "_" + layerName + "_";
    if (!layerFeatures.empty() &&
        description->minZoom <= tileInfo.zoomIdentifier &&
        description->maxZoom >= tileInfo.zoomIdentifier) {

        std::vector<std::tuple<std::vector<std::tuple<std::vector<Coord>, int>>, std::vector<int32_t>>> objectDescriptions;
        objectDescriptions.push_back({{},{}});

        std::vector<int32_t> indices;
        std::int32_t indices_offset = 0;
        for (const auto &feature : layerFeatures) {
            const FeatureContext &featureContext = std::get<0>(feature);

            if (description->filter == nullptr || description->filter->evaluateOr(EvaluationContext(-1, featureContext), true)) {
                const auto &geometryHandler = std::get<1>(feature);
                const auto &polygonCoordinates = geometryHandler.getPolygonCoordinates();
                const auto &polygonHoles = geometryHandler.getHoleCoordinates();

                std::vector<Coord> positions;

                for (int i = 0; i < polygonCoordinates.size(); i++) {
                    std::vector<std::vector<::Coord>> pol = {polygonCoordinates[i]};
                    for (auto const &hole: polygonHoles[i]) {
                        pol.push_back(hole);
                    }
                    std::vector<uint16_t> new_indices = mapbox::earcut<uint16_t>(pol);


                    std::size_t posAdded = 0;
                    for (auto const &coords: pol) {
                        positions.insert(positions.end(), coords.begin(), coords.end());
                        posAdded += coords.size();
                    }

                    // check overflow
                    size_t new_size = indices_offset + posAdded;
                    if (new_size >= std::numeric_limits<uint16_t>::max()) {
                        objectDescriptions.push_back({{},
                                                      {}});
                        indices_offset = 0;
                    }

                    for (auto const &index: new_indices) {
                        std::get<1>(objectDescriptions.back()).push_back(indices_offset + index);
                    }

                    indices_offset += posAdded;
                }

                int styleIndex = -1;
                {
                    auto const hash = featureContext.getStyleHash(usedKeys);
                    std::lock_guard<std::recursive_mutex> lock(featureGroupsMutex);

                    for (size_t i = 0; i != featureGroups.size(); i++) {
                        auto const &[groupHash, group] = featureGroups.at(i);
                        if (hash == groupHash) {
                            styleIndex = (int) i;
                            break;
                        }
                    }

                    if (styleIndex == -1) {
                        featureGroups.push_back({ hash, featureContext });
                        styleIndex = (int) featureGroups.size() - 1;
                    }
                }

                std::get<0>(objectDescriptions.back()).push_back({positions, styleIndex});
            }
        }

        addPolygons(tileInfo, objectDescriptions);

        preGenerateRenderPasses();
    } else {
        if (auto delegate = readyDelegate.lock()) {
            delegate->tileIsReady(tileInfo);
        }
    }
}

void Tiled2dMapVectorPolygonSubLayer::addPolygons(const Tiled2dMapTileInfo &tileInfo, const std::vector<std::tuple<std::vector<std::tuple<std::vector<Coord>, int>>, std::vector<int32_t>>> &polygons){

    if (polygons.empty()) {
        if (auto delegate = readyDelegate.lock()) {
            delegate->tileIsReady(tileInfo);
        }
        return;
    }

    auto mapInterface = this->mapInterface;
    auto objectFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
    auto scheduler = mapInterface ? mapInterface->getScheduler() : nullptr;
    auto converter = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;

    if (!mapInterface || !objectFactory || !scheduler || !converter) {
        return;
    }

    std::vector<std::shared_ptr<PolygonGroup2dLayerObject>> polygonObjects;
    std::vector<std::shared_ptr<GraphicsObjectInterface>> newGraphicObjects;

    for (auto const& tuple: polygons) {
        const auto polygonObject = objectFactory->createPolygonGroup(shader->asShaderProgramInterface());

        auto layerObject = std::make_shared<PolygonGroup2dLayerObject>(converter,
                                                                       polygonObject, shader);


        layerObject->setVertices(std::get<0>(tuple), std::get<1>(tuple));

        polygonObjects.push_back(layerObject);
        newGraphicObjects.push_back(polygonObject->asGraphicsObject());
    }

    {
        std::lock_guard<std::recursive_mutex> lock(polygonMutex);
        tilePolygonMap[tileInfo] = polygonObjects;
    }

    {
        std::lock_guard<std::recursive_mutex> lock(tilesInSetupMutex);
        tilesInSetup.insert(tileInfo);
    }

    std::weak_ptr<Tiled2dMapVectorPolygonSubLayer> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapVectorPolygonSubLayer>(
            shared_from_this());
    scheduler->addTask(std::make_shared<LambdaTask>(
            TaskConfig("Tiled2dMapVectorPolygonSubLayer_setup", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
            [weakSelfPtr, tileInfo, newGraphicObjects] {
                auto selfPtr = weakSelfPtr.lock();
                if (selfPtr) {
                    selfPtr->setupPolygons(tileInfo, newGraphicObjects);
                }
            }));
}

void Tiled2dMapVectorPolygonSubLayer::setupPolygons(const Tiled2dMapTileInfo &tileInfo, const std::vector<std::shared_ptr<GraphicsObjectInterface>> &newPolygonObjects) {
    auto mapInterface = this->mapInterface;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) {
        return;
    }

    {
        std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(tilesInSetupMutex, polygonMutex);
        if (tilePolygonMap.count(tileInfo) == 0)
        {
            if (auto delegate = readyDelegate.lock()) {
                delegate->tileIsReady(tileInfo);
            }
            return;
        }

        for (auto const &polygon: newPolygonObjects) {
            if (!polygon->isReady()) polygon->setup(renderingContext);
        }
        tilesInSetup.erase(tileInfo);
    }
    if (auto delegate = readyDelegate.lock()) {
        delegate->tileIsReady(tileInfo);
    }
}

void Tiled2dMapVectorPolygonSubLayer::update() {

    std::lock_guard<std::recursive_mutex> lock(featureGroupsMutex);
    double zoomIdentifier = Tiled2dMapVectorRasterSubLayerConfig::getZoomIdentifier(mapInterface->getCamera()->getZoom());

    std::vector<float> shaderStyles;

    for (auto const &[hash, feature]: featureGroups) {
        const auto& ec = EvaluationContext(zoomIdentifier, feature);
        const auto& color = description->style.getFillColor(ec);
        const auto& opacity = description->style.getFillOpacity(ec);

        shaderStyles.push_back(color.r);
        shaderStyles.push_back(color.g);
        shaderStyles.push_back(color.b);
        shaderStyles.push_back(color.a);
        shaderStyles.push_back(opacity);
    }

    auto s = SharedBytes((int64_t)shaderStyles.data(), (int32_t)featureGroups.size(), 5 * (int32_t)sizeof(float));
    shader->setStyles(s);
}

void Tiled2dMapVectorPolygonSubLayer::clearTileData(const Tiled2dMapTileInfo &tileInfo) {
    auto mapInterface = this->mapInterface;
    if (!mapInterface) { return; }

    std::vector<std::shared_ptr<GraphicsObjectInterface>> objectsToClear;
    Tiled2dMapVectorSubLayer::clearTileData(tileInfo);

    {
        std::lock_guard<std::recursive_mutex> lock(polygonMutex);
        if (tilePolygonMap.count(tileInfo) != 0) {
            for (auto const &polygon: tilePolygonMap[tileInfo]) {
                objectsToClear.push_back(polygon->getPolygonObject());
            }
            tilePolygonMap.erase(tileInfo);
        }
    }
    if (objectsToClear.empty()) return;

    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
            TaskConfig("LineGroupTile_clear_" + std::to_string(tileInfo.zoomIdentifier) + "/" + std::to_string(tileInfo.x) + "/" +
                       std::to_string(tileInfo.y), 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
            [objectsToClear] {
                for (const auto &lineObject : objectsToClear) {
                    if (lineObject->isReady()) lineObject->clear();
                }
            }));
}


void Tiled2dMapVectorPolygonSubLayer::preGenerateRenderPasses() {
    std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(maskMutex, polygonMutex);
    std::unordered_map<Tiled2dMapTileInfo, std::vector<std::shared_ptr<RenderPassInterface>>> newRenderPasses;
    for (auto const &tileLineTuple : tilePolygonMap) {
        std::vector<std::shared_ptr<RenderPassInterface>> newTileRenderPasses;
        std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> renderPassObjectMap;
        for (auto const &object : tileLineTuple.second) {
            for (const auto &config : object->getRenderConfig()) {
                renderPassObjectMap[config->getRenderIndex()].push_back(
                        std::make_shared<RenderObject>(config->getGraphicsObject()));
            }
        }
        const auto &tileMask = tileMaskMap[tileLineTuple.first];
        for (const auto &passEntry : renderPassObjectMap) {
            std::shared_ptr<RenderPass> renderPass = std::make_shared<RenderPass>(RenderPassConfig(passEntry.first),
                                                                                  passEntry.second,
                                                                                  (tileMask
                                                                                   ? tileMask
                                                                                   : nullptr));
            newTileRenderPasses.push_back(renderPass);
        }
        newRenderPasses.insert({tileLineTuple.first, newTileRenderPasses});
    }
    renderPasses = newRenderPasses;
}

void Tiled2dMapVectorPolygonSubLayer::updateTileMask(const Tiled2dMapTileInfo &tileInfo,
                                                     const std::shared_ptr<MaskingObjectInterface> &tileMask) {
    Tiled2dMapVectorSubLayer::updateTileMask(tileInfo, tileMask);
    preGenerateRenderPasses();
}
