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
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

using shard_mutex_t = std::shared_mutex;
using shard_read_lock_t = std::shared_lock<shard_mutex_t>;
using shard_write_lock_t = std::unique_lock<shard_mutex_t>;

struct CoordinatorXCompare {
    using is_transparent = void;

    bool operator()(const std::shared_ptr<SymbolAnimationCoordinator> &a,
                    const std::shared_ptr<SymbolAnimationCoordinator> &b) const noexcept {
        if (a->coordinate.x < b->coordinate.x)
            return true;
        if (b->coordinate.x < a->coordinate.x)
            return false;
        // stable tie-break to avoid comparator collisions
        return std::less<const void *>()(a.get(), b.get());
    }

    bool operator()(const std::shared_ptr<SymbolAnimationCoordinator> &a, const double &value) const noexcept {
        return a->coordinate.x < value;
    }

    bool operator()(const double &value, const std::shared_ptr<SymbolAnimationCoordinator> &b) const noexcept {
        return value < b->coordinate.x;
    }
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

        // First try a read lock for fast path
        {
            shard_read_lock_t rlock(shard.mutex);
            if (auto hit = findExistingLocked(shard, crossTileIdentifier, coord, zoomIdentifier, xTolerance)) {
                return hit;
            }
        }

        // Upgrade to write lock and double-check before inserting
        {
            shard_write_lock_t wlock(shard.mutex);
            if (auto hit = findExistingLocked(shard, crossTileIdentifier, coord, zoomIdentifier, xTolerance)) {
                return hit;
            }
            return emplaceNewLocked(shard, crossTileIdentifier, coord, zoomIdentifier, xTolerance, yTolerance, animationDuration,
                                    animationDelay);
        }
    }

    bool isAnimating() {
        for (auto &shard : shards) {
            shard_read_lock_t lock(shard.mutex);
            for (const auto &[_, coordinators] : shard.animationCoordinators) {
                for (const auto &[_, coordinatorSet] : coordinators) {
                    for (const auto &coordinator : coordinatorSet) {
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
            shard_write_lock_t lock(shard.mutex);

            for (auto it = shard.animationCoordinators.begin(); it != shard.animationCoordinators.end();) {
                auto &zoomMap = it->second;

                // prune unused coordinators per zoom
                for (auto zmIt = zoomMap.begin(); zmIt != zoomMap.end();) {
                    auto &setByX = zmIt->second;

                    for (auto sIt = setByX.begin(); sIt != setByX.end();) {
                        if (!(*sIt)->isUsed()) {
                            sIt = setByX.erase(sIt);
                        } else {
                            ++sIt;
                        }
                    }

                    if (setByX.empty()) {
                        zmIt = zoomMap.erase(zmIt);
                    } else {
                        ++zmIt;
                    }
                }

                if (zoomMap.empty()) {
                    it = shard.animationCoordinators.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    void enableAnimations(bool enabled) {
        animationsEnabled = enabled;
        for (auto &shard : shards) {
            shard_write_lock_t lock(shard.mutex);
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
    using SetByX = std::set<std::shared_ptr<SymbolAnimationCoordinator>, CoordinatorXCompare>;
    using ZoomMap = std::unordered_map<int, SetByX>;

    struct Shard {
        shard_mutex_t mutex;
        std::unordered_map<size_t, ZoomMap> animationCoordinators;
    };

    static std::shared_ptr<SymbolAnimationCoordinator> searchInSetByX(const SetByX &coordinators, const Vec2D &coord,
                                                                      int searchZoomIdentifier, int levelZoomIdentifier,
                                                                      double xTolerance) {
        // tolerance grows with higher stored zoom levels; ldexp is fast and precise
        const double factor = std::ldexp(1.0, std::max(0, levelZoomIdentifier - searchZoomIdentifier));
        const double maxXTolerance = factor * xTolerance;

        const double minX = coord.x - maxXTolerance;
        const double maxX = coord.x + maxXTolerance;

        // heterogeneous lower_bound avoids temporary node
        for (auto it = coordinators.lower_bound(minX); it != coordinators.end() && (*it)->coordinate.x <= maxX; ++it) {
            if ((*it)->isMatching(coord, searchZoomIdentifier)) {
                return *it;
            }
        }
        return {};
    }

    std::shared_ptr<SymbolAnimationCoordinator> findExistingLocked(Shard &shard, size_t crossTileIdentifier, const Vec2D &coord,
                                                                   int zoomIdentifier, double xTolerance) {
        auto ctIt = shard.animationCoordinators.find(crossTileIdentifier);
        if (ctIt == shard.animationCoordinators.end())
            return {};

        auto &zoomMap = ctIt->second;

        // 1) same zoom first
        if (auto zIt = zoomMap.find(zoomIdentifier); zIt != zoomMap.end()) {
            if (auto hit = searchInSetByX(zIt->second, coord, zoomIdentifier, zoomIdentifier, xTolerance)) {
                return hit;
            }
        }
        // 2) other zooms
        for (const auto &kv : zoomMap) {
            const int levelZoomIdentifier = kv.first;
            if (levelZoomIdentifier == zoomIdentifier)
                continue;
            if (auto hit = searchInSetByX(kv.second, coord, zoomIdentifier, levelZoomIdentifier, xTolerance)) {
                return hit;
            }
        }
        return {};
    }

    std::shared_ptr<SymbolAnimationCoordinator> emplaceNewLocked(Shard &shard, size_t crossTileIdentifier, const Vec2D &coord,
                                                                 int zoomIdentifier, double xTolerance, double yTolerance,
                                                                 const int64_t animationDuration, const int64_t animationDelay) {
        auto &zoomMap = shard.animationCoordinators.try_emplace(crossTileIdentifier).first->second;
        auto &coordinators = zoomMap.try_emplace(zoomIdentifier).first->second;

        auto animationCoordinator = std::make_shared<SymbolAnimationCoordinator>(coord, zoomIdentifier, xTolerance, yTolerance,
                                                                                 animationDuration, animationDelay);
        animationCoordinator->enableAnimations(animationsEnabled);

        // hinted insert using heterogeneous lower_bound on x
        auto hint = coordinators.lower_bound(coord.x);
        auto it = coordinators.emplace_hint(hint, std::move(animationCoordinator));
        return (it != coordinators.end()) ? *it : nullptr;
    }

  private:
    bool animationsEnabled = true;
    static constexpr size_t numShards = 32;
    std::array<Shard, numShards> shards;
    std::hash<size_t> hash;
};
