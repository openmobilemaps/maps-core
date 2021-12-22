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

#include "TextLayerInterface.h"
#include "LayerInterface.h"

#include "Coord.h"
#include "FontLoaderInterface.h"
#include "TextLayerObject.h"
#include "TextInfo.h"
#include "MapInterface.h"

#include <atomic>
#include <mutex>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <vector>

class TextLayer : public TextLayerInterface,
                  public LayerInterface,
                  public std::enable_shared_from_this<TextLayer> {
  public:
    TextLayer(const std::shared_ptr<::FontLoaderInterface> & fontLoader);

    virtual void setTexts(const std::vector<std::shared_ptr<TextInfoInterface>> & texts);

    virtual std::shared_ptr<::LayerInterface> asLayerInterface();

    virtual void invalidate();

    virtual void setMaskingObject(const std::shared_ptr<::MaskingObjectInterface> & maskingObject);

    virtual void update() {};

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses();

    virtual void onAdded(const std::shared_ptr<MapInterface> & mapInterface);

    virtual void onRemoved();

    virtual void pause();

    virtual void resume();

    virtual void hide();

    virtual void show();

  private:
    void clear();
    void generateRenderPasses();

    void add(const std::shared_ptr<TextInfoInterface> &text);
    void addTexts(const std::vector<std::shared_ptr<TextInfoInterface>> &texts);

    std::vector<std::shared_ptr<TextInfoInterface>> getTexts();

  private:
    std::shared_ptr<::FontLoaderInterface> fontLoader;

    std::shared_ptr<MapInterface> mapInterface;

    std::recursive_mutex textMutex;
    std::unordered_map<std::shared_ptr<TextInfoInterface>, std::shared_ptr<TextLayerObject>> texts;

    std::recursive_mutex renderPassMutex;
    std::vector<std::shared_ptr<::RenderPassInterface>> renderPasses;

    std::recursive_mutex addingQueueMutex;
    std::unordered_set<std::shared_ptr<TextInfoInterface>> addingQueue;

    std::atomic<bool> isHidden;
};
