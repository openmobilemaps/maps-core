/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "TiledDisplacedRasterLayer.h"
#include "MapCameraInterface.h"
#include "RectCoord.h"
#include "Tiled2dMapVectorSettings.h"
#include "Tiled2dMapZoomInfo.h"
#include "Tiled2dMapZoomLevelInfo.h"
#include <algorithm>
#include <optional>
#include <unordered_set>

namespace {
class ZoomLevelLimitedLayerConfig final : public Tiled2dMapLayerConfig {
public:
    ZoomLevelLimitedLayerConfig(std::shared_ptr<Tiled2dMapLayerConfig> baseConfig, std::optional<int32_t> maxZoomLevelIdentifier)
        : baseConfig(std::move(baseConfig))
        , maxZoomLevelIdentifier(maxZoomLevelIdentifier) {}

    int32_t getCoordinateSystemIdentifier() override {
        return baseConfig->getCoordinateSystemIdentifier();
    }

    std::string getTileUrl(int32_t x, int32_t y, int32_t t, int32_t zoom) override {
        return baseConfig->getTileUrl(x, y, t, zoom);
    }

    std::vector<Tiled2dMapZoomLevelInfo> getZoomLevelInfos() override {
        return limit(baseConfig->getZoomLevelInfos());
    }

    std::vector<Tiled2dMapZoomLevelInfo> getVirtualZoomLevelInfos() override {
        return limit(baseConfig->getVirtualZoomLevelInfos());
    }

    Tiled2dMapZoomInfo getZoomInfo() override {
        return baseConfig->getZoomInfo();
    }

    std::string getLayerName() override {
        return baseConfig->getLayerName();
    }

    std::optional<Tiled2dMapVectorSettings> getVectorSettings() override {
        return baseConfig->getVectorSettings();
    }

    std::optional<RectCoord> getBounds() override {
        return baseConfig->getBounds();
    }

private:
    std::vector<Tiled2dMapZoomLevelInfo> limit(const std::vector<Tiled2dMapZoomLevelInfo> &zoomLevelInfos) const {
        if (!maxZoomLevelIdentifier.has_value()) {
            return {};
        }

        std::vector<Tiled2dMapZoomLevelInfo> limitedInfos;
        limitedInfos.reserve(zoomLevelInfos.size());
        std::copy_if(zoomLevelInfos.begin(), zoomLevelInfos.end(), std::back_inserter(limitedInfos),
                     [this](const auto &zoomLevelInfo) {
                         return zoomLevelInfo.zoomLevelIdentifier <= maxZoomLevelIdentifier.value();
                     });
        return limitedInfos;
    }

    std::shared_ptr<Tiled2dMapLayerConfig> baseConfig;
    std::optional<int32_t> maxZoomLevelIdentifier;
};

std::optional<int32_t> findMaxSharedZoomLevelIdentifier(const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                                        const std::shared_ptr<Tiled2dMapLayerConfig> &elevationConfig) {
    std::unordered_set<int32_t> elevationZoomLevelIdentifiers;
    for (const auto &zoomLevelInfo : elevationConfig->getZoomLevelInfos()) {
        elevationZoomLevelIdentifiers.insert(zoomLevelInfo.zoomLevelIdentifier);
    }

    std::optional<int32_t> maxSharedZoomLevelIdentifier;
    for (const auto &zoomLevelInfo : layerConfig->getZoomLevelInfos()) {
        if (elevationZoomLevelIdentifiers.find(zoomLevelInfo.zoomLevelIdentifier) == elevationZoomLevelIdentifiers.end()) {
            continue;
        }
        if (!maxSharedZoomLevelIdentifier.has_value() ||
            zoomLevelInfo.zoomLevelIdentifier > maxSharedZoomLevelIdentifier.value()) {
            maxSharedZoomLevelIdentifier = zoomLevelInfo.zoomLevelIdentifier;
        }
    }

    return maxSharedZoomLevelIdentifier;
}

std::pair<std::shared_ptr<Tiled2dMapLayerConfig>, std::shared_ptr<Tiled2dMapLayerConfig>>
limitDisplacedConfigsToSharedZoomLevelDepth(const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                            const std::shared_ptr<Tiled2dMapLayerConfig> &elevationConfig) {
    const auto maxSharedZoomLevelIdentifier = findMaxSharedZoomLevelIdentifier(layerConfig, elevationConfig);
    return {
        std::make_shared<ZoomLevelLimitedLayerConfig>(layerConfig, maxSharedZoomLevelIdentifier),
        std::make_shared<ZoomLevelLimitedLayerConfig>(elevationConfig, maxSharedZoomLevelIdentifier)
    };
}
} // namespace

TiledDisplacedRasterLayer::TiledDisplacedRasterLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                                     const std::shared_ptr<::Tiled2dMapLayerConfig> &elevationConfig,
                                                     const std::vector<std::shared_ptr<::LoaderInterface>> &tileLoaders,
                                                     bool registerToTouchHandler)
    : TiledDisplacedRasterLayer(
        limitDisplacedConfigsToSharedZoomLevelDepth(layerConfig, elevationConfig),
        tileLoaders,
        registerToTouchHandler) {}

