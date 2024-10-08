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
    bool operator() (const std::shared_ptr<SymbolAnimationCoordinator> &a, const std::shared_ptr<SymbolAnimationCoordinator> &b) const {
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
            for (auto &[levelZoomIdentifier, coordinators]: coordinatorIt->second) {
                size_t numElements = coordinators.size();
                if (levelZoomIdentifier == zoomIdentifier && numElements <= MIN_NUM_SEARCH) {
                    // limit comparisons with equal zoomIdentifier entries
                    continue;
                }

                double toleranceFactor = 1 << std::max(0, levelZoomIdentifier - zoomIdentifier);
                double maxXTolerance = toleranceFactor * xTolerance;

                std::set<std::shared_ptr<SymbolAnimationCoordinator>, CoordinatorXCompare>::const_iterator targetIt = coordinators.begin();
                if (numElements > MIN_NUM_SEARCH) {
                    targetIt = std::lower_bound(coordinators.begin(),
                                                coordinators.end(),
                                                coord.x,
                                                [](const std::shared_ptr<SymbolAnimationCoordinator> &a, const double &value) {
                                                    return a->coordinate.x < value;
                                                });
                }

                while (targetIt != coordinators.end()) {
                    if (targetIt->get()->isMatching(coord, zoomIdentifier)) {
                        return *targetIt;
                    } else if (targetIt->get()->coordinate.x > coord.x + maxXTolerance) {
                        break;
                    }
                    targetIt++;
                }

                auto animationCoordinator = std::make_shared<SymbolAnimationCoordinator>(coord, zoomIdentifier, xTolerance,
                                                                                         yTolerance, animationDuration,
                                                                                         animationDelay);
                coordinators.insert(animationCoordinator);
                return animationCoordinator;
            }
        }

        auto animationCoordinator = std::make_shared<SymbolAnimationCoordinator>(coord, zoomIdentifier, xTolerance, yTolerance,
                                                                                 animationDuration, animationDelay);
        animationCoordinator->enableAnimations(animationsEnabled);
        std::set<std::shared_ptr<SymbolAnimationCoordinator>, CoordinatorXCompare> newSet;
        newSet.insert(animationCoordinator);
        std::map<int, std::set<std::shared_ptr<SymbolAnimationCoordinator>, CoordinatorXCompare>> newMap;
        newMap.insert(std::make_pair(zoomIdentifier, newSet));
        animationCoordinators.insert({crossTileIdentifier, newMap});
        return animationCoordinator;
    }

    bool isAnimating() {
        std::lock_guard<std::mutex> lock(mapMutex);
        for (const auto &[id, coordinators]: animationCoordinators) {
            for (const auto &[levelZoomIdentifier, coordinatorSet]: coordinators) {
                for (const auto &coordinator: coordinatorSet) {
                    if (coordinator->isAnimating()) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    void clearAnimationCoordinators() {
        std::lock_guard<std::mutex> lock(mapMutex);
        for (auto it = animationCoordinators.begin(), next_it = it; it != animationCoordinators.end(); it = next_it) {
            ++next_it;
            for (auto setIt = it->second.begin(), nextSetIt = setIt; setIt != it->second.end(); setIt = nextSetIt) {
                ++nextSetIt;
                for (auto coordsIt = setIt->second.begin(); coordsIt != setIt->second.end();) {
                    if (!coordsIt->get()->isUsed()) {
                        coordsIt = setIt->second.erase(coordsIt);
                    } else {
                        ++coordsIt;
                    }
                }
                if (setIt->second.empty()) {
                    it->second.erase(setIt);
                }
            }
            if (it->second.empty()) {
                animationCoordinators.erase(it);
            }
        }
    }

    void enableAnimations(bool enabled) {
        animationsEnabled = enabled;
        std::lock_guard<std::mutex> lock(mapMutex);
        for (const auto &[_, coordinatorMap] : animationCoordinators) {
            for (const auto &[_, coordinatorSet] : coordinatorMap) {
                for (const auto &coordinator : coordinatorSet) {
                    coordinator->enableAnimations(enabled);
                }
            }
        }
    }

private:
    static const size_t MIN_NUM_SEARCH = 9;

    bool animationsEnabled = true;

    std::mutex mapMutex;
    std::unordered_map<size_t, std::map<int, std::set<std::shared_ptr<SymbolAnimationCoordinator>, CoordinatorXCompare>>> animationCoordinators;
};
