/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorSourceTileDataManager.h"
#include "Tiled2dMapVectorRasterTile.h"
#include "Tiled2dMapVectorPolygonTile.h"
#include "Tiled2dMapVectorPolygonPatternTile.h"
#include "Tiled2dMapVectorLineTile.h"
#include "Tiled2dMapVectorLayer.h"
#include "RenderPass.h"
#include "PerformanceLogger.h"

void Tiled2dMapVectorSourceTileDataManager::update() {
    if (!noPendingUpdateMasks.test_and_set()) {
        updateMaskObjects();
    }

    for (const auto &[tileInfo, subTiles] : tiles) {
        const auto tileState = tileStateMap.find(tileInfo);

        if (tilesReady.count(tileInfo) == 0) {
            // Tile is not ready
            continue;
        }

        if (tileState == tileStateMap.end() || tileState->second == TileState::CACHED) {
            // Tile is in cached state
            continue;
        }

        for (const auto &[index, identifier, tile]: subTiles) {
            PERF_LOG_START(identifier + "_update");
            tile.syncAccess([](auto t) {
                t->update();
            });
            PERF_LOG_END(identifier + "_update");
        }
    }
}


void Tiled2dMapVectorSourceTileDataManager::pregenerateRenderPasses() {
    std::vector<std::shared_ptr<Tiled2dMapVectorLayer::TileRenderDescription>> renderDescriptions;
    bool maskTile = layerConfig->getZoomInfo().maskTile;
    for (const auto &[tile, subTiles] : tileRenderObjectsMap) {
        if (tilesReady.count(tile) == 0) {
            // Tile is not ready
            continue;
        }

        const auto tileState = tileStateMap.find(tile);
        if (tileState == tileStateMap.end() || tileState->second != TileState::VISIBLE) {
            // Tile is not visible or the mask is not yet ready
            continue;
        }

        const auto tileMaskWrapper = tileMaskMap.find(tile);
        if (tileMaskWrapper == tileMaskMap.end() && maskTile) {
            // There is no mask for this tile
            continue;
        }

        const std::shared_ptr<MaskingObjectInterface> &mask = maskTile ? tileMaskWrapper->second.getGraphicsMaskObject() : nullptr;
        assert(!mask || mask->asGraphicsObject()->isReady());
        for (const auto &[layerIndex, renderObjects]: subTiles) {
            const bool modifiesMask = modifyingMaskLayers.find(layerIndex) != modifyingMaskLayers.end();
            const bool selfMasked = selfMaskedLayers.find(layerIndex) != selfMaskedLayers.end();
            const auto optRenderPassIndex = mapDescription->layers[layerIndex]->renderPassIndex;
            const int32_t renderPassIndex = optRenderPassIndex ? *optRenderPassIndex : 0;
            renderDescriptions.push_back(std::make_shared<Tiled2dMapVectorLayer::TileRenderDescription>(Tiled2dMapVectorLayer::TileRenderDescription{layerIndex, sourceHash, tile.tileInfo.zoomIdentifier, renderObjects, mask, modifiesMask, selfMasked, renderPassIndex}));
        }
    }
    vectorLayer.syncAccess([source = this->source, &renderDescriptions](const auto &layer){
        if(auto strong = layer.lock()) {
            strong->onRenderPassUpdate(source, false, renderDescriptions);
        }
    });
}

void Tiled2dMapVectorSourceTileDataManager::pause() {
    for (const auto &tileMask: tileMaskMap) {
        if (tileMask.second.getGraphicsObject() &&
            tileMask.second.getGraphicsObject()->isReady()) {
            tileMask.second.getGraphicsObject()->clear();
        }
    }

    for (const auto &[tileInfo, subTiles] : tiles) {
        for (const auto &[index, identifier, tile]: subTiles) {
            tile.syncAccess([](const auto &tile) {
                tile->clear();
            });
        }
    }

    tilesReady.clear();
    tilesReadyControlSet.clear();
    tileRenderObjectsMap.clear();
}

