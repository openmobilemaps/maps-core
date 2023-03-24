/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorInteractionManager.h"
#include <functional>

Tiled2dMapVectorInteractionManager::Tiled2dMapVectorInteractionManager(
        const std::unordered_map<std::string, WeakActor<Tiled2dMapVectorSourceDataManager>> &sourceDataManagers,
        const std::shared_ptr<VectorMapDescription> &mapDescription)
        : sourceDataManagers(sourceDataManagers), mapDescription(mapDescription) {
}

bool Tiled2dMapVectorInteractionManager::onTouchDown(const Vec2F &posScreen) {
    return false;
}

bool Tiled2dMapVectorInteractionManager::onClickUnconfirmed(const Vec2F &posScreen) {
    const auto lambda = [&posScreen](std::unordered_set<std::string> &layers, auto &manager) -> bool {
        if (auto strongManager = manager.lock()) {
            return strongManager->onClickUnconfirmed(layers, posScreen);
        }
        return false;
    };
    return callInReverseOrder(lambda);;
}

bool Tiled2dMapVectorInteractionManager::onClickConfirmed(const Vec2F &posScreen) {
    const auto lambda = [&posScreen](std::unordered_set<std::string> &layers, auto &manager) -> bool {
        if (auto strongManager = manager.lock()) {
            return strongManager->onClickConfirmed(layers, posScreen);
        }
        return false;
    };
    return callInReverseOrder(lambda);
}

bool Tiled2dMapVectorInteractionManager::onDoubleClick(const Vec2F &posScreen) {
    const auto lambda = [&posScreen](std::unordered_set<std::string> &layers, auto &manager) -> bool {
        if (auto strongManager = manager.lock()) {
            return strongManager->onDoubleClick(layers, posScreen);
        }
        return false;
    };
    return callInReverseOrder(lambda);
}

bool Tiled2dMapVectorInteractionManager::onLongPress(const Vec2F &posScreen) {
    const auto lambda = [&posScreen](std::unordered_set<std::string> &layers, auto &manager) -> bool {
        if (auto strongManager = manager.lock()) {
            return strongManager->onLongPress(layers, posScreen);
        }
        return false;
    };
    return callInReverseOrder(lambda);
}

bool Tiled2dMapVectorInteractionManager::onMove(const Vec2F &deltaScreen, bool confirmed, bool doubleClick) {
    return false;
}

bool Tiled2dMapVectorInteractionManager::onMoveComplete() {
    return false;
}

bool Tiled2dMapVectorInteractionManager::onTwoFingerClick(const Vec2F &posScreen1, const Vec2F &posScreen2) {
    const auto lambda =
            [&posScreen1, &posScreen2](std::unordered_set<std::string> &layers, auto &manager) -> bool {
                if (auto strongManager = manager.lock()) {
                    return strongManager->onTwoFingerClick(layers, posScreen1, posScreen2);
                }
                return false;
            };
    return callInReverseOrder(lambda);
}

bool Tiled2dMapVectorInteractionManager::onTwoFingerMove(const std::vector<::Vec2F> &posScreenOld,
                                                         const std::vector<::Vec2F> &posScreenNew) {
    return false;
}

bool Tiled2dMapVectorInteractionManager::onTwoFingerMoveComplete() {
    return false;
}

void Tiled2dMapVectorInteractionManager::clearTouch() {
    for (const auto &[source, sourceDataManager] : sourceDataManagers) {
        sourceDataManager.syncAccess([](auto &manager){
            if (auto strongManager = manager.lock()) {
                strongManager->clearTouch();
            }
        });
    }
}

template<typename F>
bool Tiled2dMapVectorInteractionManager::callInReverseOrder(F&& managerLambda) {
    if (!mapDescription) {
        return false;
    }
    std::unordered_set<std::string> layers;
    std::string currentSource;

    for (auto revIt = mapDescription->layers.rbegin(); revIt != mapDescription->layers.rend(); revIt++) {
        const auto &layer = *revIt;
        if (layer->source != currentSource) {
            if (!currentSource.empty()) {
                auto reducedLambda = std::bind(managerLambda, layers, std::placeholders::_1);
                if (sourceDataManagers.at(currentSource).syncAccess(reducedLambda)) {
                    return true;
                }
            }
            layers.clear();
            currentSource = layer->source;
        }
        layers.insert(layer->identifier);
    }
    if (!currentSource.empty()) {
        auto reducedLambda = std::bind(managerLambda, layers, std::placeholders::_1);
        return sourceDataManagers.at(currentSource).syncAccess(reducedLambda);
    }
    return false;
}
