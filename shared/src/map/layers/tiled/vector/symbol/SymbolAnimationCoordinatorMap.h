/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "SymbolAnimationCoordinator.h"

class SymbolAnimationCoordinatorMap {
public:
    std::shared_ptr<SymbolAnimationCoordinator> getOrAddAnimationController(size_t crossTileIdentifier, const Coord &coord, int zoomIdentifier, double xTolerance, double yTolerance) {
        std::lock_guard<std::mutex> lock(mapMutex);
        auto coordinatorIt = animationCoordinators.find(crossTileIdentifier);
        if (coordinatorIt != animationCoordinators.end()) {
            for (auto const &coordinator: coordinatorIt->second) {
                if (coordinator->isMatching(coord, zoomIdentifier)) {
                    return coordinator;
                }
            }
        }

        auto animationCoordinator = std::make_shared<SymbolAnimationCoordinator>(coord, zoomIdentifier, xTolerance, yTolerance);
        animationCoordinators.insert({crossTileIdentifier, { animationCoordinator }});
        return animationCoordinator;
    }

    bool isAnimating() {
        std::lock_guard<std::mutex> lock(mapMutex);
        for (const auto &[id, coordinators]: animationCoordinators) {
            for (const auto &coordinator: coordinators) {
                if (coordinator->isAnimating()) {
                    return true;
                }
            }
        }
        return false;
    }

    void clearAnimationCoordinators() {
        std::lock_guard<std::mutex> lock(mapMutex);
        for (auto it = animationCoordinators.begin(), next_it = it; it != animationCoordinators.end(); it = next_it) {
            ++next_it;
            it->second.erase(std::remove_if(it->second.begin(), it->second.end(),
                                            [](auto coordinator) { return !coordinator->isUsed(); }), it->second.end());
            if (it->second.empty()) {
                animationCoordinators.erase(it);
            }
        }
    }

private:
    std::mutex mapMutex;
    std::unordered_map<size_t, std::vector<std::shared_ptr<SymbolAnimationCoordinator>>> animationCoordinators;
};
