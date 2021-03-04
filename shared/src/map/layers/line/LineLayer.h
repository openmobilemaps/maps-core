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

#include "Line2dLayerObject.h"
#include "LineCompare.h"
#include "LineLayerCallbackInterface.h"
#include "LineLayerInterface.h"
#include "SimpleTouchInterface.h"
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class LineLayer : public LineLayerInterface,
                     public LayerInterface,
                     public SimpleTouchInterface,
                     public std::enable_shared_from_this<LineLayer> {
  public:
    LineLayer();

    ~LineLayer(){};

    // LineLayerInterface
    virtual void setLines(const std::vector<LineInfo> & lines) override;

    virtual std::vector<LineInfo> getLines() override;

    virtual void remove(const LineInfo & line) override;

    virtual void add(const LineInfo & line) override;

    virtual void clear() override;

    virtual void setCallbackHandler(const std::shared_ptr<LineLayerCallbackInterface> & handler) override;

    virtual std::shared_ptr<::LayerInterface> asLayerInterface() override;

    virtual void invalidate() override;

    // LayerInterface
    virtual void update() override {};

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() override;

    virtual void onAdded(const std::shared_ptr<MapInterface> &mapInterface) override;

    virtual void onRemoved() override {};

    virtual void pause() override;

    virtual void resume() override;

    virtual void hide() override;

    virtual void show() override;

    virtual bool onTouchDown(const ::Vec2F &posScreen) override;

    virtual bool onClickUnconfirmed(const ::Vec2F &posScreen) override;

    virtual void clearTouch() override;

  private:
    std::shared_ptr<MapInterface> mapInterface;

    std::shared_ptr<LineLayerCallbackInterface> callbackHandler;

    std::recursive_mutex linesMutex;
    std::unordered_map<LineInfo, std::shared_ptr<Line2dLayerObject>> lines;

    void generateRenderPasses();
    std::vector<std::shared_ptr<::RenderPassInterface>> renderPasses;

    std::recursive_mutex addingQueueMutex;
    std::unordered_set<LineInfo> addingQueue;

    std::vector<LineInfo> highlightedLines;

    std::atomic<bool> isHidden;
};
