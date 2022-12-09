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

#include "NetworkActivityManager.h"
#include "NetworkActivityListener.h"
#include "TiledLayerError.h"
#include <mutex>
#include <unordered_map>
#include <vector>
#include "TasksProgressInfo.h"

class NetworkActivityManagerImpl : public NetworkActivityManager, public std::enable_shared_from_this<NetworkActivityManagerImpl> {
  public:
    NetworkActivityManagerImpl(){};

    virtual void addNetworkActivityListener(const std::shared_ptr<NetworkActivityListener> &listener) override;

    virtual void removeNetworkActivityListener(const std::shared_ptr<NetworkActivityListener> &listener) override;

    virtual void addTiledLayerError(const TiledLayerError &error) override;

    virtual void removeError(const std::string &url) override;

    virtual void removeAllErrorsForLayer(const std::string & layerName) override;

    virtual void clearAllErrors() override;

    virtual void updateRemainingTasks(const std::string & layerName, int32_t taskCount) override;

  private:
    std::recursive_mutex mutex;
    std::unordered_map<std::string, TiledLayerError> tiledLayerErrors;
    std::vector<TasksProgressInfo> progressInfos;
    int32_t currentMaxTasks = 0;
    std::vector<std::shared_ptr<NetworkActivityListener>> listeners;

    bool containsRect(const ::RectCoord &outer, const ::RectCoord &inner);
    void notifyListeners();
};
