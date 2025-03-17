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
#include <mutex>
#include <unordered_map>
#include <map>
#include <set>
#include <memory>
#include <vector>
#include <functional>
#include <algorithm>

struct CoordinatorXCompare {
    bool operator() (const std::shared_ptr<SymbolAnimationCoordinator> &a, const std::shared_ptr<SymbolAnimationCoordinator> &b) const {
        return a->coordinate.x < b->coordinate.x;
    };
};

class SymbolAnimationCoordinatorMap {
public:
    SymbolAnimationCoordinatorMap() {}

    std::shared_ptr<SymbolAnimationCoordinator> getOrAddAnimationController(size_t crossTileIdentifier,
                                                                            const Vec2D &coord,
                                                                            int zoomIdentifier,
                                                                            double xTolerance,
                                                                            double yTolerance,
                                                                            const int64_t animationDuration,
                                                                            const int64_t animationDelay) {
        size_t shardIndex = hash(crossTileIdentifier) % numShards;
        auto &shard = shards[shardIndex];
        std::lock_guard<std::mutex> lock(shard.mutex);

        auto coordinatorIt = shard.animationCoordinators.find(crossTileIdentifier);
        if (coordinatorIt != shard.animationCoordinators.end()) {
            for (auto &[levelZoomIdentifier, coordinators]: coordinatorIt->second) {
                if (levelZoomIdentifier == zoomIdentifier) {
                    // limit comparisons with equal zoomIdentifier entries
                    continue;
                }

                double toleranceFactor = 1 << std::max(0, levelZoomIdentifier - zoomIdentifier);
                double maxXTolerance = toleranceFactor * xTolerance;

                auto targetIt = std::lower_bound(coordinators.begin(),
                                            coordinators.end(),
                                            coord.x,
                                            [](const std::shared_ptr<SymbolAnimationCoordinator> &a, const double &value) {
                                                return a->coordinate.x < value;
                                            });

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
        shard.animationCoordinators.insert({crossTileIdentifier, newMap});
        return animationCoordinator;
    }

    bool isAnimating() {
        for (auto &shard : shards) {
            std::lock_guard<std::mutex> lock(shard.mutex);
            for (const auto &[id, coordinators]: shard.animationCoordinators) {
                for (const auto &[levelZoomIdentifier, coordinatorSet]: coordinators) {
                    for (const auto &coordinator: coordinatorSet) {
                        if (coordinator->isAnimating()) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    void clearAnimationCoordinators() {
        for (auto &shard : shards) {
            std::lock_guard<std::mutex> lock(shard.mutex);
            for (auto it = shard.animationCoordinators.begin(), next_it = it; it != shard.animationCoordinators.end(); it = next_it) {
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
                    shard.animationCoordinators.erase(it);
                }
            }
        }
    }

    void enableAnimations(bool enabled) {
        animationsEnabled = enabled;
        for (auto &shard : shards) {
            std::lock_guard<std::mutex> lock(shard.mutex);
            for (const auto &[_, coordinatorMap] : shard.animationCoordinators) {
                for (const auto &[_, coordinatorSet] : coordinatorMap) {
                    for (const auto &coordinator : coordinatorSet) {
                        coordinator->enableAnimations(enabled);
                    }
                }
            }
        }
    }

private:
    struct Shard {
        std::mutex mutex;
        std::unordered_map<size_t, std::map<int, std::set<std::shared_ptr<SymbolAnimationCoordinator>, CoordinatorXCompare>>> animationCoordinators;
    };

    bool animationsEnabled = true;
    static constexpr size_t numShards = 32;
    std::array<Shard, numShards> shards;
    std::hash<size_t> hash;
};