void Tiled2dMapVectorSourceTileDataManager::resume() {
    auto mapInterface = this->mapInterface.lock();
    const auto &context = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!context) {
        return;
    }

    for (const auto &tileMask: tileMaskMap) {
        if (tileMask.second.getGraphicsObject() && !tileMask.second.getGraphicsObject()->isReady()) {
            tileMask.second.getGraphicsObject()->setup(context);
        }
    }

    for (const auto &[tileInfo, subTiles] : tiles) {
        if (tilesReadyControlSet.count(tileInfo) > 0 || tilesReady.count(tileInfo) > 0) {
            continue;
        }

        std::unordered_set<int32_t> controlSet = {};
        for (const auto &[index, identifier, tile]: subTiles) {
            controlSet.insert(index);
            tile.syncAccess([](const auto &tile) {
                tile->setup();
            });
        }

        tilesReadyControlSet[tileInfo] = controlSet;
    }
}

void Tiled2dMapVectorSourceTileDataManager::setAlpha(float alpha) {
    this->alpha = alpha;
    for (const auto &[tileInfo, subTiles] : tiles) {
        for (const auto &[index, identifier, tile]: subTiles) {
            tile.message(MFN(&Tiled2dMapVectorTile::setAlpha), alpha);
        }
    }
}

void Tiled2dMapVectorSourceTileDataManager::setSelectionDelegate(const std::weak_ptr<Tiled2dMapVectorLayerSelectionCallbackInterface> &selectionDelegate) {
    Tiled2dMapVectorSourceDataManager::setSelectionDelegate(selectionDelegate);
    for (const auto &[tileInfo, subTiles] : tiles) {
        for (const auto &[index, identifier, tile] : subTiles) {
            tile.message(MFN(&Tiled2dMapVectorTile::setSelectionDelegate), selectionDelegate);
        }
    }
}

void Tiled2dMapVectorSourceTileDataManager::updateMaskObjects() {
    auto mapInterface = this->mapInterface.lock();
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) return;

    {
        std::lock_guard<std::recursive_mutex> updateLock(updateMutex);

        for (const auto &[tile, state]: tileStateUpdates) {
            tileStateMap[tile] = state;
        }

        tileStateUpdates.clear();

        for (const auto &[tileInfo, wrapper]: tileMasksToSetup) {
            wrapper.getGraphicsObject()->setup(renderingContext);

            std::shared_ptr<GraphicsObjectInterface> toClear = nullptr;
            auto it = tileMaskMap.find(tileInfo);
            if (it != tileMaskMap.end()) {
                auto obj = it->second.getGraphicsMaskObject();
                toClear = obj ? obj->asGraphicsObject() : nullptr;
                it->second = wrapper;
            } else {
                tileMaskMap[tileInfo] = wrapper;
            }

            if (toClear) {
                toClear->clear();
            }
        }
        tileMasksToSetup.clear();

        for (const auto &tileToRemove: tilesToRemove) {
            auto tilesVectorIt = tiles.find(tileToRemove);
            if (tilesVectorIt != tiles.end()) {
                for (const auto &[index, identifier, tile] : tilesVectorIt->second) { // Access the value directly
                    tile.unsafe()->clear();
                }
                tiles.erase(tilesVectorIt); // Use the iterator to erase directly
            }

            tileStateMap.erase(tileToRemove);

            auto maskIt = tileMaskMap.find(tileToRemove);
            if (maskIt != tileMaskMap.end()) {
                const auto &object = maskIt->second.getGraphicsMaskObject()->asGraphicsObject();
                if (object->isReady()) { object->clear(); }

                tileMaskMap.erase(maskIt);
            }

            tilesReady.erase(tileToRemove);
            tilesReadyControlSet.erase(tileToRemove);
            tileRenderObjectsMap.erase(tileToRemove);
        }

        std::unordered_set<Tiled2dMapVersionedTileInfo> localToRemove = std::unordered_set(tilesToRemove);
        tilesToRemove.clear();
        readyManager.syncAccess([localToRemove](auto manager) {
            manager->remove(localToRemove);
        });
    }

    pregenerateRenderPasses();

    mapInterface->invalidate();
}

