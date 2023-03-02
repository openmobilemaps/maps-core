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

#include "Value.h"
#include "VectorLayerDescription.h"
#include "Coord.h"

class Tiled2dMapVectorLayerSelectionInterface {
public:
    virtual void didSelectFeature(const FeatureContext &featureContext, const std::shared_ptr<VectorLayerDescription> &layer, const ::Coord &coordinate) = 0;
};
