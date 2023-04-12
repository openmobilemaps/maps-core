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

void Tiled2dMapVectorSourceSymbolCollisionManager::collisionDetection() {
    std::unordered_set<std::string> layers;
    std::string currentSource;

    std::shared_ptr<std::vector<OBB2D>> placements = std::make_shared<std::vector<OBB2D>>();

    const auto lambda = [&placements, &layers](auto manager){
        if (auto strongManager = manager.lock()) {
            strongManager->collisionDetection(layers, placements);
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
        layers.insert(layer->identifier);
    }
    
    if (!currentSource.empty()) {
        symbolSourceDataManagers.at(currentSource).syncAccess(lambda);
    }
}

void Tiled2dMapVectorSourceSymbolCollisionManager::update() {
    const auto lambda = [](auto manager){
        if (auto strongManager = manager.lock()) {
            strongManager->update();
        }
    };

    for(const auto &[source, symbolManager] : symbolSourceDataManagers) {
        symbolManager.syncAccess(lambda);
    }
}
