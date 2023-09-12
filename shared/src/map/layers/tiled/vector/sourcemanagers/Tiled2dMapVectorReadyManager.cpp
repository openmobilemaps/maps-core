/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorReadyManager.h"

Tiled2dMapVectorReadyManager::Tiled2dMapVectorReadyManager(const WeakActor<Tiled2dMapSourceReadyInterface> vectorSource): vectorSource(vectorSource) {}


size_t Tiled2dMapVectorReadyManager::registerManager() {
    size_t newVal = 1 << (managerCount++);
    managerCountControlVal += newVal;
    return newVal;
}

void Tiled2dMapVectorReadyManager::didProcessData(size_t managerIndex, const Tiled2dMapTileInfo &tile, const size_t notReadyCount) {
    auto tileProcessIt = tileDataProcessCount.find(tile);
    if (tileProcessIt != tileDataProcessCount.end()){
        tileProcessIt->second |= managerIndex;
        if (tileProcessIt->second == managerCountControlVal && notReadyCount == 0) {
            auto tileIt = tileNotReadyCount.find(tile);
            if (tileIt == tileNotReadyCount.end()) {
                tileDataProcessCount.erase(tileProcessIt);
                vectorSource.message(&Tiled2dMapSourceReadyInterface::setTileReady, tile);
                return;
            }
        }
    } else if (managerCount == 1 && notReadyCount == 0) {
        vectorSource.message(&Tiled2dMapSourceReadyInterface::setTileReady, tile);
        return;
    } else {
        tileDataProcessCount.insert({tile, managerIndex});
    }

    if (notReadyCount != 0) {
        auto tileIt = tileNotReadyCount.find(tile);
        if (tileIt != tileNotReadyCount.end()){
            tileIt->second += notReadyCount;
        } else {
            tileNotReadyCount.insert({tile, notReadyCount});
        }
    }
}

void Tiled2dMapVectorReadyManager::setReady(size_t managerIndex, const Tiled2dMapTileInfo &tile, const size_t readyCount) {
    auto tileIt = tileNotReadyCount.find(tile);
    if (tileIt != tileNotReadyCount.end()){
        tileIt->second -= readyCount;
        if (tileIt->second <= 0) {
            auto tileProcessIt = tileDataProcessCount.find(tile);
            if (tileProcessIt->second == managerCountControlVal) {
                tileDataProcessCount.erase(tileProcessIt);
                tileNotReadyCount.erase(tileIt);
                vectorSource.message(&Tiled2dMapSourceReadyInterface::setTileReady, tile);
            }
        }
    }
}

void Tiled2dMapVectorReadyManager::remove(const std::unordered_set<Tiled2dMapTileInfo> &tilesToRemove) {
    for (const auto &tile: tilesToRemove) {
        auto tileIt = tileNotReadyCount.find(tile);
        if (tileIt != tileNotReadyCount.end()){
            tileNotReadyCount.erase(tileIt);
        }
        auto tileProcessIt = tileDataProcessCount.find(tile);
        if (tileProcessIt != tileDataProcessCount.end()) {
            tileDataProcessCount.erase(tileProcessIt);
        }
    }
}
