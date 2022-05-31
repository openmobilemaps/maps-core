//
//  ErrorManagerImpl.cpp
//  SwisstopoShared
//
//  Created by Zeno Koller on 28.01.21.
//  Copyright © 2021 Ubique Innovation AG. All rights reserved.
//

#include "ErrorManagerImpl.h"
#include "CoordinateConversionHelperInterface.h"
#include "Logger.h"

std::shared_ptr<ErrorManager> ErrorManager::create(const ErrorManagerConfiguration & config, const std::shared_ptr<::MapCamera2dInterface> & camera) {
    auto sharedPtr = std::make_shared<ErrorManagerImpl>(config, camera);
    camera->addListener(sharedPtr);
    return sharedPtr;
}

ErrorManagerImpl::ErrorManagerImpl(const ErrorManagerConfiguration & config, const std::shared_ptr<::MapCamera2dInterface> & camera): config(config), camera(camera) {}


ErrorManagerImpl::~ErrorManagerImpl() {
    camera->removeListener(shared_from_this());
}

void ErrorManagerImpl::setConfiguration(const ErrorManagerConfiguration & config) {
    this->config = config;
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
    LogError << "add Error: " <<= std::to_string(tiledLayerErrors.size());
    notifyListeners();
}

void ErrorManagerImpl::removeError(const std::string & url) {
    std::lock_guard<std::recursive_mutex> lock_guard(mutex);
    auto it = tiledLayerErrors.find(url);
    if (it != tiledLayerErrors.end())
    {
        tiledLayerErrors.erase(it);
        LogError << "remove Error: " <<= std::to_string(tiledLayerErrors.size());
        notifyListeners();
    }
}

void ErrorManagerImpl::clearAllErrors() {
    std::lock_guard<std::recursive_mutex> lock_guard(mutex);
    tiledLayerErrors.clear();
    notifyListeners();
}

bool ErrorManagerImpl::containsRect(const ::RectCoord & outer, const ::RectCoord & inner)
{
    auto const innerConv = CoordinateConversionHelperInterface::independentInstance()->convertRect(outer.topLeft.systemIdentifier, inner);
    auto outerRight = outer.bottomRight.x;
    auto outerBottom = outer.bottomRight.y;
    auto innerRight = innerConv.bottomRight.x;
    auto innerBottom = innerConv.bottomRight.y;
    return (innerRight >= outer.topLeft.x && outerRight >= innerConv.topLeft.x) && (innerBottom >= outer.topLeft.y && outerBottom >= innerConv.topLeft.y);
}


void ErrorManagerImpl::onVisibleBoundsChanged(const ::RectCoord & visibleBounds, double zoom){
    /*if (!config.clearErrorsOnMapBoundsUpdate) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock_guard(mutex);

    auto countBefore = tiledLayerErrors.size();
    tiledLayerErrors.erase(std::remove_if(tiledLayerErrors.begin(), tiledLayerErrors.end(), [&](auto&& e) {
        return !containsRect(visibleBounds, e.bounds);
    }), tiledLayerErrors.end());

    if (countBefore != tiledLayerErrors.size())
    {
        notifyListeners();
    }*/
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
