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

#include "PrioritizedTiled2dMapTileInfo.h"
#include <functional>
#include "HashedTuple.h"

struct PrioritizedLoadTask {
    PrioritizedTiled2dMapTileInfo prioritizedInfo;
    int subtaskIndex;

    PrioritizedLoadTask(PrioritizedTiled2dMapTileInfo prioritizedInfo, int subtaskIndex)
    : prioritizedInfo(prioritizedInfo)
    , subtaskIndex(subtaskIndex)
    {}

    bool operator==(const PrioritizedLoadTask &o) const {
        return prioritizedInfo == o.prioritizedInfo && subtaskIndex == o.subtaskIndex;
    }

    bool operator<(const PrioritizedLoadTask &o) const {
        return (prioritizedInfo < o.prioritizedInfo) || subtaskIndex < o.subtaskIndex;
    }
};

namespace std {
    template <> struct hash<PrioritizedLoadTask> {
        inline size_t operator()(const PrioritizedLoadTask &task) const {
            auto h = std::hash<PrioritizedTiled2dMapTileInfo>()(task.prioritizedInfo);
            std::hash_combine(h, std::hash<int>{}(task.subtaskIndex));
            return h;
        }
    };
} // namespace std


