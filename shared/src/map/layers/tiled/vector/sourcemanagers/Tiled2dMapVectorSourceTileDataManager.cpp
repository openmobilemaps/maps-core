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

void Tiled2dMapVectorSourceTileDataManager::update() {
    for (const auto &[tileInfo, subTiles] : tiles) {
        const auto tileMaskWrapper = tileMaskMap.find(tileInfo);
        const auto tileState = tileStateMap.find(tileInfo);
        if (tilesReady.count(tileInfo) > 0 && tileMaskWrapper != tileMaskMap.end() && tileState != tileStateMap.end() && tileState->second == TileState::VISIBLE) {
            for (const auto &[index, identifier, tile]: subTiles) {
                tile.syncAccess([](auto t) {
                    t->update();
                });
            }
        }
    }
}


void Tiled2dMapVectorSourceTileDataManager::pregenerateRenderPasses() {
    std::vector<std::shared_ptr<Tiled2dMapVectorLayer::TileRenderDescription>> renderDescriptions;
    for (const auto &[tile, subTiles] : tileRenderObjectsMap) {
        const auto tileMaskWrapper = tileMaskMap.find(tile);
        const auto tileState = tileStateMap.find(tile);
        if (tilesReady.count(tile) > 0 && tileMaskWrapper != tileMaskMap.end() && tileState != tileStateMap.end() && tileState->second == TileState::VISIBLE) {
            const auto &mask = tileMaskWrapper->second.getGraphicsMaskObject();
            for (const auto &[layerIndex, renderObjects]: subTiles) {
                const bool modifiesMask = modifyingMaskLayers.find(layerIndex) != modifyingMaskLayers.end();
                renderDescriptions.push_back(std::make_shared<Tiled2dMapVectorLayer::TileRenderDescription>(Tiled2dMapVectorLayer::TileRenderDescription{layerIndex, renderObjects, mask, modifiesMask}));
            }
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
            tile.syncAccess([](const auto &t){
                t->clear();
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
            tile.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorTile::setup);
        }

        tilesReadyControlSet[tileInfo] = controlSet;
    }
}

void Tiled2dMapVectorSourceTileDataManager::setAlpha(float alpha) {
    for (const auto &[tileInfo, subTiles] : tiles) {
        for (const auto &[index, identifier, tile]: subTiles) {
            tile.message(&Tiled2dMapVectorTile::setAlpha, alpha);
        }
    }
}

void Tiled2dMapVectorSourceTileDataManager::setScissorRect(const std::optional<RectI> &scissorRect) {
    Tiled2dMapVectorSourceDataManager::setScissorRect(scissorRect);
    pregenerateRenderPasses();
}

void Tiled2dMapVectorSourceTileDataManager::setSelectionDelegate(const std::shared_ptr<Tiled2dMapVectorLayerSelectionCallbackInterface> &selectionDelegate) {
    Tiled2dMapVectorSourceDataManager::setSelectionDelegate(selectionDelegate);
    for (const auto &[tileInfo, subTiles] : tiles) {
        for (const auto &[index, identifier, tile] : subTiles) {
            tile.message(&Tiled2dMapVectorTile::setSelectionDelegate, selectionDelegate);
        }
    }
}

void Tiled2dMapVectorSourceTileDataManager::setSelectedFeatureIdentifier(std::optional<int64_t> identifier) {
    Tiled2dMapVectorSourceDataManager::setSelectedFeatureIdentifier(identifier);
    for (const auto &[tileInfo, subTiles] : tiles) {
        for (const auto &[index, layerIdentifier, tile] : subTiles) {
            tile.message(&Tiled2dMapVectorTile::setSelectedFeatureIdentifier, identifier);
        }
    }
}

void Tiled2dMapVectorSourceTileDataManager::updateMaskObjects(
        const std::unordered_map<Tiled2dMapTileInfo, Tiled2dMapLayerMaskWrapper> &toSetupMaskObject,
        const std::unordered_set<Tiled2dMapTileInfo> &tilesToRemove) {
    auto mapInterface = this->mapInterface.lock();
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) return;

    for (const auto &[tileInfo, wrapper] : toSetupMaskObject) {
        wrapper.getGraphicsObject()->setup(renderingContext);

        std::shared_ptr<GraphicsObjectInterface> toClear;
        auto it = tileMaskMap.find(tileInfo);
        if (it != tileMaskMap.end() && it->second.getGraphicsMaskObject()) {
            toClear = it->second.getGraphicsMaskObject()->asGraphicsObject();
        }
        tileMaskMap[tileInfo] = wrapper;

        if (toClear) {
            toClear->clear();
        }
    }

    for (const auto &tileToRemove: tilesToRemove) {
        auto tilesVectorIt = tiles.find(tileToRemove);
        if (tilesVectorIt != tiles.end()) {
            for (const auto &[index, identifier, tile]: tiles.at(tileToRemove)) {
                tile.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorTile::clear);
            }
        }
        tiles.erase(tileToRemove);

        tileStateMap.erase(tileToRemove);

        auto maskIt = tileMaskMap.find(tileToRemove);
        if (maskIt != tileMaskMap.end()) {
            const auto &object = maskIt->second.getGraphicsMaskObject()->asGraphicsObject();
            if (object->isReady()) object->clear();

            tileMaskMap.erase(tileToRemove);
        }

        tilesReady.erase(tileToRemove);
        tilesReadyControlSet.erase(tileToRemove);
        tileRenderObjectsMap.erase(tileToRemove);
    }

    pregenerateRenderPasses();

    mapInterface->invalidate();
}

