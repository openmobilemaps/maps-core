/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include <map>
#include "MapInterface.h"
#include "Tiled2dMapVectorSubLayer.h"
#include "VectorTilePolygonHandler.h"
#include "RenderObject.h"
#include "RenderPass.h"
#include "LambdaTask.h"

void Tiled2dMapVectorSubLayer::update() {

}

std::vector<std::shared_ptr<::RenderPassInterface>> Tiled2dMapVectorSubLayer::buildRenderPasses() {
    std::vector<std::shared_ptr<RenderPassInterface>> newRenderPasses;
    for (const auto &tilePasses : renderPasses) {
        newRenderPasses.insert(newRenderPasses.end(), tilePasses.second.begin(), tilePasses.second.end());
    }
    return newRenderPasses;
}

std::vector<std::shared_ptr<::RenderPassInterface>>
Tiled2dMapVectorSubLayer::buildRenderPasses(const std::unordered_set<Tiled2dMapTileInfo> &tiles) {
    std::vector<std::shared_ptr<RenderPassInterface>> newRenderPasses;
    for (const auto &tilePasses : renderPasses) {
        if (tiles.count(tilePasses.first) > 0) {
            newRenderPasses.insert(newRenderPasses.end(), tilePasses.second.begin(), tilePasses.second.end());
        }
    }
    return newRenderPasses;
}

void Tiled2dMapVectorSubLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface, int32_t layerIndex) {
    this->mapInterface = mapInterface;
}

void Tiled2dMapVectorSubLayer::onRemoved() {
    this->mapInterface = nullptr;
}

void Tiled2dMapVectorSubLayer::pause() {
}

void Tiled2dMapVectorSubLayer::resume() {
}

void Tiled2dMapVectorSubLayer::hide() {
    // TODO
}

void Tiled2dMapVectorSubLayer::show() {
    // TODO
}


void Tiled2dMapVectorSubLayer::updateTileData(const Tiled2dMapTileInfo &tileInfo, const std::shared_ptr<MaskingObjectInterface> &tileMask,
                            const std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>> &layerFeatures) {
    if (!mapInterface) return;

    std::lock_guard<std::recursive_mutex> lock(maskMutex);
    tileMaskMap[tileInfo] = tileMask;
}

void Tiled2dMapVectorSubLayer::updateTileMask(const Tiled2dMapTileInfo &tileInfo, const std::shared_ptr<MaskingObjectInterface> &tileMask) {
    if (!mapInterface) return;

    std::lock_guard<std::recursive_mutex> lock(maskMutex);
    tileMaskMap[tileInfo] = tileMask;
}

void Tiled2dMapVectorSubLayer::clearTileData(const Tiled2dMapTileInfo &tileInfo) {
    {
        std::lock_guard<std::recursive_mutex> lock(maskMutex);
        const auto &maskObject = tileMaskMap[tileInfo];
        if (maskObject) {
            tileMaskMap.erase(tileInfo);
        }
    }
    {
        std::lock_guard<std::recursive_mutex> lock(tilesInSetupMutex);
        tilesInSetup.erase(tileInfo);
    }
}

void Tiled2dMapVectorSubLayer::setMaskingObject(const std::shared_ptr<::MaskingObjectInterface> & maskingObject) {}

void Tiled2dMapVectorSubLayer::setTilesReadyDelegate(const std::weak_ptr<Tiled2dMapVectorLayerReadyInterface> readyDelegate) {
    this->readyDelegate = readyDelegate;
}


void Tiled2dMapVectorSubLayer::setSelectionDelegate(const std::weak_ptr<Tiled2dMapVectorLayerSelectionInterface> selectionDelegate) {
    this->selectionDelegate = selectionDelegate;
}

void Tiled2dMapVectorSubLayer::setSelectedFeatureIdentfier(std::optional<int64_t> identifier) {
    {
        std::lock_guard<std::recursive_mutex> lock(selectedFeatureIdentifierMutex);
        selectedFeatureIdentifier = identifier;
    }
    auto mapInterface = this->mapInterface;
    if (mapInterface) {
        mapInterface->invalidate();
    }
}


void Tiled2dMapVectorSubLayer::setAlpha(float alpha) {
    if (this->alpha == alpha) {
        return;
    }
    this->alpha = alpha;

    auto mapInterface = this->mapInterface;
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

float Tiled2dMapVectorSubLayer::getAlpha() { return alpha; }

