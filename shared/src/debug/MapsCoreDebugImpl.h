//
//  MapsCoreDebugImpl.h
//  
//
//  Created by Stefan Mitterrutzner on 21.12.22.
//

#pragma once

#include "MapsCoreDebug.h"
#include "MapsCoreDebugInterface.h"

enum class MapsCoreDebugErrorCodes : int32_t {
    PROTOBUF_OUT_OF_BOUNDS,
    PROTOBUF_INVALID_TAG,
    PROTOBUF_UNKNOWN_WIRE_TYPE,
    PROTOBUF_GEOM
};

class MapsCoreDebugImpl {
public:
    static MapsCoreDebugImpl& instance() {
        static MapsCoreDebugImpl singelton;
        return singelton;
    }
    
    void logMessage(MapsCoreDebugErrorCodes code, const std::string & message);
    void logMessage(int32_t code, const std::string & message);
    
    void setDebugDelegate(const std::shared_ptr<MapsCoreDebugInterface> & delegate);
    
private:
    std::shared_ptr<MapsCoreDebugInterface> delegate;
};