Actor<Tiled2dMapVectorTile> Tiled2dMapVectorSourceTileDataManager::createTileActor(const Tiled2dMapVersionedTileInfo &tileInfo,
                                                                                   const std::shared_ptr<VectorLayerDescription> &layerDescription) {
    Actor<Tiled2dMapVectorTile> actor;

    auto mapInterface = this->mapInterface.lock();
    if (!mapInterface) {
        return actor;
    }

    auto castedMe = std::static_pointer_cast<Tiled2dMapVectorLayerTileCallbackInterface>(shared_from_this());
    auto selfActor = WeakActor<Tiled2dMapVectorLayerTileCallbackInterface>(mailbox, castedMe);

    switch (layerDescription->getType()) {
        case VectorLayerType::background: {
            break;
        }
        case VectorLayerType::line: {
            auto mailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());

            auto lineActor = Actor<Tiled2dMapVectorLineTile>(mailbox, (std::weak_ptr<MapInterface>) mapInterface, vectorLayer.unsafe(), tileInfo,
                                                             selfActor,
                                                             std::static_pointer_cast<LineVectorLayerDescription>(layerDescription),
                                                             layerConfig,
                                                             featureStateManager);

            actor = lineActor.strongActor<Tiled2dMapVectorTile>();
            break;
        }
        case VectorLayerType::polygon: {
            auto mailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());

            auto polygonDescription = std::static_pointer_cast<PolygonVectorLayerDescription>(layerDescription);

            if (polygonDescription->style.hasPatternPotentially()) {
                auto polygonActor = Actor<Tiled2dMapVectorPolygonPatternTile>(mailbox, (std::weak_ptr<MapInterface>) mapInterface, vectorLayer.unsafe(),
                                                                           tileInfo, selfActor, polygonDescription, layerConfig,
                                                                           spriteData, spriteTexture,
                                                                              featureStateManager);
                actor = polygonActor.strongActor<Tiled2dMapVectorTile>();
            } else {
                auto polygonActor = Actor<Tiled2dMapVectorPolygonTile>(mailbox, (std::weak_ptr<MapInterface>) mapInterface, vectorLayer.unsafe(),
                                                                       tileInfo, selfActor, polygonDescription, layerConfig,
                                                                       featureStateManager);
                actor = polygonActor.strongActor<Tiled2dMapVectorTile>();
            }

            break;
        }
        case VectorLayerType::symbol: {
            break;
        }
        case VectorLayerType::raster: {
            auto mailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());

            auto rasterActor = Actor<Tiled2dMapVectorRasterTile>(mailbox, (std::weak_ptr<MapInterface>) mapInterface, vectorLayer.unsafe(),
                                                                 tileInfo, selfActor,
                                                                 std::static_pointer_cast<RasterVectorLayerDescription>(
                                                                         layerDescription),
                                                                 layerConfig,
                                                                 featureStateManager);

            actor = rasterActor.strongActor<Tiled2dMapVectorTile>();
            break;
        }
        case VectorLayerType::custom: {
            break;
        }
    }
    if (actor) {
        actor.unsafe()->setAlpha(alpha);
    }
    return actor;
}

