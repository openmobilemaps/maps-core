// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from task_scheduler.djinni

#pragma once

#include <memory>

class SchedulerInterface;

class ThreadPoolScheduler {
public:
    virtual ~ThreadPoolScheduler() = default;

    static /*not-null*/ std::shared_ptr<SchedulerInterface> create();
};
