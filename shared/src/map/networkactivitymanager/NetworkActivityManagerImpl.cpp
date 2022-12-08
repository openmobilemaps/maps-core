/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "NetworkActivityManagerImpl.h"
#include "CoordinateConversionHelperInterface.h"

std::shared_ptr<NetworkActivityManager> NetworkActivityManager::create() { return std::make_shared<NetworkActivityManagerImpl>(); }

void NetworkActivityManagerImpl::addNetworkActivityListener(const std::shared_ptr<NetworkActivityListener> &listener) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    listeners.push_back(listener);
}

void NetworkActivityManagerImpl::removeNetworkActivityListener(const std::shared_ptr<NetworkActivityListener> &listener) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    auto it = std::find(listeners.begin(), listeners.end(), listener);
    if (it != listeners.end()) {
        listeners.erase(it);
    }
}

void NetworkActivityManagerImpl::addTiledLayerError(const TiledLayerError &error) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    tiledLayerErrors.insert({error.url, error});
    notifyListeners();
}

void NetworkActivityManagerImpl::removeError(const std::string &url) {
    std::lock_guard<std::recursive_mutex> lock_guard(mutex);
    auto it = tiledLayerErrors.find(url);
    if (it != tiledLayerErrors.end()) {
        tiledLayerErrors.erase(it);
        notifyListeners();
    }
}

void NetworkActivityManagerImpl::removeAllErrorsForLayer(const std::string &layerName) {
    std::lock_guard<std::recursive_mutex> lock_guard(mutex);
    bool hasChanges = false;
    for (auto it = tiledLayerErrors.cbegin(), next_it = it; it != tiledLayerErrors.cend(); it = next_it)
    {
        auto const &[url, error] = *it;
        ++next_it;
        if (error.layerName == layerName) {
            tiledLayerErrors.erase(it);
            hasChanges = true;
        }
    }

    if (hasChanges)
        notifyListeners();
}


void NetworkActivityManagerImpl::clearAllErrors() {
    std::lock_guard<std::recursive_mutex> lock_guard(mutex);
    tiledLayerErrors.clear();
    notifyListeners();
}

void NetworkActivityManagerImpl::updateRemainingTasks(const std::string &layerName, int32_t taskCount) {
    std::lock_guard<std::recursive_mutex> lock_guard(mutex);
    auto it = std::find_if(remainingTasks.begin(), remainingTasks.end(), [&layerName] (const RemainingTasksInfo& i) { return i.layerName == layerName; });
    if (it != remainingTasks.end()) {
        it->count = taskCount;
    } else {
        remainingTasks.emplace_back(RemainingTasksInfo(layerName, taskCount));
    }
    for (const auto &l : listeners) {
        l->onRemainingTasksChanged(remainingTasks);
    }
}

void NetworkActivityManagerImpl::notifyListeners() {
    std::lock_guard<std::recursive_mutex> lock_guard(mutex);
    std::vector<TiledLayerError> errors;
    for (auto const &[url, error] : tiledLayerErrors) {
        errors.push_back(error);
    }
    for (const auto &l : listeners) {
        l->onTiledLayerErrorStateChanged(errors);
    }
}
