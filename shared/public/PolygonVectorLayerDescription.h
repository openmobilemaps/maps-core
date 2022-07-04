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

#include "VectorLayerDescription.h"
#include "Color.h"

class PolygonVectorStyle {
public:
    std::shared_ptr<Value<Color>> color;
    std::shared_ptr<Value<float>> opacity;

    PolygonVectorStyle(std::shared_ptr<Value<Color>> color = nullptr,
                       std::shared_ptr<Value<float>> opacity = nullptr):
    color(color),
    opacity(opacity) {}
};

class PolygonVectorLayerDescription: public VectorLayerDescription {
public:
    VectorLayerType getType() override { return VectorLayerType::polygon; };
    PolygonVectorStyle style;

    PolygonVectorLayerDescription(std::string identifier,
                               std::string sourceId,
                               int minZoom,
                               int maxZoom,
                               std::shared_ptr<Value<bool>> filter,
                               PolygonVectorStyle style):
    VectorLayerDescription(identifier, sourceId, minZoom, maxZoom, filter),
    style(style) {};

};
