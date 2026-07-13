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

#include "Tiled2dMap3dTileDetailSelector.h"

class Tiled2dMap3dDistanceBasedTerrainSelector final : public Tiled2dMap3dTileDetailSelector {
  public:
    bool needsTerrainDesiredLevelIndex() const override { return true; }
    bool retainsTilesUntilReplacementReady() const override { return true; }
    bool isPreciseEnough(const Tiled2dMap3dTileDetailSelectionContext &context) const override {
        return context.candidateLevelIndex >= context.terrainDesiredLevelIndex;
    }
};
