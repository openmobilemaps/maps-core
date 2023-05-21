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
#include "LineInfoInterface.h"
#include "LineLayerInterface.h"
#include "SimpleLayerInterface.h"
#include "SimpleTouchInterface.h"
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class LineLayer : public LineLayerInterface,
                     public SimpleLayerInterface,
                     public SimpleTouchInterface,
                     public std::enable_shared_from_this<LineLayer> {
  public:
    LineLayer();

    ~LineLayer(){};

    // LineLayerInterface
    virtual void setLines(const std::vector<std::shared_ptr<LineInfoInterface>> & lines) override;

    virtual std::vector<std::shared_ptr<LineInfoInterface>> getLines() override;

    virtual void remove(const std::shared_ptr<LineInfoInterface> & line) override;

    virtual void add(const std::shared_ptr<LineInfoInterface> & line) override;

    virtual void setupLine(const std::shared_ptr<LineGroup2dInterface> & line);

    virtual void clear() override;

    virtual void setCallbackHandler(const std::shared_ptr<LineLayerCallbackInterface> & handler) override;

    virtual std::shared_ptr<::LayerInterface> asLayerInterface() override;

    virtual void invalidate() override;

    // LayerInterface

    virtual void setMaskingObject(const std::shared_ptr<::MaskingObjectInterface> & maskingObject) override;

    virtual void update() override;

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() override;

    virtual void onAdded(const std::shared_ptr<MapInterface> &mapInterface, int32_t layerIndex) override;

    virtual void onRemoved() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void hide() override;

    virtual void show() override;

    virtual bool onTouchDown(const ::Vec2F &posScreen) override;

    virtual bool onClickUnconfirmed(const ::Vec2F &posScreen) override;

    virtual void clearTouch() override;

    void setLayerClickable(bool isLayerClickable) override;

    void resetSelection() override;

    void setSelected(const std::unordered_set<std::string> &selectedIds) override;

protected:
    std::shared_ptr<MapInterface> mapInterface;

    std::shared_ptr<LineLayerCallbackInterface> callbackHandler;

    std::recursive_mutex linesMutex;
    std::vector<std::pair<std::shared_ptr<LineInfoInterface>, std::shared_ptr<Line2dLayerObject>>> lines;
    std::shared_ptr<::MaskingObjectInterface> mask = nullptr;
    std::shared_ptr<GraphicsObjectInterface> maskGraphicsObject = nullptr;

    void generateRenderPasses();
    std::recursive_mutex renderPassMutex;
    std::vector<std::shared_ptr<::RenderPassInterface>> renderPasses;

    std::recursive_mutex addingQueueMutex;
    std::vector<std::shared_ptr<LineInfoInterface>> addingQueue;

    std::vector<std::shared_ptr<LineInfoInterface>> highlightedLines;
    std::vector<std::shared_ptr<LineInfoInterface>> selectedLines;

    std::atomic<bool> isHidden;
    std::atomic<bool> isLayerClickable = true;
};
