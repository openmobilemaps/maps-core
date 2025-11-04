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
#include <shared_mutex>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

// --- Read-heavy Optimierung: shared_mutex standardmässig aktiv
using shard_mutex_t = std::shared_mutex;
using shard_read_lock_t  = std::shared_lock<shard_mutex_t>;
using shard_write_lock_t = std::unique_lock<shard_mutex_t>;

// Comparator mit heterogenem Lookup (double) + stabilem Tie-Break
struct CoordinatorXCompare {
  using is_transparent = void;

  bool operator()(const std::shared_ptr<SymbolAnimationCoordinator> &a,
                  const std::shared_ptr<SymbolAnimationCoordinator> &b) const noexcept {
    const double ax = a->coordinate.x;
    const double bx = b->coordinate.x;
    if (ax < bx) return true;
    if (bx < ax) return false;
    return std::less<const void*>()(a.get(), b.get());
  }
  bool operator()(const std::shared_ptr<SymbolAnimationCoordinator> &a,
                  const double &x) const noexcept {
    return a->coordinate.x < x;
  }
  bool operator()(const double &x,
                  const std::shared_ptr<SymbolAnimationCoordinator> &b) const noexcept {
    return x < b->coordinate.x;
  }
};

class SymbolAnimationCoordinatorMap {
public:
  SymbolAnimationCoordinatorMap() = default;

  std::shared_ptr<SymbolAnimationCoordinator> getOrAddAnimationController(size_t crossTileIdentifier, const Vec2D &coord,
                                                                          int zoomIdentifier, double xTolerance,
                                                                          double yTolerance, const int64_t animationDuration,
                                                                          const int64_t animationDelay) {
    const size_t shardIndex = hasher(crossTileIdentifier) % numShards;
    auto &shard = shards[shardIndex];

    // 1) read-lock: schnelle Suche ohne Contention
    {
      shard_read_lock_t rlock(shard.mutex);
      if (auto hit = findExistingLocked(shard, crossTileIdentifier, coord, zoomIdentifier, xTolerance)) {
        return hit;
      }
    }

    // 2) write-lock: Double-Check + Insert
    {
      shard_write_lock_t wlock(shard.mutex);
      if (auto hit = findExistingLocked(shard, crossTileIdentifier, coord, zoomIdentifier, xTolerance)) {
        return hit;
      }
      return emplaceNewLocked(shard, crossTileIdentifier, coord, zoomIdentifier,
                              xTolerance, yTolerance, animationDuration, animationDelay);
    }
  }

  bool isAnimating() {
    for (auto &shard : shards) {
      shard_read_lock_t lock(shard.mutex);
      for (const auto &[id, coordinators] : shard.animationCoordinators) {
        (void)id;
        for (const auto &[levelZoomIdentifier, coordinatorSet] : coordinators) {
          (void)levelZoomIdentifier;
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

      for (auto &kv : shard.animationCoordinators) {
        auto &zoomMap = kv.second;

        // Elemente in Sets entfernen
        for (auto &zv : zoomMap) {
          auto &setByX = zv.second;
          std::erase_if(setByX, [](const auto &p){ return !p->isUsed(); });
        }

        // leere Zoom-Buckets entfernen
        std::erase_if(zoomMap, [](const auto &kv2){ return kv2.second.empty(); });
      }

      // leere CrossTile-Einträge entfernen
      std::erase_if(shard.animationCoordinators, [](const auto &kv){ return kv.second.empty(); });
    }
  }

  void enableAnimations(bool enabled) {
    animationsEnabled = enabled;
    for (auto &shard : shards) {
      shard_write_lock_t lock(shard.mutex);
      for (auto &[_, coordinatorMap] : shard.animationCoordinators) {
        for (auto &[__, coordinatorSet] : coordinatorMap) {
          for (auto &coordinator : coordinatorSet) {
            coordinator->enableAnimations(enabled);
          }
        }
      }
    }
  }

private:
  using SetByX  = std::set<std::shared_ptr<SymbolAnimationCoordinator>, CoordinatorXCompare>;
  using ZoomMap = std::unordered_map<int, SetByX>;      // schneller als std::map<int,...> bei wenigen Keys
  using CrossMap= std::unordered_map<size_t, ZoomMap>;

  struct Shard {
    shard_mutex_t mutex;
    CrossMap animationCoordinators;
  };

  static std::shared_ptr<SymbolAnimationCoordinator>
  searchInSetByX(const SetByX &setByX,
                 const Vec2D &coord,
                 int searchZoomIdentifier,
                 int levelZoomIdentifier,
                 double xTolerance)
  {
    const int dz = levelZoomIdentifier - searchZoomIdentifier;
    // 2^max(0,dz) als double (robuster als 1<<dz)
    const double factor = std::ldexp(1.0, std::max(0, dz));
    const double maxXtol = factor * xTolerance;

    const double minX = coord.x - maxXtol;
    const double maxX = coord.x + maxXtol;

    for (auto it = setByX.lower_bound(minX);
         it != setByX.end() && (*it)->coordinate.x <= maxX; ++it) {
      if ((*it)->isMatching(coord, searchZoomIdentifier)) {
        return *it;
      }
    }
    return {};
  }

  std::shared_ptr<SymbolAnimationCoordinator>
  findExistingLocked(Shard &shard,
                     size_t crossTileIdentifier,
                     const Vec2D &coord,
                     int zoomIdentifier,
                     double xTolerance)
  {
    auto ctIt = shard.animationCoordinators.find(crossTileIdentifier);
    if (ctIt == shard.animationCoordinators.end()) return {};

    auto &zoomMap = ctIt->second;

    // 1) gleicher Zoom zuerst
    if (auto itZ = zoomMap.find(zoomIdentifier); itZ != zoomMap.end()) {
      if (auto hit = searchInSetByX(itZ->second, coord, zoomIdentifier, zoomIdentifier, xTolerance)) {
        return hit;
      }
    }
    // 2) andere Zoom-Level
    for (const auto &kv : zoomMap) {
      const int levelZoomIdentifier = kv.first;
      if (levelZoomIdentifier == zoomIdentifier) continue;
      if (auto hit = searchInSetByX(kv.second, coord, zoomIdentifier, levelZoomIdentifier, xTolerance)) {
        return hit;
      }
    }
    return {};
  }

  std::shared_ptr<SymbolAnimationCoordinator>
  emplaceNewLocked(Shard &shard,
                   size_t crossTileIdentifier,
                   const Vec2D &coord,
                   int zoomIdentifier,
                   double xTolerance,
                   double yTolerance,
                   int64_t animationDuration,
                   int64_t animationDelay)
  {
    auto &zoomMap = shard.animationCoordinators.try_emplace(crossTileIdentifier).first->second;
    auto &setForZoom = zoomMap.try_emplace(zoomIdentifier).first->second;

    auto animationCoordinator = std::make_shared<SymbolAnimationCoordinator>(
                                                                             coord, zoomIdentifier, xTolerance, yTolerance, animationDuration, animationDelay);
    animationCoordinator->enableAnimations(animationsEnabled);

    // hinted insert via heterogenem lower_bound
    auto hint = setForZoom.lower_bound(coord.x);
    auto it = setForZoom.emplace_hint(hint, std::move(animationCoordinator));

    return (it != setForZoom.end()) ? *it : nullptr;
  }

private:
  bool animationsEnabled = true;
  static constexpr size_t numShards = 32; // bei Bedarf an CPU-Kerne anpassen
  std::array<Shard, numShards> shards;
  std::hash<size_t> hasher;
};
