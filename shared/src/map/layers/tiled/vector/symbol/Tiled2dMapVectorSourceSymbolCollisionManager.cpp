/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorSourceSymbolCollisionManager.h"
#include "CollisionGrid.h"

void Tiled2dMapVectorSourceSymbolCollisionManager::collisionDetection(const std::vector<float> &vpMatrix, const Vec2I &viewportSize, float viewportRotation, bool enforceRecomputation, bool persistingPlacement, bool is3d) {
    std::vector<std::string> layers;
    std::string currentSource;

    // TODO: Smarter logic to check if rebuilding the CollisionGrid is really necessary
    if (!enforceRecomputation && vpMatrix == lastVpMatrix) {
        return;
    }
    lastVpMatrix = vpMatrix;

    auto collisionGrid = std::make_shared<CollisionGrid>(vpMatrix, viewportSize, viewportRotation, persistingPlacement, is3d);

    const auto lambda = [&collisionGrid, &layers](auto manager){
        if (auto strongManager = manager.lock()) {
            strongManager->collisionDetection(layers, collisionGrid);
        }
    };

    for(auto it = mapDescription->layers.rbegin(); it != mapDescription->layers.rend(); ++it) {
        auto& layer = *it;

        if (layer->getType() != VectorLayerType::symbol) {
            continue;
        }
        if (layer->source != currentSource) {
            if (!currentSource.empty()) {
                symbolSourceDataManagers.at(currentSource).syncAccess(lambda);
            }
            layers.clear();
            currentSource = layer->source;
        }
        layers.push_back(layer->identifier);
    }
    
    if (!currentSource.empty()) {
        symbolSourceDataManagers.at(currentSource).syncAccess(lambda);
    }
}
