#pragma once

#include "TaskInterface.h"
#include "TaskConfig.h"
#include <functional>

class LambdaTask: public TaskInterface {
public:
    LambdaTask(TaskConfig config_, std::function<void()> method_);
    virtual TaskConfig getConfig() override;
    virtual void run() override;
private:
    TaskConfig config;
    std::function<void()> method;
};
