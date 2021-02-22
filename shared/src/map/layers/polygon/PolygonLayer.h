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

#include "Polygon2dLayerObject.h"
#include "PolygonCompare.h"
#include "PolygonLayerCallbackInterface.h"
#include "PolygonLayerInterface.h"
#include "SimpleTouchInterface.h"
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class PolygonLayer : public PolygonLayerInterface,
                     public LayerInterface,
                     public SimpleTouchInterface,
                     public std::enable_shared_from_this<PolygonLayer> {
  public:
    PolygonLayer();

    ~PolygonLayer(){};

    // PolygonLayerInterface
    virtual void setPolygons(const std::vector<PolygonInfo> &polygons) override;

    virtual std::vector<PolygonInfo> getPolygons() override;

    virtual void remove(const PolygonInfo &polygon) override;

    virtual void add(const PolygonInfo &polygon) override;

    virtual void clear() override;

    virtual std::shared_ptr<::LayerInterface> asLayerInterface() override;

    virtual void setCallbackHandler(const std::shared_ptr<PolygonLayerCallbackInterface> &handler) override;

    // LayerInterface
    virtual void update() override{};

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() override;

    virtual void onAdded(const std::shared_ptr<MapInterface> &mapInterface) override;

    virtual void onRemoved() override{};

    virtual void pause() override;

    virtual void resume() override;

    virtual void hide() override;

    virtual void show() override;

    virtual bool onTouchDown(const ::Vec2F &posScreen) override;

    virtual bool onClickUnconfirmed(const ::Vec2F &posScreen) override;

    virtual void clearTouch() override;

  private:
    std::shared_ptr<MapInterface> mapInterface;

    std::shared_ptr<PolygonLayerCallbackInterface> callbackHandler;

    std::recursive_mutex polygonsMutex;
    std::unordered_map<PolygonInfo, std::shared_ptr<Polygon2dLayerObject>> polygons;

    void generateRenderPasses();
    std::vector<std::shared_ptr<::RenderPassInterface>> renderPasses;

    std::recursive_mutex addingQueueMutex;
    std::unordered_set<PolygonInfo> addingQueue;

    std::optional<PolygonInfo> highlightedPolygon;

    std::atomic<bool> isHidden;
};
