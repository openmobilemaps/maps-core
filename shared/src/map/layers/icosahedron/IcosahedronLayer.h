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

#include "SimpleLayerInterface.h"
#include "IcosahedronLayerInterface.h"
#include "IcosahedronLayerCallbackInterface.h"

class IcosahedronLayer: public IcosahedronLayerInterface,
    public SimpleLayerInterface,
    public std::enable_shared_from_this<LayerInterface> {
public:
    IcosahedronLayer(const /*not-null*/ std::shared_ptr<IcosahedronLayerCallbackInterface> & callbackHandler);
    ~IcosahedronLayer(){};

    virtual void onAdded(const std::shared_ptr<MapInterface> & mapInterface, int32_t layerIndex) override;

    virtual /*not-null*/ std::shared_ptr<::LayerInterface> asLayerInterface() override;
private:
    const /*not-null*/ std::shared_ptr<IcosahedronLayerCallbackInterface> callbackHandler;
};


