/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "BoundingBox.h"

#include "CoordinateConversionHelperInterface.h"
#include "CoordinateSystemIdentifiers.h"

#include <algorithm>
#include <limits>

std::shared_ptr<BoundingBoxInterface> BoundingBoxInterface::create(int32_t systemIdentifier) {
    return std::make_shared<BoundingBox>(systemIdentifier);
}

BoundingBox::BoundingBox()
    : systemIdentifier(CoordinateSystemIdentifiers::RENDERSYSTEM())
    , min(CoordinateSystemIdentifiers::RENDERSYSTEM(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max())
    , max(CoordinateSystemIdentifiers::RENDERSYSTEM(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
      std::numeric_limits<float>::lowest()) {}

BoundingBox::BoundingBox(const int32_t systemIdentifier)
    : systemIdentifier(systemIdentifier)
    , min(systemIdentifier, std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max())
    , max(systemIdentifier, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
          std::numeric_limits<float>::lowest()) {}

BoundingBox::BoundingBox(const Coord& p):
    systemIdentifier(p.systemIdentifier),
    min(p),
    max(p) {}

BoundingBox::operator bool() {
    return min.x != std::numeric_limits<float>::max() ||
           min.y != std::numeric_limits<float>::max() ||
           max.x != std::numeric_limits<float>::min() ||
           max.y != std::numeric_limits<float>::min();
}

void BoundingBox::addPoint(const Coord &p) {
    auto const &conv = CoordinateConversionHelperInterface::independentInstance()->convert(systemIdentifier, p);
    addPoint(conv.x, conv.y, conv.z);
}

void BoundingBox::addPoint(const double x, const double y, const double z) {
    min.x = std::min(x, min.x);
    min.y = std::min(y, min.y);
    min.z = std::min(z, min.z);
    max.x = std::max(x, max.x);
    max.y = std::max(y, max.y);
    max.z = std::max(z, max.z);
}

void BoundingBox::addBox(const std::optional<BoundingBox> &box) {
    if (!box) {
        return;
    }

    addPoint(box->min);
    addPoint(box->max);
}

Coord BoundingBox::center() const {
    return Coord(systemIdentifier, 0.5 * (max.x + min.x), 0.5 * (max.y + min.y), 0.5 * (max.z + min.z));
}

RectCoord BoundingBox::asRectCoord() {
    auto minCoord = Coord(systemIdentifier, min.x, min.y, min.z);
    auto maxCoord = Coord(systemIdentifier, max.x, max.y, max.z);

    return RectCoord(minCoord, maxCoord);
}

Coord BoundingBox::getCenter() {
    return center();
}

Coord BoundingBox::getMin() {
    return min;
}

Coord BoundingBox::getMax() {
    return max;
}

int32_t BoundingBox::getSystemIdentifier() {
    return systemIdentifier;
}
