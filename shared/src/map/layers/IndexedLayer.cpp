/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "IndexedLayer.h"

IndexedLayer::IndexedLayer(int32_t index, const std::shared_ptr<LayerInterface> &layerInterface) : index(index),
                                                                                                   layerInterface(layerInterface) {}

std::shared_ptr<LayerInterface> IndexedLayer::getLayerInterface() {
    return layerInterface;
}

int32_t IndexedLayer::getIndex() {
    return index;
}
