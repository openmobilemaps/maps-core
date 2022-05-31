//
//  ErrorManager.h
//  SwisstopoShared
//
//  Created by Zeno Koller on 28.01.21.
//  Copyright © 2021 Ubique Innovation AG. All rights reserved.

#pragma once

#include "ErrorManager.h"
#include "TiledLayerError.h"
#include "ErrorManagerListener.h"
#include <vector>
#include <mutex>
#include <unordered_map>

class ErrorManagerImpl: public ErrorManager,
                         public std::enable_shared_from_this<ErrorManagerImpl> {
public:
   ErrorManagerImpl() {};

    virtual void addErrorListener(const std::shared_ptr<ErrorManagerListener> & listener) override;

    virtual void removeErrorListener(const std::shared_ptr<ErrorManagerListener> & listener) override;

    virtual void addTiledLayerError(const TiledLayerError & error) override;

    virtual void removeError(const std::string & url) override;

    virtual void clearAllErrors() override;

private:
    std::recursive_mutex mutex;
    std::unordered_map<std::string, TiledLayerError> tiledLayerErrors;
    std::vector<std::shared_ptr<ErrorManagerListener>> listeners;

    bool containsRect(const ::RectCoord & outer, const ::RectCoord & inner);
    void notifyListeners();
};
