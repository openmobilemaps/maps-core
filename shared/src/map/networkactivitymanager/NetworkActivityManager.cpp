/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "NetworkActivityManager.h"
#include "CoordinateConversionHelperInterface.h"

std::shared_ptr<NetworkActivityManagerInterface> NetworkActivityManagerInterface::create() { return std::make_shared<NetworkActivityManager>(); }

void NetworkActivityManager::addNetworkActivityListener(const std::shared_ptr<NetworkActivityListenerInterface> &listener) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    listeners.push_back(listener);
}

void NetworkActivityManager::removeNetworkActivityListener(const std::shared_ptr<NetworkActivityListenerInterface> &listener) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    auto it = std::find(listeners.begin(), listeners.end(), listener);
    if (it != listeners.end()) {
        listeners.erase(it);
    }
}

void NetworkActivityManager::addTiledLayerError(const TiledLayerError &error) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    tiledLayerErrors.insert({error.url, error});
    notifyListeners();
}

void NetworkActivityManager::removeError(const std::string &url) {
    std::lock_guard<std::recursive_mutex> lock_guard(mutex);
    auto it = tiledLayerErrors.find(url);
    if (it != tiledLayerErrors.end()) {
        tiledLayerErrors.erase(it);
        notifyListeners();
    }
}

void NetworkActivityManager::removeAllErrorsForLayer(const std::string &layerName) {
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


void NetworkActivityManager::clearAllErrors() {
    std::lock_guard<std::recursive_mutex> lock_guard(mutex);
    tiledLayerErrors.clear();
    notifyListeners();
}

void NetworkActivityManager::updateRemainingTasks(const std::string &layerName, int32_t taskCount) {
    std::lock_guard<std::recursive_mutex> lock_guard(mutex);
    auto it = std::find_if(progressInfos.begin(), progressInfos.end(), [&layerName] (const TasksProgressInfo& i) { return i.layerName == layerName; });
    if (it != progressInfos.end()) {
        it->remainingCount = taskCount;
        if (taskCount == 0) {
            it->maxCount = 0;
            it->progress = 1.0;
        } else {
            it->maxCount = std::max(it->maxCount, taskCount);
            it->progress = (float)(it->maxCount - taskCount) / (float)it->maxCount;
        }
    } else {
        if (taskCount == 0) {
            progressInfos.emplace_back(TasksProgressInfo(layerName, 0, 0, 1.0));
        } else {
            progressInfos.emplace_back(TasksProgressInfo(layerName, taskCount, taskCount, 0.0));
        }
    }

    int32_t totalRemaining = 0;
    int32_t totalMax = 0;
    for (const auto &pi: progressInfos) {
        totalRemaining += pi.remainingCount;
        totalMax += pi.maxCount;
    }

    float totalProgress;
    if (totalMax == 0) {
        currentMaxTasks = 0;
        totalProgress = 1;
    } else {
        currentMaxTasks = std::max(currentMaxTasks, totalMax);
        totalProgress = (float)(currentMaxTasks - totalRemaining) / (float)currentMaxTasks;
    }

    for (const auto &l : listeners) {
        l->onTasksProgressChanged(totalProgress, progressInfos);
    }
}

void NetworkActivityManager::notifyListeners() {
    std::lock_guard<std::recursive_mutex> lock_guard(mutex);
    std::vector<TiledLayerError> errors;
    for (auto const &[url, error] : tiledLayerErrors) {
        errors.push_back(error);
    }
    for (const auto &l : listeners) {
        l->onTiledLayerErrorStateChanged(errors);
    }
}
