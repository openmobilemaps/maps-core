#include "LambdaTask.h"

LambdaTask::LambdaTask(TaskConfig config_, std::function<void()> method_)
    : config(config_)
    , method(method_) {}

TaskConfig LambdaTask::getConfig() { return config; }

void LambdaTask::run() { method(); }
