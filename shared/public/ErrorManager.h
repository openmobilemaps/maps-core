// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from map_helpers.djinni

#pragma once

#include <memory>
#include <string>

class ErrorManagerListener;
struct TiledLayerError;

class ErrorManager {
public:
    virtual ~ErrorManager() = default;

    static /*not-null*/ std::shared_ptr<ErrorManager> create();

    virtual void addTiledLayerError(const TiledLayerError & error) = 0;

    virtual void removeError(const std::string & url) = 0;

    virtual void removeAllErrorsForLayer(const std::string & layerName) = 0;

    virtual void clearAllErrors() = 0;

    virtual void addErrorListener(const /*not-null*/ std::shared_ptr<ErrorManagerListener> & listener) = 0;

    virtual void removeErrorListener(const /*not-null*/ std::shared_ptr<ErrorManagerListener> & listener) = 0;
};
