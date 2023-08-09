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


void Tiled2dMapVectorReadyManager::registerManager() {
    managerCount += 1;
}

void Tiled2dMapVectorReadyManager::didProcessData(const Tiled2dMapTileInfo &tile, const size_t notReadyCount) {
    auto tileProcessIt = tileDataProcessCount.find(tile);
    if (tileProcessIt != tileDataProcessCount.end()){
        tileProcessIt->second += 1;
        if (tileProcessIt->second == managerCount && notReadyCount == 0) {
            auto tileIt = tileNotReadyCount.find(tile);
            if (tileIt == tileNotReadyCount.end()) {
                tileDataProcessCount.erase(tile);
                tileNotReadyCount.erase(tile);
                vectorSource.message(&Tiled2dMapSourceReadyInterface::setTileReady, tile);
            }
        }
    } else if (managerCount == 1 && notReadyCount == 0) {
        vectorSource.message(&Tiled2dMapSourceReadyInterface::setTileReady, tile);
    } else {
        tileDataProcessCount.insert({tile, 1});
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

void Tiled2dMapVectorReadyManager::setReady(const Tiled2dMapTileInfo &tile, const size_t readyCount) {
    auto tileIt = tileNotReadyCount.find(tile);
    if (tileIt != tileNotReadyCount.end()){
        tileIt->second -= readyCount;
        if (tileIt->second <= 0) {
            tileNotReadyCount.erase(tile);

            auto tileProcessIt = tileDataProcessCount.find(tile);
            if (tileProcessIt->second == managerCount) {
                tileDataProcessCount.erase(tile);
                vectorSource.message(&Tiled2dMapSourceReadyInterface::setTileReady, tile);
            }
        }
    }
}
