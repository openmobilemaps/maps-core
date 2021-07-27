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

#include <mutex>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include "MapInterface.h"
#include "IconLayerInterface.h"
#include "LayerInterface.h"
#include "SimpleTouchInterface.h"
#include "IconInfoInterface.h"
#include "IconLayerCallbackInterface.h"
#include "Textured2dLayerObject.h"

class IconLayer : public IconLayerInterface,
                  public LayerInterface,
                  public SimpleTouchInterface,
                  public std::enable_shared_from_this<IconLayer> {
  public:
    IconLayer();

    ~IconLayer(){};

    // IconLayerInterface
    virtual void setIcons(const std::vector<std::shared_ptr<IconInfoInterface>> & icons) override;

    virtual std::vector<std::shared_ptr<IconInfoInterface>> getIcons() override;

    virtual void remove(const std::shared_ptr<IconInfoInterface> & icon) override;

    virtual void add(const std::shared_ptr<IconInfoInterface> & icon) override;

    virtual void addIcons(const std::vector<std::shared_ptr<IconInfoInterface>> & icons);

    virtual void clear() override;

    virtual void setCallbackHandler(const std::shared_ptr<IconLayerCallbackInterface> & handler) override;

    virtual std::shared_ptr<::LayerInterface> asLayerInterface() override;

    virtual void invalidate() override;

    // LayerInterface

    virtual void setMaskingObject(const std::shared_ptr<::MaskingObjectInterface> & maskingObject) override;

    virtual void update() override {};

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() override;

    virtual void onAdded(const std::shared_ptr<MapInterface> &mapInterface) override;

    virtual void onRemoved() override {};

    virtual void pause() override;

    virtual void resume() override;

    virtual void hide() override;

    virtual void show() override;

    virtual bool onClickConfirmed(const ::Vec2F &posScreen) override;

  private:
    std::shared_ptr<MapInterface> mapInterface;

    std::shared_ptr<IconLayerCallbackInterface> callbackHandler;

    std::recursive_mutex iconsMutex;
    std::unordered_map<std::shared_ptr<IconInfoInterface>, std::shared_ptr<Textured2dLayerObject>> icons;

    void preGenerateRenderPasses();
    std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> renderPassObjectMap;

    std::recursive_mutex addingQueueMutex;
    std::unordered_set<std::shared_ptr<IconInfoInterface>> addingQueue;

    std::atomic<bool> isHidden;
};
