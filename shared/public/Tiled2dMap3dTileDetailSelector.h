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

#include "Coord.h"
#include "CoordinateConversionHelperInterface.h"
#include "RectCoord.h"
#include "Vec3D.h"

#include <vector>

struct Tiled2dMap3dTileDetailSelectionContext {
    int candidateLevelIndex;
    int terrainDesiredLevelIndex;
    int32_t layerSystemId;
    float focusPointAltitude;
    float width;
    float height;
    double zoomLevelScaleFactor;
    const Coord &focusPointInLayerCoords;
    const Coord &tileCenter;
    const RectCoord &tileBounds;
    const std::vector<float> &viewMatrix;
    const std::vector<float> &projectionMatrix;
    const Vec3D &origin;
    CoordinateConversionHelperInterface &conversionHelper;
};

class Tiled2dMap3dTileDetailSelector {
  public:
    virtual ~Tiled2dMap3dTileDetailSelector() = default;

    virtual bool needsTerrainDesiredLevelIndex() const { return false; }
    virtual bool retainsTilesUntilReplacementReady() const { return false; }
    virtual bool isPreciseEnough(const Tiled2dMap3dTileDetailSelectionContext &context) const = 0;
};

class Tiled2dMap3dScreenSpaceEdgeLengthSelector final : public Tiled2dMap3dTileDetailSelector {
  public:
    bool isPreciseEnough(const Tiled2dMap3dTileDetailSelectionContext &context) const override;

  private:
    static Vec3D transformToView(const Coord &position, const std::vector<float> &viewMatrix, const Vec3D &origin,
                                 CoordinateConversionHelperInterface &conversionHelper);
    static Vec3D projectToScreen(const Vec3D &position, const std::vector<float> &projectionMatrix);
};
