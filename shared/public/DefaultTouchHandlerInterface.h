// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from touch_handler.djinni

#pragma once

#include "SchedulerInterface.h"
#include <memory>

class TouchHandlerInterface;

class DefaultTouchHandlerInterface {
public:
    virtual ~DefaultTouchHandlerInterface() {}

    static std::shared_ptr<TouchHandlerInterface> create(const std::shared_ptr<::SchedulerInterface> & scheduler, float density);
};
