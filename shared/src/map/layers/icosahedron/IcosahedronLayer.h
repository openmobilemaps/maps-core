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
#include "MapInterface.h"
#include "Actor.h"

#include "ColorShaderInterface.h"
#include "IcosahedronInterface.h"

class IcosahedronLayer: public IcosahedronLayerInterface,
public SimpleLayerInterface,
public ActorObject,
public std::enable_shared_from_this<IcosahedronLayer> {
public:
    IcosahedronLayer(const /*not-null*/ std::shared_ptr<IcosahedronLayerCallbackInterface> & callbackHandler);
    ~IcosahedronLayer(){};

    virtual void onAdded(const std::shared_ptr<MapInterface> & mapInterface, int32_t layerIndex) override;

    virtual std::vector<std::shared_ptr< ::RenderPassInterface>> buildRenderPasses() override;

    virtual /*not-null*/ std::shared_ptr<::LayerInterface> asLayerInterface() override;

    void setupObject();
private:
    const /*not-null*/ std::shared_ptr<IcosahedronLayerCallbackInterface> callbackHandler;
    std::shared_ptr<MapInterface> mapInterface;
    std::shared_ptr<ColorShaderInterface> shader;
    std::shared_ptr<IcosahedronInterface> object;

    std::vector<std::shared_ptr< ::RenderPassInterface>> renderPasses;
};


