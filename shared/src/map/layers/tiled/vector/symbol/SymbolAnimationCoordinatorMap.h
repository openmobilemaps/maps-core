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

struct CoordinatorXCompare {
    bool operator() (const std::shared_ptr<SymbolAnimationCoordinator> &a, const std::shared_ptr<SymbolAnimationCoordinator> &b) {
        return a->coordinate.x < b->coordinate.x;
    };
};

class SymbolAnimationCoordinatorMap {
public:

    std::shared_ptr<SymbolAnimationCoordinator> getOrAddAnimationController(size_t crossTileIdentifier,
                                                                            const Coord &coord,
                                                                            int zoomIdentifier,
                                                                            double xTolerance,
                                                                            double yTolerance,
                                                                            const int64_t animationDuration,
                                                                            const int64_t animationDelay) {
        std::lock_guard<std::mutex> lock(mapMutex);
        auto coordinatorIt = animationCoordinators.find(crossTileIdentifier);
        if (coordinatorIt != animationCoordinators.end()) {
            size_t numElements = std::get<1>(coordinatorIt->second).size();

            double toleranceFactor = 1 << std::max(0, std::get<0>(coordinatorIt->second) - zoomIdentifier);
            double maxXTolerance = toleranceFactor * xTolerance;

            std::set<std::shared_ptr<SymbolAnimationCoordinator>, CoordinatorXCompare>::const_iterator targetIt = std::get<1>(coordinatorIt->second).begin();
            if (numElements > MIN_NUM_SORTED) {
                targetIt = std::lower_bound(std::get<1>(coordinatorIt->second).begin(),
                                                 std::get<1>(coordinatorIt->second).end(),
                                                 coord.x,
                                                 [](const std::shared_ptr<SymbolAnimationCoordinator> &a, const double &value) {
                                                     return a->coordinate.x < value;
                                                 });
            }

            while (targetIt != std::get<1>(coordinatorIt->second).end()) {
                if (targetIt->get()->isMatching(coord, zoomIdentifier)) {
                    return *targetIt;
                } else if (targetIt->get()->coordinate.x > coord.x + maxXTolerance) {
                    break;
                }
                targetIt++;
            }

            auto animationCoordinator = std::make_shared<SymbolAnimationCoordinator>(coord, zoomIdentifier, xTolerance, yTolerance, animationDuration, animationDelay);
            std::get<1>(coordinatorIt->second).insert(animationCoordinator);
            if (zoomIdentifier < std::get<0>(coordinatorIt->second)) {
                std::get<0>(coordinatorIt->second) = zoomIdentifier;
            }
            return animationCoordinator;
        }

        auto animationCoordinator = std::make_shared<SymbolAnimationCoordinator>(coord, zoomIdentifier, xTolerance, yTolerance,
                                                                                 animationDuration, animationDelay);
        animationCoordinators.insert({crossTileIdentifier, {zoomIdentifier, {animationCoordinator}}});
        return animationCoordinator;
    }

    bool isAnimating() {
        std::lock_guard<std::mutex> lock(mapMutex);
        for (const auto &[id, coordinators]: animationCoordinators) {
            for (const auto &coordinator: std::get<1>(coordinators)) {
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
            for (auto coordsIt = std::get<1>(it->second).begin(); coordsIt != std::get<1>(it->second).end(); ) {
                if (!coordsIt->get()->isUsed()) {
                    coordsIt = std::get<1>(it->second).erase(coordsIt);
                }
                else {
                    ++coordsIt;
                }
            }
            if (std::get<1>(it->second).empty()) {
                animationCoordinators.erase(it);
            }
        }
    }

private:
    static const size_t MIN_NUM_SORTED = 100;

    std::mutex mapMutex;
    std::unordered_map<size_t, std::tuple<int, std::set<std::shared_ptr<SymbolAnimationCoordinator>, CoordinatorXCompare>>> animationCoordinators;
};