TiledDisplacedRasterLayer::TiledDisplacedRasterLayer(
    std::pair<std::shared_ptr<::Tiled2dMapLayerConfig>, std::shared_ptr<::Tiled2dMapLayerConfig>> limitedConfigs,
    const std::vector<std::shared_ptr<::LoaderInterface>> &tileLoaders,
    bool registerToTouchHandler)
    : Tiled2dMapRasterLayer(limitedConfigs.first, tileLoaders, registerToTouchHandler)
    , elevationConfig(limitedConfigs.second) {}

void TiledDisplacedRasterLayer::onAdded(const std::shared_ptr<::MapInterface> &mapInterface, int32_t layerIndex) {
    std::shared_ptr<Mailbox> selfMailbox = mailbox;
    if (!mailbox) {
        selfMailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());
    }

    if (sourceInterfaces.empty() || this->mailbox != selfMailbox) {
        auto castedMe = std::static_pointer_cast<Tiled2dMapRasterLayer>(shared_from_this());
        auto selfActor = WeakActor<Tiled2dMapRasterSourceListener>(selfMailbox, castedMe);

        auto sourceMailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());
        rasterSource.emplaceObject(sourceMailbox, mapInterface->getMapConfig(), layerConfig, elevationConfig,
                                   mapInterface->getCoordinateConverterHelper(), mapInterface->getScheduler(), tileLoaders,
                                   selfActor, mapInterface->getCamera()->getScreenDensityPpi(), layerConfig->getLayerName());
        setSourceMaskTileGeometryTileSelectionOptimization(usesMaskTileGeometry());

        setSourceInterfaces({rasterSource.weakActor<Tiled2dMapSourceInterface>()});
    }

    Tiled2dMapLayer::onAdded(mapInterface, layerIndex);

    if (registerToTouchHandler) {
        mapInterface->getTouchHandler()->insertListener(std::dynamic_pointer_cast<TouchInterface>(shared_from_this()), layerIndex);
    }
}

void TiledDisplacedRasterLayer::setupTiles() {
    auto mapInterface = this->mapInterface;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) {
        return;
    }

    std::vector<Tiled2dMapVersionedTileInfo> tilesReady;
    {
        std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);

        for (const auto &mask : newMaskObjects) {
            const auto &object = mask->asGraphicsObject();
            if (!object->isReady())
                object->setup(mapInterface->getRenderingContext());
        }
        newMaskObjects.clear();

        for (const auto &mask : obsoleteMaskObjects) {
            mask->asGraphicsObject()->clear();
        }
        obsoleteMaskObjects.clear();

        for (const auto &[tile, tileObject] : tilesToClean) {
            tileObject->getGraphicsObject()->clear();
            tileObjectMap.erase(tile);
            tileMaskMap.erase(tile.tileInfo);
        }

        for (const auto &stateUpdate : tileStateUpdates) {
            auto extracted = tileObjectMap.extract(stateUpdate);
            if (extracted) {
                extracted.key() = stateUpdate;
                tileObjectMap.insert(std::move(extracted));
            }
        }
        tileStateUpdates.clear();

        for (const auto &tile : tilesToSetup) {
            const auto &tileInfo = tile.first;
            const auto &tileObject = tile.second;
            if (!tileObject) {
                continue;
            }

            if (!tileObject->getGraphicsObject()->isReady()) {
                tileObject->getGraphicsObject()->setup(renderingContext);
            }

            if (tileInfo.textureHolder) {
                loadTileTextures(tileObject, renderingContext, tileInfo);
            }
            tilesReady.push_back(tileInfo.tileInfo);
        }
    }
    tilesToSetup.clear();

    generateRenderPasses();

    {
        std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
        for (const auto &[tile, tileObject] : tilesToClean) {
            if (!tileObject)
                continue;
            tileObject->removeTexture();
        }
        tilesToClean.clear();
    }

    rasterSource.message(MFN(&TiledDisplacedRasterSource::setTilesReady), tilesReady);

    updateReadyStateListenerIfNeeded();

    mapInterface->invalidate();
}

std::shared_ptr<Quad2dInterface> TiledDisplacedRasterLayer::createQuadForTile(
    const std::shared_ptr<GraphicsObjectFactoryInterface> &graphicsFactory,
    const std::shared_ptr<RasterShaderInterface> &rasterShader,
    bool is3D) {
    (void)rasterShader;
    (void)is3D;
    return graphicsFactory->createQuadTessellatedDisplaced();
}

void TiledDisplacedRasterLayer::loadTileTextures(const std::shared_ptr<Textured2dLayerObject> &tileObject,
                                                 const std::shared_ptr<RenderingContextInterface> &renderingContext,
                                                 const Tiled2dMapRasterTileInfo &tileInfo) {
    tileObject->loadDualTexture(renderingContext, tileInfo.textureHolder, tileInfo.elevationTextureHolder);
}

bool TiledDisplacedRasterLayer::useRenderPassMaskingFor3d() const {
    return false;
}

void TiledDisplacedRasterLayer::setSourceMaskTileGeometryTileSelectionOptimization(bool enabled) {
    rasterSource.message(MFN(&TiledDisplacedRasterSource::setMaskTileGeometryTileSelectionOptimizationEnabled), enabled);
}
