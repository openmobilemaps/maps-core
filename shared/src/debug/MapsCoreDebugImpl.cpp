//
//  MapsCoreDebugImpl.cpp
//  
//
//  Created by Stefan Mitterrutzner on 21.12.22.
//

#include "MapsCoreDebugImpl.h"

void MapsCoreDebug::setDebugDelegate(const std::shared_ptr<MapsCoreDebugInterface> & delegate) {
    MapsCoreDebugImpl::instance().setDebugDelegate(delegate);
}

void MapsCoreDebugImpl::logMessage(MapsCoreDebugErrorCodes code, const std::string & message) {
    logMessage((int32_t) code, message);
}

void MapsCoreDebugImpl::logMessage(int32_t code, const std::string & message){
    auto delegate = this->delegate;
    if (!delegate) {
        return;
    }
    delegate->logMessage(code, message);
}

void MapsCoreDebugImpl::setDebugDelegate(const std::shared_ptr<MapsCoreDebugInterface> & delegate) {
    this->delegate = delegate;
}