void Tiled2dMapVectorSourceTileDataManager::tileIsReady(const Tiled2dMapVersionedTileInfo &tile,
                                                        const std::string &layerIdentifier,
                                                        const WeakActor<Tiled2dMapVectorTile> &tileActor) {
    if (!tileActor) {
        return;
    }

    const auto &tilesIt = tiles.find(tile);
    if (tilesIt == tiles.end()) {
        return;
    }

    bool found = false;
    for (auto const &[index, string, actor]: tilesIt->second) {
        if (layerIdentifier == string && actor.unsafe() == tileActor.unsafe().lock()) {
            found = true;
            break;
        }
    }

    if (!found) {
        return;
    }

    if (tilesReady.count(tile) > 0) {
        return;
    }

    const auto &renderObjects = tileActor.syncAccess([](const auto &t){
        if (auto strongT = t.lock()) {
            return strongT->generateRenderObjects();
        } else {
            return std::vector<std::shared_ptr<RenderObjectInterface>>{};
        }
    });

    int32_t layerIndex = layerNameIndexMap.at(layerIdentifier);

    bool isCompletelyReady = false;
    {
        const auto &tileControlSet = tilesReadyControlSet.find(tile);
        if (tileControlSet != tilesReadyControlSet.end()) {
            tileControlSet->second.erase(layerIndex);
            if (tileControlSet->second.empty()) {
                tilesReadyControlSet.erase(tileControlSet);
                tilesReady.insert(tile);
                isCompletelyReady = true;
            }
        }
    }

    auto tileRenderObjects = tileRenderObjectsMap.find(tile);
    if (tileRenderObjects != tileRenderObjectsMap.end()) {
        if( !tileRenderObjects->second.empty()) {
            // Remove invalid legacy tile (only one - identifier is unique)
            for (auto renderObjIter = tileRenderObjects->second.begin(); renderObjIter != tileRenderObjects->second.end(); renderObjIter++) {
                if (std::get<0>(*renderObjIter) == layerIndex) {
                    tileRenderObjects->second.erase(renderObjIter);
                    break;
                }
            }
        }

        tileRenderObjects->second.emplace_back(layerIndex, renderObjects);
    } else {
        tileRenderObjectsMap[tile].emplace_back(layerIndex, renderObjects);
    }

    if(!isCompletelyReady) {
        return;
    }

    auto tileStateMapIt = tileStateMap.find(tile);
    if (tileStateMapIt != tileStateMap.end() && tileStateMapIt->second == TileState::VISIBLE) {
        pregenerateRenderPasses();
        return;
    }

    onTileCompletelyReady(tile);
}

void Tiled2dMapVectorSourceTileDataManager::tileIsInteractable(const std::string &layerIdentifier) {
    interactableLayers.insert(layerIdentifier);
}

bool
Tiled2dMapVectorSourceTileDataManager::onClickUnconfirmed(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) {
    if (interactableLayers.empty()) {
        return false;
    }
    for (const auto &[tileInfo, subTiles]: tiles) {
        const auto tileState = tileStateMap.find(tileInfo);
        if (tileState == tileStateMap.end() || tileState->second != TileState::VISIBLE) {
            continue;
        }
        for (auto rIter = subTiles.rbegin(); rIter != subTiles.rend(); rIter++) {
            if (interactableLayers.count(std::get<1>(*rIter)) == 0 || layers.count(std::get<1>(*rIter)) == 0) {
                continue;
            }
            bool hit = std::get<2>(*rIter).syncAccess([posScreen](auto tile) {
                return tile->onClickUnconfirmed(posScreen);
            });
            if (hit) {
                return true;
            }
        }
    }
    return false;
}

bool
Tiled2dMapVectorSourceTileDataManager::onClickConfirmed(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) {
    if (interactableLayers.empty()) {
        return false;
    }
    for (const auto &[tileInfo, subTiles]: tiles) {
        const auto tileState = tileStateMap.find(tileInfo);
        if (tileState == tileStateMap.end() || tileState->second != TileState::VISIBLE) {
            continue;
        }
        for (auto rIter = subTiles.rbegin(); rIter != subTiles.rend(); rIter++) {
            if (interactableLayers.count(std::get<1>(*rIter)) == 0 || layers.count(std::get<1>(*rIter)) == 0) {
                continue;
            }
            bool hit = std::get<2>(*rIter).syncAccess([posScreen](auto tile) {
                return tile->onClickConfirmed(posScreen);
            });
            if (hit) {
                return true;
            }
        }
    }
    return false;
}

bool Tiled2dMapVectorSourceTileDataManager::performClick(const std::unordered_set<std::string> &layers, const Coord &coord) {
    if (interactableLayers.empty()) {
        return false;
    }
    for (const auto &[tileInfo, subTiles]: tiles) {
        const auto tileState = tileStateMap.find(tileInfo);
        if (tileState == tileStateMap.end() || tileState->second != TileState::VISIBLE) {
            continue;
        }
        for (auto rIter = subTiles.rbegin(); rIter != subTiles.rend(); rIter++) {
            if (interactableLayers.count(std::get<1>(*rIter)) == 0 || layers.count(std::get<1>(*rIter)) == 0) {
                continue;
            }
            bool hit = std::get<2>(*rIter).syncAccess([coord](auto tile) {
                return tile->performClick(coord);
            });
            if (hit) {
                return true;
            }
        }
    }
    return false;
}


