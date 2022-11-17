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

#include "ErrorManager.h"
#include "ErrorManagerListener.h"
#include "TiledLayerError.h"
#include <mutex>
#include <unordered_map>
#include <vector>

class ErrorManagerImpl : public ErrorManager, public std::enable_shared_from_this<ErrorManagerImpl> {
  public:
    ErrorManagerImpl(){};

    virtual void addErrorListener(const std::shared_ptr<ErrorManagerListener> &listener) override;

    virtual void removeErrorListener(const std::shared_ptr<ErrorManagerListener> &listener) override;

    virtual void addTiledLayerError(const TiledLayerError &error) override;

    virtual void removeError(const std::string &url) override;

    virtual void removeAllErrorsForLayer(const std::string & layerName) override;

    virtual void clearAllErrors() override;

  private:
    std::recursive_mutex mutex;
    std::unordered_map<std::string, TiledLayerError> tiledLayerErrors;
    std::vector<std::shared_ptr<ErrorManagerListener>> listeners;

    bool containsRect(const ::RectCoord &outer, const ::RectCoord &inner);
    void notifyListeners();
};