Actor<Tiled2dMapVectorTile> Tiled2dMapVectorSourceTileDataManager::createTileActor(const Tiled2dMapTileInfo &tileInfo,
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

            auto lineActor = Actor<Tiled2dMapVectorLineTile>(mailbox, (std::weak_ptr<MapInterface>) mapInterface, tileInfo,
                                                             selfActor, std::static_pointer_cast<LineVectorLayerDescription>(
                            layerDescription));

            actor = lineActor.strongActor<Tiled2dMapVectorTile>();
            break;
        }
        case VectorLayerType::polygon: {
            auto mailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());

            auto polygonDescription = std::static_pointer_cast<PolygonVectorLayerDescription>(layerDescription);

            if (polygonDescription->style.hasPatternPotentially()) {
                auto polygonActor = Actor<Tiled2dMapVectorPolygonPatternTile>(mailbox, (std::weak_ptr<MapInterface>) mapInterface,
                                                                           tileInfo, selfActor,
                                                                           polygonDescription,
                                                                              spriteData, spriteTexture);
                actor = polygonActor.strongActor<Tiled2dMapVectorTile>();
            } else {
                auto polygonActor = Actor<Tiled2dMapVectorPolygonTile>(mailbox, (std::weak_ptr<MapInterface>) mapInterface,
                                                                       tileInfo, selfActor,
                                                                       polygonDescription);
                actor = polygonActor.strongActor<Tiled2dMapVectorTile>();
            }

            break;
        }
        case VectorLayerType::symbol: {
            break;
        }
        case VectorLayerType::raster: {
            auto mailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());

            auto rasterActor = Actor<Tiled2dMapVectorRasterTile>(mailbox, (std::weak_ptr<MapInterface>) mapInterface,
                                                                 tileInfo, selfActor,
                                                                 std::static_pointer_cast<RasterVectorLayerDescription>(
                                                                         layerDescription));

            actor = rasterActor.strongActor<Tiled2dMapVectorTile>();
            break;
        }
        case VectorLayerType::custom: {
            break;
        }
    }
    return actor;
}

void Tiled2dMapVectorSourceTileDataManager::tileIsReady(const Tiled2dMapTileInfo &tile,
                                                        const std::string &layerIdentifier,
                                                        const WeakActor<Tiled2dMapVectorTile> &tileActor) {
    if (!tileActor) {
        return;
    }
    const auto &renderObjects = tileActor.syncAccess([](const auto &t){
        if (auto strongT = t.lock()) {
            return strongT->generateRenderObjects();
        } else {
            return std::vector<std::shared_ptr<RenderObjectInterface>>{};
        }
    });

    if (tilesReady.count(tile) > 0) {
        return;
    }

    int32_t layerIndex = layerNameIndexMap.at(layerIdentifier);

    bool isCompletelyReady = false;
    {
        const auto &tileControlSet = tilesReadyControlSet.find(tile);
        if (tileControlSet == tilesReadyControlSet.end()) {
            return;
        }
        tileControlSet->second.erase(layerIndex);
        if (tileControlSet->second.empty()) {
            tilesReadyControlSet.erase(tile);
            tilesReady.insert(tile);
            isCompletelyReady = true;
        }
    }

    auto tileRenderObjects = tileRenderObjectsMap.find(tile);
    if (tileRenderObjects != tileRenderObjectsMap.end() && !tileRenderObjects->second.empty()) {
        // Remove invalid legacy tile (only one - identifier is unique)
        for (auto renderObjIter = tileRenderObjects->second.begin(); renderObjIter != tileRenderObjects->second.end(); renderObjIter++) {
            if (std::get<0>(*renderObjIter) == layerIndex) {
                tileRenderObjects->second.erase(renderObjIter);
                break;
            }
        }
        tileRenderObjectsMap[tile].emplace_back(layerIndex, renderObjects);
    } else {
        tileRenderObjectsMap[tile].emplace_back(layerIndex, renderObjects);
    }

    if (isCompletelyReady) {
        onTileCompletelyReady(tile);
        pregenerateRenderPasses();
    }
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
            std::get<2>(*rIter).message(&Tiled2dMapVectorTile::clearTouch);
        }

    }
}

void Tiled2dMapVectorSourceTileDataManager::setSprites(std::shared_ptr<SpriteData> spriteData, std::shared_ptr<TextureHolderInterface> spriteTexture) {
    this->spriteData = spriteData;
    this->spriteTexture = spriteTexture;

    if (!tiles.empty()) {
        auto selfActor = WeakActor(mailbox, weak_from_this());
        selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSourceTileDataManager::setupExistingTilesWithSprite);
    }
}

void Tiled2dMapVectorSourceTileDataManager::setupExistingTilesWithSprite() {
    for (const auto &[tile, subTiles] : tiles) {
        for (const auto &[index, string, actor]: subTiles) {
            actor.message(&Tiled2dMapVectorTile::setSpriteData, spriteData, spriteTexture);
        }
    }

    pregenerateRenderPasses();
}
