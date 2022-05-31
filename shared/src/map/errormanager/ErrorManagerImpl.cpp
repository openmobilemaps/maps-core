//
//  ErrorManagerImpl.cpp
//  SwisstopoShared
//
//  Created by Zeno Koller on 28.01.21.
//  Copyright © 2021 Ubique Innovation AG. All rights reserved.
//

#include "ErrorManagerImpl.h"
#include "CoordinateConversionHelperInterface.h"

std::shared_ptr<ErrorManager> ErrorManager::create() {
    return std::make_shared<ErrorManagerImpl>();
}

void ErrorManagerImpl::addErrorListener(const std::shared_ptr<ErrorManagerListener> & listener) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    listeners.push_back(listener);
}

void ErrorManagerImpl::removeErrorListener(const std::shared_ptr<ErrorManagerListener> & listener) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    auto it = std::find(listeners.begin(), listeners.end(), listener);
    if (it != listeners.end()) {
        listeners.erase(it);
    }
}

void ErrorManagerImpl::addTiledLayerError(const TiledLayerError & error) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    tiledLayerErrors.insert({error.url, error});
    notifyListeners();
}

void ErrorManagerImpl::removeError(const std::string & url) {
    std::lock_guard<std::recursive_mutex> lock_guard(mutex);
    auto it = tiledLayerErrors.find(url);
    if (it != tiledLayerErrors.end())
    {
        tiledLayerErrors.erase(it);
        notifyListeners();
    }
}

void ErrorManagerImpl::clearAllErrors() {
    std::lock_guard<std::recursive_mutex> lock_guard(mutex);
    tiledLayerErrors.clear();
    notifyListeners();
}

void ErrorManagerImpl::notifyListeners()
{
    std::lock_guard<std::recursive_mutex> lock_guard(mutex);
    std::vector<TiledLayerError> errors;
    for (auto const &[url, error]: tiledLayerErrors) {
        errors.push_back(error);
    }
    for (const auto &l : listeners)
    {
        l->onTiledLayerErrorStateChanged(errors);
    }
}
