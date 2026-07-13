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
#include "MapCamera3dMode.h"
#include "Vec3D.h"

#include <memory>
#include <optional>
#include <vector>

template <class Source> class DefaultTiled2dMap3dTileSelection;
template <class Source> class DisplacedTerrainTiled2dMap3dTileSelection;

template <class Source> class Tiled2dMap3dTileSelection {
  public:
    virtual ~Tiled2dMap3dTileSelection() = default;

    virtual void onCameraChange(Source &source, const std::vector<float> &viewMatrix, const std::vector<float> &projectionMatrix,
                                const ::Vec3D &origin, float verticalFov, float horizontalFov, float width, float height,
                                float focusPointAltitude, const ::Coord &focusPointPosition, float zoom,
                                const std::optional<::Vec3D> &cameraPosition, ::MapCamera3dMode cameraMode) const = 0;
};

template <class Source> std::unique_ptr<Tiled2dMap3dTileSelection<Source>> makeDefaultTiled2dMap3dTileSelection();

template <class Source> std::unique_ptr<Tiled2dMap3dTileSelection<Source>> makeDisplacedTerrainTiled2dMap3dTileSelection();
