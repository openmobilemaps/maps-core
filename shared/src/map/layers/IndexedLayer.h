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

#include "IndexedLayerInterface.h"

class IndexedLayer : public IndexedLayerInterface {
public:
    IndexedLayer(int32_t index, const std::shared_ptr<LayerInterface> &layerInterface);

    std::shared_ptr<LayerInterface> getLayerInterface() override;

    int32_t getIndex() override;

private:
    int32_t index;
    std::shared_ptr<LayerInterface> layerInterface;
};
