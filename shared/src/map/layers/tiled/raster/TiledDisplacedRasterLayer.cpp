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

TiledDisplacedRasterLayer::TiledDisplacedRasterLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                                     const std::shared_ptr<::Tiled2dMapLayerConfig> &elevationConfig,
                                                     const std::vector<std::shared_ptr<::LoaderInterface>> &tileLoaders,
                                                     bool registerToTouchHandler)
    : Tiled2dMapRasterLayer(layerConfig, tileLoaders, registerToTouchHandler)
    , elevationConfig(elevationConfig) {}

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
            const auto &object = mask->asGraphicsObject();
            if (object->isReady())
                object->clear();
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
            if (!tileObject || tileObject->getGraphicsObject()->isReady()) {
                continue;
            }

            tileObject->getGraphicsObject()->setup(renderingContext);

            if (tileInfo.textureHolder) {
                tileObject->getQuadObject()->loadTexture(renderingContext, tileInfo.textureHolder, nullptr);
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
            tileObject->getQuadObject()->removeTexture();
        }
        tilesToClean.clear();
    }

    rasterSource.message(MFN(&TiledDisplacedRasterSource::setTilesReady), tilesReady);

    updateReadyStateListenerIfNeeded();

    mapInterface->invalidate();
}
