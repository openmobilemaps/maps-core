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

#include "IconInfoInterface.h"
#include "IconLayerCallbackInterface.h"
#include "IconLayerInterface.h"
#include "MapInterface.h"
#include "SimpleLayerInterface.h"
#include "SimpleTouchInterface.h"
#include "IconLayerObject.h"
#include <atomic>
#include <map>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

class IconLayer : public IconLayerInterface,
                  public SimpleLayerInterface,
                  public SimpleTouchInterface,
                  public std::enable_shared_from_this<IconLayer> {
  public:
    IconLayer();

    ~IconLayer(){};

    // IconLayerInterface
    virtual void setIcons(const std::vector<std::shared_ptr<IconInfoInterface>> &icons) override;

    virtual std::vector<std::shared_ptr<IconInfoInterface>> getIcons() override;

    virtual void remove(const std::shared_ptr<IconInfoInterface> &icon) override;

    virtual void removeList(const std::vector</*not-null*/ std::shared_ptr<IconInfoInterface>> & iconsToRemove) override;

    virtual void add(const std::shared_ptr<IconInfoInterface> &icon) override;

    virtual void removeIdentifier(const std::string & identifier) override;

    virtual void removeIdentifierList(const std::vector<std::string> & identifiers) override;

    void removeIdentifierSet(const std::unordered_set<std::string> &identifiersToRemove);

    virtual void addList(const std::vector</*not-null*/ std::shared_ptr<IconInfoInterface>> &iconsToAdd) override;

    virtual void addIcons(const std::vector<std::shared_ptr<IconInfoInterface>> &iconsToAdd);

    virtual void clear() override;

    virtual void setCallbackHandler(const std::shared_ptr<IconLayerCallbackInterface> &handler) override;

    virtual std::shared_ptr<::LayerInterface> asLayerInterface() override;

    virtual void invalidate() override;

    // LayerInterface

    virtual void setMaskingObject(const std::shared_ptr<::MaskingObjectInterface> &maskingObject) override;

    virtual void update() override;

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() override;

    virtual void onAdded(const std::shared_ptr<MapInterface> &mapInterface, int32_t layerIndex) override;

    virtual void onRemoved() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void hide() override;

    virtual void show() override;

    virtual bool onClickConfirmed(const ::Vec2F &posScreen) override;

    virtual bool onLongPress(const ::Vec2F &posScreen) override;

    void setLayerClickable(bool isLayerClickable) override;

    virtual void setAlpha(float alpha) override;

    virtual float getAlpha() override;

  private:
    virtual void clearSync(const std::vector<std::pair<std::shared_ptr<IconInfoInterface>, std::shared_ptr<IconLayerObject>>> &iconsToClear);

    void
    setupIconObjects(const std::vector<std::pair<std::shared_ptr<IconInfoInterface>, std::shared_ptr<IconLayerObject>>>
                         &iconObjects);

    std::vector<std::shared_ptr<IconInfoInterface>> getIconsAtPosition(const ::Vec2F &posScreen);

    const static int32_t SUBDIVISION_FACTOR_3D_DEFAULT = 2;

    std::shared_ptr<MapInterface> mapInterface;
    bool is3D = false;

    std::shared_ptr<IconLayerCallbackInterface> callbackHandler;

    std::recursive_mutex iconsMutex;
    std::vector<std::pair<std::shared_ptr<IconInfoInterface>, std::shared_ptr<IconLayerObject>>> icons;
    std::shared_ptr<MaskingObjectInterface> mask = nullptr;

    void preGenerateRenderPasses();
    std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> renderPassObjectMap;

    std::recursive_mutex addingQueueMutex;
    std::vector<std::shared_ptr<IconInfoInterface>> addingQueue;

    std::atomic<bool> isHidden;
    std::atomic<bool> isLayerClickable = true;
    float alpha = 1.0;
};
