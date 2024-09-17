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

#include "LayerInterface.h"
#include "LayerReadyState.h"

class SimpleLayerInterface : public LayerInterface {

    virtual void setMaskingObject(const std::shared_ptr<::MaskingObjectInterface> & maskingObject) {};

    virtual void update() {};

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() {
        return {};
    };

    virtual std::vector<std::shared_ptr<::ComputePassInterface>> buildComputePasses() {
        return {};
    };

    virtual void onAdded(const std::shared_ptr<MapInterface> & mapInterface, int32_t layerIndex) {};

    virtual void onRemoved() {};

    virtual void pause() {};

    virtual void resume() {};

    virtual void hide() {};

    virtual void show() {};

    virtual void setAlpha(float alpha) {};

    virtual float getAlpha() { return 1.0; };

    /** optional rectangle, remove scissoring when not set */
    virtual void setScissorRect(const std::optional<::RectI> & scissorRect) {};

    virtual LayerReadyState isReadyToRenderOffscreen() { return LayerReadyState::READY; }

    virtual void enableAnimations(bool enabled) {}

    virtual void setErrorManager(const std::shared_ptr<::ErrorManager> & errorManager) {}

    virtual void forceReload() {}
};