bool Tiled2dMapVectorSourceTileDataManager::onDoubleClick(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) {
    if (interactableLayers.empty()) {
        return false;
    }
    for (const auto &[tileInfo, subTiles]: tiles) {
        const auto tileState = tileStateMap.find(tileInfo);
        if (tileState == tileStateMap.end() || tileState->second != TileState::VISIBLE) {
            continue;
        }
        for (auto rIter = subTiles.rbegin(); rIter != subTiles.rend(); rIter++) {
            if (interactableLayers.count(std::get<1>(*rIter)) == 0 || layers.count(std::get<1>(*rIter)) == 0) {
                continue;
            }
            bool hit = std::get<2>(*rIter).syncAccess([posScreen](auto tile) {
                return tile->onDoubleClick(posScreen);
            });
            if (hit) {
                return true;
            }
        }
    }
    return false;
}

bool Tiled2dMapVectorSourceTileDataManager::onLongPress(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) {
    if (interactableLayers.empty()) {
        return false;
    }
    for (const auto &[tileInfo, subTiles]: tiles) {
        const auto tileState = tileStateMap.find(tileInfo);
        if (tileState == tileStateMap.end() || tileState->second != TileState::VISIBLE) {
            continue;
        }
        for (auto rIter = subTiles.rbegin(); rIter != subTiles.rend(); rIter++) {
            if (interactableLayers.count(std::get<1>(*rIter)) == 0 || layers.count(std::get<1>(*rIter)) == 0) {
                continue;
            }
            bool hit = std::get<2>(*rIter).syncAccess([posScreen](auto tile) {
                return tile->onLongPress(posScreen);
            });
            if (hit) {
                return true;
            }
        }
    }
    return false;
}

bool Tiled2dMapVectorSourceTileDataManager::onTwoFingerClick(const std::unordered_set<std::string> &layers, const Vec2F &posScreen1,
                                                             const Vec2F &posScreen2) {
    if (interactableLayers.empty()) {
        return false;
    }
    for (const auto &[tileInfo, subTiles]: tiles) {
        const auto tileState = tileStateMap.find(tileInfo);
        if (tileState == tileStateMap.end() || tileState->second != TileState::VISIBLE) {
            continue;
        }
        for (auto rIter = subTiles.rbegin(); rIter != subTiles.rend(); rIter++) {
            if (interactableLayers.count(std::get<1>(*rIter)) == 0 || layers.count(std::get<1>(*rIter)) == 0) {
                continue;
            }
            bool hit = std::get<2>(*rIter).syncAccess([&posScreen1, &posScreen2](auto tile) {
                return tile->onTwoFingerClick(posScreen1, posScreen2);
            });
            if (hit) {
                return true;
            }
        }
    }
    return false;
}

void Tiled2dMapVectorSourceTileDataManager::clearTouch() {
    if (interactableLayers.empty()) {
        return;
    }
    for (const auto &[tileInfo, subTiles]: tiles) {
        const auto tileState = tileStateMap.find(tileInfo);
        if (tileState == tileStateMap.end() || tileState->second != TileState::VISIBLE) {
            continue;
        }
        for (auto rIter = subTiles.rbegin(); rIter != subTiles.rend(); rIter++) {
            std::get<2>(*rIter).message(MFN(&Tiled2dMapVectorTile::clearTouch));
        }

    }
}

void Tiled2dMapVectorSourceTileDataManager::setSprites(std::shared_ptr<SpriteData> spriteData, std::shared_ptr<TextureHolderInterface> spriteTexture) {
    this->spriteData = spriteData;
    this->spriteTexture = spriteTexture;

    if (!tiles.empty()) {
        auto selfActor = WeakActor(mailbox, weak_from_this());
        selfActor.message(MFN(&Tiled2dMapVectorSourceTileDataManager::setupExistingTilesWithSprite));
    }
}

void Tiled2dMapVectorSourceTileDataManager::setupExistingTilesWithSprite() {
    for (const auto &[tile, subTiles] : tiles) {
        for (const auto &[index, string, actor]: subTiles) {
            actor.message(MailboxExecutionEnvironment::graphics, MFN(&Tiled2dMapVectorTile::setSpriteData), spriteData, spriteTexture);
        }
    }

    pregenerateRenderPasses();
}
