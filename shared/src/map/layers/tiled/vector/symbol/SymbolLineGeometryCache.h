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

#include "CoordinateConversionHelperInterface.h"
#include "Vec2DHelper.h"
#include "Vec2I.h"
#include "Vec3D.h"
#include "Vec3DHelper.h"
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

struct LineSegmentIndex {
    int index = 0;
    double percentage = 0.0;

    LineSegmentIndex() = default;
    LineSegmentIndex(int index_, double percentage_) : index(index_), percentage(percentage_) {}
};

class SymbolLineGeometryCache {
public:
    static std::shared_ptr<SymbolLineGeometryCache> create(const std::shared_ptr<CoordinateConversionHelperInterface> &converter,
                                                          const std::vector<Vec2D> &lineCoordinates,
                                                          int32_t systemIdentifier,
                                                          bool is3d);

    size_t vertexCount() const { return renderLineCoordinates.size(); }
    bool empty() const { return renderLineCoordinates.empty(); }

    void ensureScreenProjection3D(const std::vector<float> &vpMatrix, const Vec3D &origin, const Vec2I &viewportSize);

    Vec2D renderPointAt(int segmentIndex, double percentage, bool reversed) const;
    Vec2D screenPointAt(int segmentIndex, double percentage, bool reversed) const;
    Vec2D tilePointAt(int segmentIndex, double percentage, bool reversed) const;

    LineSegmentIndex findReferencePoint(const Vec3D &point, bool reversed) const;

    void indexAtDistance(const LineSegmentIndex &index, const Vec2D &currentPoint, double distance, bool reversed, LineSegmentIndex &result) const;

    const std::vector<Vec3D> &cartesianVertices() const { return cartesianRenderLineCoordinates; }

private:
    SymbolLineGeometryCache() = default;

    void buildCartesianVertices();

    size_t mapArrayIndex(size_t arrayIndex, bool reversed) const;

    Vec2D pointOnSegment(const std::vector<Vec3D> &coordinates, int segmentIndex, double percentage, bool reversed) const;

    Vec2D pointOnTileSegment(int segmentIndex, double percentage, bool reversed) const;

    std::vector<Vec2D> tileLineCoordinates;
    std::vector<Vec3D> renderLineCoordinates;
    std::vector<Vec3D> screenLineCoordinates;
    std::vector<Vec3D> cartesianRenderLineCoordinates;

    bool is3d = false;

    Vec3D lastProjectionOrigin = Vec3D(0, 0, 0);
    Vec2I lastViewportSize = Vec2I(0, 0);
    std::vector<float> lastVpMatrix;
    bool screenProjectionValid = false;
};
