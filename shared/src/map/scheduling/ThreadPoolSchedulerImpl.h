//
//  ThreadPoolSchedulerImpl.hpp
//  
//
//  Created by Stefan Mitterrutzner on 07.02.23.
//

#pragma once

#include "ThreadPoolScheduler.h"
#include "SchedulerInterface.h"
#include "ThreadPoolCallbacks.h"
#include "TaskInterface.h"
#include "TaskConfig.h"
#include <condition_variable>
#include <mutex>
#include <deque>
#include <thread>
#include <array>

class ThreadPoolSchedulerImpl: public SchedulerInterface {
public:
    ThreadPoolSchedulerImpl(const std::shared_ptr<ThreadPoolCallbacks> &callbacks,
                            bool separateGraphicsQueue);

    ~ThreadPoolSchedulerImpl();
    
    virtual void addTask(const std::shared_ptr<TaskInterface> & task) override;

    virtual void addTasks(const std::vector<std::shared_ptr<TaskInterface>> & tasks) override;

    virtual void removeTask(const std::string & id) override;

    virtual void clear() override;

    virtual void pause() override;

    virtual void resume() override;

    bool hasSeparateGraphicsInvocation() override;

    void runGraphicsTasks() override;

private:
    std::thread makeSchedulerThread(size_t index, TaskPriority priority);
    
    const std::shared_ptr<ThreadPoolCallbacks> callbacks;

    std::mutex defaultMutex;
    std::deque<std::shared_ptr<TaskInterface>> defaultQueue;
    std::condition_variable defaultCv;

    bool separateGraphicsQueue;
    static const uint8_t MAX_NUM_GRAPHICS_TASKS = 12;
    static const uint64_t MAX_TIME_GRAPHICS_TASKS_MS = 6;
    std::mutex graphicsMutex;

    std::deque<std::shared_ptr<TaskInterface>> graphicsQueue;
    static const uint8_t DEFAULT_MAX_NUM_THREADS = 4;
    std::vector<std::thread> threads;

    bool terminated{false};
};
